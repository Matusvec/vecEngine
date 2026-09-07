#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 WorldPos;
out vec3 Normal;
out float FogDistance;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;

    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;
    FogDistance = -viewPos.z;
}
