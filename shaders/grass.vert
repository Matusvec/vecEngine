#version 330 core
layout (location = 0) in vec3 aPos;          // blade quad vertex (object space)
layout (location = 1) in vec3 aNormal;       // unused but Vertex layout requires it
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aInstancePos;  // world position of this blade base
layout (location = 4) in vec3 aInstanceRand; // (rotationRadians, heightScale, tintMultiplier)

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform float time;

out vec2 TexCoords;
out vec3 Color;
out vec3 WorldPos;
out vec4 LightSpacePos;
out float FogDistance;

const vec3 BASE_COLOR = vec3(0.18, 0.32, 0.10);
const vec3 TIP_COLOR  = vec3(0.55, 0.80, 0.30);

vec2 windOffset(vec2 worldXZ, float t) {
    vec2 dir1 = vec2(0.70, 0.30);
    vec2 dir2 = vec2(0.50, -0.20);
    float w1 = sin(dot(worldXZ, vec2(0.012, 0.012)) + t * 0.9);
    float w2 = sin(dot(worldXZ, vec2(0.025, 0.030)) + t * 1.4) * 0.4;
    return dir1 * w1 * 0.40 + dir2 * w2 * 0.20;
}

void main() {
    float angle = aInstanceRand.x;
    float c = cos(angle), s = sin(angle);
    vec3 rotated = vec3(aPos.x * c + aPos.z * s, aPos.y, -aPos.x * s + aPos.z * c);

    rotated.y *= aInstanceRand.y;

    vec3 worldPos = rotated + aInstancePos;

    float bend = aTexCoords.y * aTexCoords.y;
    vec2 wind = windOffset(aInstancePos.xz, time) * bend * 0.6;
    worldPos.x += wind.x;
    worldPos.z += wind.y;

    Color = mix(BASE_COLOR, TIP_COLOR, aTexCoords.y);
    Color *= aInstanceRand.z;

    TexCoords = aTexCoords;
    WorldPos = worldPos;
    LightSpacePos = lightSpaceMatrix * vec4(worldPos, 1.0);

    vec4 viewPos = view * vec4(worldPos, 1.0);
    gl_Position = projection * viewPos;
    FogDistance = -viewPos.z;
}
