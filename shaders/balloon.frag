#version 330 core
in vec3 WorldPos;
in vec3 Normal;
in float FogDistance;
out vec4 FragColor;

uniform vec3 balloonColor;
uniform vec3 cameraPos;

const vec3 SUN_DIR = normalize(vec3(0.50, 0.70, 0.35));
const vec3 FOG_COLOR = vec3(0.62, 0.65, 0.70);
const float FOG_START = 100.0;
const float FOG_END   = 400.0;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - WorldPos);

    // Quick rim-lit shading — small target spheres look better with a clear
    // bright edge against the gloomy sky.
    float diff = max(dot(norm, SUN_DIR), 0.0) * 0.7 + 0.3;
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 2.5);

    vec3 lit = balloonColor * diff + vec3(1.0) * rim * 0.3;

    float fogFactor = clamp((FOG_END - FogDistance) / (FOG_END - FOG_START), 0.0, 1.0);
    FragColor = vec4(mix(FOG_COLOR, lit, fogFactor), 1.0);
}
