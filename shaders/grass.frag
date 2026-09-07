#version 330 core
in vec2 TexCoords;
in vec3 Color;
in vec3 WorldPos;
in vec4 LightSpacePos;
in float FogDistance;
out vec4 FragColor;

uniform sampler2D shadowMap;
uniform vec3  sunDir;
uniform vec3  sunColor;
uniform float sunAmbient;

const vec3 FOG_COLOR  = vec3(0.62, 0.65, 0.70);
const float FOG_START = 80.0;
const float FOG_END   = 220.0;

float sampleShadow(vec4 lightSpacePos, vec3 lightDir) {
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    // Larger fixed bias for grass — its "normal" is faked as up, so slope-scale
    // doesn't help and a flat bias avoids the leaning-blade self-shadow.
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
    float fogFactor = clamp((FOG_END - FogDistance) / (FOG_END - FOG_START), 0.0, 1.0);
    if (fogFactor < 0.02) discard;

    // Treat each blade as a vertical sliver — most accurate normal for diffuse
    // shading is "facing-up" since blades are thin and we want sun-on-field
    // rather than sun-on-blade.
    vec3 lightDir = normalize(sunDir);
    float diff = max(lightDir.y, 0.0);

    vec3 ambient = Color * sunColor * sunAmbient;
    vec3 diffuse = Color * sunColor * diff;

    float shadow = sampleShadow(LightSpacePos, lightDir);
    vec3 lit = ambient + (1.0 - shadow) * diffuse;

    vec3 finalColor = mix(FOG_COLOR, lit, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
