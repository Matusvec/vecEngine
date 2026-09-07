#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;        // caller passes view with translation stripped
uniform mat4 projection;

out vec3 viewDir;

void main() {
    viewDir = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
