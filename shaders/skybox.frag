#version 330 core
in vec3 viewDir;
out vec4 FragColor;

uniform float time;
uniform vec3 sunDir;  // direction TOWARD the sun in world space

// Overcast / stormy palette — flatter, grayer, gloomier.
const vec3 ZENITH        = vec3(0.45, 0.50, 0.55);
const vec3 HORIZON       = vec3(0.62, 0.65, 0.70);
const vec3 BELOW         = vec3(0.10, 0.12, 0.10);
const vec3 SUN_COL       = vec3(1.00, 0.95, 0.80);
const vec3 CLOUD_BRIGHT  = vec3(0.78, 0.78, 0.80);
const vec3 CLOUD_SHADOW  = vec3(0.38, 0.40, 0.45);

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash21(i),                hash21(i + vec2(1.0, 0.0)), f.x),
        mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; ++i) {
        v += valueNoise(p) * a;
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec3 dir = normalize(viewDir);
    vec3 sun = normalize(sunDir);

    float t = clamp(dir.y, 0.0, 1.0);
    t = pow(t, 0.65);
    vec3 sky = mix(HORIZON, ZENITH, t);

    float below = smoothstep(0.08, 0.5, -dir.y);  // fog colour hugs the horizon, so a ground plane never shows an edge
    sky = mix(sky, BELOW, below);

    if (dir.y > 0.05) {
        vec2 cloudUV = dir.xz / max(dir.y, 0.15);
        cloudUV = clamp(cloudUV, vec2(-50.0), vec2(50.0));
        cloudUV *= 0.35;
        cloudUV += vec2(time * 0.025, time * 0.015);

        float cloudNoise = fbm(cloudUV);
        float coverage = smoothstep(0.20, 0.55, cloudNoise);

        float sunInfluence = max(0.0, dot(dir, sun));
        vec3 cloudColor = mix(CLOUD_SHADOW, CLOUD_BRIGHT, sunInfluence * 0.5 + 0.5);

        float horizonFade = smoothstep(0.05, 0.30, dir.y);
        sky = mix(sky, cloudColor, coverage * horizonFade);
    }

    // Faint glow where the sun would punch through the cloud cover.
    float sunDot = dot(dir, sun);
    float bigGlow = smoothstep(0.65, 1.0, sunDot) * 0.06;
    sky += SUN_COL * bigGlow;

    FragColor = vec4(sky, 1.0);
}
