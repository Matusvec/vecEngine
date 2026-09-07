#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform float time;

out vec3 WorldPos;
out vec3 WaveNormal;
out vec4 LightSpacePos;
out float FogDistance;

// Long-wavelength swells so flying over water at speed doesn't make the surface
// look like white noise. Wavelengths ~250-500 units.
float waveHeight(vec2 p) {
    float h = 0.0;
    h += sin(p.x * 0.013 + time * 0.6) * 1.50;
    h += sin(p.y * 0.011 + time * 0.4) * 1.20;
    h += sin((p.x + p.y) * 0.025 + time * 1.0) * 0.65;
    h += sin((p.x - p.y) * 0.022 + time * 0.8) * 0.50;
    return h;
}

void main() {
    vec3 worldPos = (model * vec4(aPos, 1.0)).xyz;
    worldPos.y += waveHeight(worldPos.xz);

    // Finite-difference normal — gives proper per-vertex shading reaction to waves.
    float e = 1.0;
    float hL = waveHeight(worldPos.xz - vec2(e, 0.0));
    float hR = waveHeight(worldPos.xz + vec2(e, 0.0));
    float hD = waveHeight(worldPos.xz - vec2(0.0, e));
    float hU = waveHeight(worldPos.xz + vec2(0.0, e));
    WaveNormal = normalize(vec3(hL - hR, 2.0 * e, hD - hU));

    WorldPos = worldPos;
    vec4 viewPos = view * vec4(worldPos, 1.0);
    gl_Position = projection * viewPos;
    LightSpacePos = lightSpaceMatrix * vec4(worldPos, 1.0);
    FogDistance = -viewPos.z;
}
