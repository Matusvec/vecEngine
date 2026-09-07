#version 330 core
layout (location = 0) in vec3 aPos;          // streak quad vertex
layout (location = 1) in vec3 aNormal;       // unused but Vertex layout requires it
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aInstanceData; // (x_offset, initial_phase [0,1], z_offset)

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform float time;

const float FALL_SPEED = 35.0;
const float BOX_HEIGHT = 80.0;

out float Alpha;

void main() {
    // Each drop has an initial phase; mod with time to cycle continuously.
    float phase = fract(aInstanceData.y + time * (FALL_SPEED / BOX_HEIGHT));
    // Top of box → bottom as phase advances. Box centered on camera Y.
    float dropY = cameraPos.y + BOX_HEIGHT * 0.5 - phase * BOX_HEIGHT;

    vec3 base = vec3(cameraPos.x + aInstanceData.x, dropY, cameraPos.z + aInstanceData.z);
    vec3 worldPos = aPos + base;

    Alpha = 1.0 - phase;  // fade slightly as it falls

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
