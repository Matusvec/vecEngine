#pragma once

#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <climits>
#include <vector>

class LightingSystem;

class Grass {
public:
	// seaLevel  — blades are skipped where terrain dips at or below this Y
	// radius    — blades exist within this many world units of the camera
	// cellSize  — one blade per (cellSize × cellSize) cell of the world XZ grid
	Grass(float seaLevel, float radius, float cellSize);
	~Grass();

	Grass(const Grass&) = delete;
	Grass& operator=(const Grass&) = delete;

	// Regenerates the blade set + uploads to the GPU when the camera crosses a unit cell.
	// time is glfwGetTime() — used for rate-limiting regen so very fast flight
	// doesn't melt the CPU.
	void update(const glm::vec3& cameraPos, double time);

	// Pure draw — same instance buffer the regen uploaded; no per-frame CPU work,
	// no per-frame upload. Caller must have shadow texture bound to `shadowMapUnit`.
	void draw(const glm::mat4& view, const glm::mat4& projection, float time,
	          const glm::vec3& cameraPos,
	          const LightingSystem& lighting,
	          const glm::mat4& lightSpaceMatrix,
	          int shadowMapUnit) const;

	int instanceCount() const { return instanceCountCached; }

	// Per-blade data shared on CPU and uploaded to GPU as instance attributes.
	struct Blade {
		glm::vec3 position;
		glm::vec3 randoms;  // (rotationRadians, heightScale, tintMultiplier)
	};

	// Axis-aligned rectangles (minX, minZ, maxX, maxZ) that get no blades: runways, aprons, roads.
	// Set before the first update(); call regenerate through update() after changing them.
	void addPaved(float minX, float minZ, float maxX, float maxZ) { paved.push_back({minX, minZ, maxX, maxZ}); }

private:
	void regenerate(const glm::vec3& cameraPos);
	std::vector<glm::vec4> paved;

	Shader shader;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int instanceVBO = 0;

	float seaLevel;
	float radius;
	float cellSize;

	int lastCellX = INT_MIN;
	int lastCellZ = INT_MIN;
	double lastRegenTime = -1.0;     // sentinel: forces first-call regen regardless of throttle
	float lastRegenX = 0.0f;
	float lastRegenZ = 0.0f;
	int instanceCountCached = 0;     // matches what's currently in the GPU instance VBO
};
