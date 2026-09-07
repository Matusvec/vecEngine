#version 330 core
in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 LightSpacePos;
in float FogDistance;
out vec4 FragColor;

uniform sampler2D textureSampler;
uniform sampler2D shadowMap;

uniform vec3  sunDir;
uniform vec3  sunColor;
uniform float sunAmbient;
uniform float sunSpecular;
uniform float sunShininess;
uniform vec3  cameraPos;

// Altitude biome bands, calibrated for a Tatra-style profile:
//   • thin sand strip just above the waterline (sea is at y = -10)
//   • broad grass meadows + foothills
//   • forest mid-elevation
//   • bare grey rock for the bulk of the mountain
//   • snow only on the highest peaks
const float SAND_TOP   = -2.0;   // sand fades into grass right above the shore
const float GRASS_TOP  = 90.0;
const float FOREST_TOP = 180.0;
const float ROCK_TOP   = 260.0;

const vec3 SAND_COLOR   = vec3(0.78, 0.70, 0.50);
const vec3 FOREST_COLOR = vec3(0.15, 0.28, 0.10);
// Cool limestone-grey, NOT brown — mountains should read as stone, not dirt.
const vec3 ROCK_COLOR   = vec3(0.52, 0.50, 0.48);
const vec3 SNOW_COLOR   = vec3(0.95, 0.96, 0.98);

const vec3 FOG_COLOR  = vec3(0.62, 0.65, 0.70);
const float FOG_START = 100.0;
const float FOG_END   = 400.0;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float sampleShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0008);
    float current = proj.z;

    float shadow = 0.0;
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float depth = texture(shadowMap, proj.xy + vec2(x, y) * texel).r;
            shadow += (current - bias) > depth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 grassTex = texture(textureSampler, TexCoords).rgb;

    // Two-octave per-fragment detail noise — adds visible grain even on solid
    // biome bands (snow/rock/forest) that otherwise look flat near the camera.
    float n1 = valueNoise(WorldPos.xz * 0.7);
    float n2 = valueNoise(WorldPos.xz * 3.1);
    float detail = 0.85 + (n1 * 0.6 + n2 * 0.4) * 0.30;  // [0.85, 1.15]

    // Altitude blend: sand → grass-textured → forest → rock → snow.
    // Each smoothstep band uses a wide enough range that neighboring layers
    // visibly cross-fade rather than hard-stepping at the boundary.
    float h = WorldPos.y;
    vec3 c = SAND_COLOR;
    c = mix(c, grassTex,     smoothstep(SAND_TOP - 4.0,    SAND_TOP + 10.0,  h));
    c = mix(c, FOREST_COLOR, smoothstep(GRASS_TOP - 30.0,  GRASS_TOP + 30.0, h));
    c = mix(c, ROCK_COLOR,   smoothstep(FOREST_TOP - 30.0, FOREST_TOP + 30.0, h));
    c = mix(c, SNOW_COLOR,   smoothstep(ROCK_TOP - 25.0,   ROCK_TOP + 25.0,  h));

    // Steep slopes always show rock regardless of altitude — keeps cliff faces
    // looking right even when they exist in a "grass" or "snow" band.
    float slope = 1.0 - norm.y;
    c = mix(c, ROCK_COLOR, smoothstep(0.40, 0.75, slope));

    c *= detail;

    vec3 lightDir   = normalize(sunDir);
    vec3 viewDir    = normalize(cameraPos - WorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), sunShininess) * sunSpecular;

    vec3 ambient  = c * sunColor * sunAmbient;
    vec3 diffuse  = c * sunColor * diff;
    vec3 specular = sunColor * spec;

    float shadow = sampleShadow(LightSpacePos, norm, lightDir);
    vec3 lit = ambient + (1.0 - shadow) * (diffuse + specular);

    float fogFactor = clamp((FOG_END - FogDistance) / (FOG_END - FOG_START), 0.0, 1.0);
    vec3 finalColor = mix(FOG_COLOR, lit, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
