#pragma once

#include <glm/glm.hpp>

class Shader;

// Single directional light — the sun. Direction points TOWARD the light source
// (i.e. from a fragment's surface up toward the sun in the sky), so dot(N, dir)
// gives the standard diffuse term without negation.
struct DirectionalLight {
	glm::vec3 direction = glm::normalize(glm::vec3(0.50f, 0.70f, 0.35f));
	glm::vec3 color     = glm::vec3(1.00f, 0.95f, 0.82f);
	float ambient          = 0.35f;
	float specularStrength = 0.45f;
	float shininess        = 32.0f;
};

class LightingSystem {
public:
	DirectionalLight sun;

	// Pushes all per-frame lighting uniforms to the bound program. Caller is
	// responsible for binding the shadow texture to `shadowMapUnit` before any
	// draw that samples it (we just tell the shader which unit to read).
	void apply(const Shader& shader,
	           const glm::vec3& cameraPos,
	           const glm::mat4& lightSpaceMatrix,
	           int shadowMapUnit) const;
};
