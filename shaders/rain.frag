#version 330 core
in float Alpha;
out vec4 FragColor;

const vec3 RAIN_COLOR = vec3(0.78, 0.82, 0.88);

void main() {
    FragColor = vec4(RAIN_COLOR, 1.0);
}
