#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

class LightingSystem;

class Water {
public:
	// seaLevel    — world Y of the rest surface (waves displace ±~3 units around this)
	// size        — side length of the water plane (centered under the camera at draw time)
	// subdivisions — vertex grid resolution per side; more = smoother waves
	Water(float seaLevel, float size, int subdivisions);

	// Centers the plane on cameraPos.xz so the player never sees the water edge.
	// time drives the wave animation (use glfwGetTime()).
	// Caller must have the shadow texture bound to `shadowMapUnit` already.
	void draw(const glm::mat4& view, const glm::mat4& projection,
	          const glm::vec3& cameraPos, float time,
	          const LightingSystem& lighting,
	          const glm::mat4& lightSpaceMatrix,
	          int shadowMapUnit) const;

private:
	Mesh mesh;
	Shader shader;
	float seaLevel;
};
