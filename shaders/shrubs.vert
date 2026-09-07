#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aInstancePos;
layout (location = 4) in vec3 aInstanceRand;  // (rotation, scale, tint)

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform float time;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out vec4 LightSpacePos;
out float Tint;
out float FogDistance;

// Subtle wind sway — bushes are heavier than grass so they move less.
vec2 windOffset(vec2 worldXZ, float t) {
    float w1 = sin(dot(worldXZ, vec2(0.020, 0.018)) + t * 0.7);
    float w2 = sin(dot(worldXZ, vec2(0.045, 0.040)) + t * 1.3) * 0.4;
    return vec2(0.7, 0.3) * w1 * 0.18 + vec2(0.5, -0.2) * w2 * 0.10;
}

void main() {
    float angle = aInstanceRand.x;
    float scale = aInstanceRand.y;
    Tint = aInstanceRand.z;

    float c = cos(angle), s = sin(angle);
    vec3 rotated = vec3(aPos.x * c + aPos.z * s, aPos.y, -aPos.x * s + aPos.z * c);
    rotated *= scale;

    vec3 worldPos = rotated + aInstancePos;

    // Top of the bush sways more than the base.
    float bend = aTexCoords.y;
    vec2 wind = windOffset(aInstancePos.xz, time) * bend;
    worldPos.x += wind.x;
    worldPos.z += wind.y;

    // Rotate the per-quad normal too so lighting reacts to bush orientation.
    vec3 nrm = vec3(aNormal.x * c + aNormal.z * s,
                    aNormal.y,
                    -aNormal.x * s + aNormal.z * c);
    Normal = normalize(nrm);

    TexCoords = aTexCoords;
    WorldPos = worldPos;
    LightSpacePos = lightSpaceMatrix * vec4(worldPos, 1.0);

    vec4 viewPos = view * vec4(worldPos, 1.0);
    gl_Position = projection * viewPos;
    FogDistance = -viewPos.z;
}
