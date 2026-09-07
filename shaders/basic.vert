#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 LightSpacePos;
out float FogDistance;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;

    // Terrain model is identity, but transpose-inverse keeps this correct if
    // a future caller ever rotates/scales chunks.
    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoords = aTexCoords;
    LightSpacePos = lightSpaceMatrix * worldPos;

    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;
    FogDistance = -viewPos.z;
}
