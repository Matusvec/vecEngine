#version 330 core
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
in vec4 LightSpacePos;
in float Tint;
in float FogDistance;
out vec4 FragColor;

uniform sampler2D shadowMap;
uniform vec3  sunDir;
uniform vec3  sunColor;
uniform float sunAmbient;

const vec3 BUSH_BASE = vec3(0.10, 0.20, 0.06);
const vec3 BUSH_TIP  = vec3(0.30, 0.48, 0.18);
const vec3 FOG_COLOR = vec3(0.62, 0.65, 0.70);
const float FOG_START = 100.0;
const float FOG_END   = 400.0;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float sampleShadow(vec4 lightSpacePos, vec3 lightDir) {
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    float bias = 0.0025;
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
    // Distance-from-center mask + leafy hash jitter → roundish bush silhouette
    // via alpha discard (no blending needed → no sort order issues).
    vec2 d = TexCoords - 0.5;
    float r = length(d) * 1.50;

    vec2 cell = floor(TexCoords * 9.0);
    float n = hash21(cell);
    float mask = r + n * 0.18;
    if (mask > 0.78) discard;

    // Per-fragment leaf darkness for variation across the foliage.
    float leafShade = 0.85 + n * 0.30;

    vec3 color = mix(BUSH_BASE, BUSH_TIP, TexCoords.y) * Tint * leafShade;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(sunDir);

    // Use mostly-up effective normal — bush canopy reads as a single mass under
    // the sun, not as flat panels with sharp dot-product contrast.
    float diff = mix(max(lightDir.y, 0.0),
                     max(dot(norm, lightDir), 0.0),
                     0.35);

    vec3 ambient = color * sunColor * sunAmbient;
    vec3 diffuse = color * sunColor * diff;

    float shadow = sampleShadow(LightSpacePos, lightDir);
    vec3 lit = ambient + (1.0 - shadow) * diffuse;

    float fogFactor = clamp((FOG_END - FogDistance) / (FOG_END - FOG_START), 0.0, 1.0);
    if (fogFactor < 0.02) discard;

    vec3 finalColor = mix(FOG_COLOR, lit, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
