#version 330 core
in vec3 WorldPos;
in vec3 Normal;
in float FogDistance;
out vec4 FragColor;

uniform vec3 cameraPos;
uniform float time;
uniform int ringState;  // 0 = passed (dim), 1 = active (bright pulse), 2 = upcoming (faded)

const vec3 PASSED_COLOR   = vec3(0.30, 0.32, 0.35);
const vec3 ACTIVE_COLOR   = vec3(0.20, 0.95, 1.00);
const vec3 UPCOMING_COLOR = vec3(0.95, 0.75, 0.30);
const vec3 FOG_COLOR = vec3(0.62, 0.65, 0.70);
const float FOG_START = 100.0;
const float FOG_END   = 400.0;

void main() {
    vec3 base;
    float pulse = 1.0;
    if (ringState == 0)      base = PASSED_COLOR;
    else if (ringState == 1) {
        base = ACTIVE_COLOR;
        // Bright slow throb so the active ring is unmistakable from far off.
        pulse = 0.85 + 0.40 * sin(time * 3.5);
    }
    else                     base = UPCOMING_COLOR;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - WorldPos);
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 1.5);
    vec3 lit = base * pulse + vec3(1.0) * rim * 0.4;

    float fogFactor = clamp((FOG_END - FogDistance) / (FOG_END - FOG_START), 0.0, 1.0);
    FragColor = vec4(mix(FOG_COLOR, lit, fogFactor), 1.0);
}
