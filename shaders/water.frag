#version 330 core
in vec3 WorldPos;
in vec3 WaveNormal;
in vec4 LightSpacePos;
in float FogDistance;
out vec4 FragColor;

uniform sampler2D shadowMap;

uniform vec3  sunDir;
uniform vec3  sunColor;
uniform float sunAmbient;
uniform float sunSpecular;
uniform float sunShininess;  // unused for water (uses a tighter constant)
uniform vec3  cameraPos;

const vec3 WATER_DEEP    = vec3(0.04, 0.16, 0.30);
const vec3 WATER_SHALLOW = vec3(0.18, 0.42, 0.55);
const vec3 FOG_COLOR     = vec3(0.62, 0.65, 0.70);
const float FOG_START    = 100.0;
const float FOG_END      = 400.0;

// Alpha mapping: top-down (low fresnel) is more transparent so submerged
// terrain shows through; glancing angles are nearly opaque so distant water
// reads as a solid sheet.
const float ALPHA_TOP_DOWN = 0.55;
const float ALPHA_GLANCING = 0.92;

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
    vec3 norm = normalize(WaveNormal);
    vec3 viewDir = normalize(cameraPos - WorldPos);
    vec3 lightDir = normalize(sunDir);

    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    vec3 base = mix(WATER_DEEP, WATER_SHALLOW, fresnel);

    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 96.0) * sunSpecular * 2.5;
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 ambient  = base * sunColor * sunAmbient;
    vec3 diffuse  = base * sunColor * diff;
    vec3 specular = sunColor * spec;

    float shadow = sampleShadow(LightSpacePos, norm, lightDir);
    vec3 lit = ambient + (1.0 - shadow) * (diffuse + specular);

    float fogFactor = clamp((FOG_END - FogDistance) / (FOG_END - FOG_START), 0.0, 1.0);
    vec3 rgb = mix(FOG_COLOR, lit, fogFactor);

    // Fresnel-driven opacity: clear when looking down, opaque at the horizon.
    float alpha = mix(ALPHA_TOP_DOWN, ALPHA_GLANCING, fresnel);
    FragColor = vec4(rgb, alpha);
}
