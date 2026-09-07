#pragma once

#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <climits>

class LightingSystem;

// Instanced shrubbery — small 3D bushes scattered across an altitude band.
// Same upload-once / draw-every-frame pattern as Grass; regenerates only when
// the camera crosses a unit cell.
class Shrubs {
public:
	// minY/maxY  — altitude window blades may spawn in (matches a terrain biome band)
	// radius     — bushes exist within this many world units of the camera
	// cellSize   — one candidate per (cellSize × cellSize) cell of the world XZ grid
	Shrubs(float minY, float maxY, float radius, float cellSize);
	~Shrubs();

	Shrubs(const Shrubs&) = delete;
	Shrubs& operator=(const Shrubs&) = delete;

	void update(const glm::vec3& cameraPos, double time);

	void draw(const glm::mat4& view, const glm::mat4& projection, float time,
	          const glm::vec3& cameraPos,
	          const LightingSystem& lighting,
	          const glm::mat4& lightSpaceMatrix,
	          int shadowMapUnit) const;

	int instanceCount() const { return instanceCountCached; }

	// Per-bush instance data (position + packed randoms).
	struct Instance {
		glm::vec3 position;
		glm::vec3 randoms;  // (rotationRadians, scaleMultiplier, tintMultiplier)
	};

private:
	void regenerate(const glm::vec3& cameraPos);

	Shader shader;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int instanceVBO = 0;
	int indexCount = 0;

	float minY;
	float maxY;
	float radius;
	float cellSize;

	int lastCellX = INT_MIN;
	int lastCellZ = INT_MIN;
	double lastRegenTime = -1.0;
	float lastRegenX = 0.0f;
	float lastRegenZ = 0.0f;
	int instanceCountCached = 0;
};
