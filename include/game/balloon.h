#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <vector>

class Terrain;

// Floating sphere targets the player shoots with missiles. Each pop awards
// points and removes the balloon.
class BalloonSystem {
public:
	struct Balloon {
		glm::vec3 position;
		glm::vec3 color;
		float radius;
		bool alive = true;
	};

	BalloonSystem();

	// Procedurally place balloons across the world above terrain.
	void seed(int count, float spread, float minHeightAboveTerrain,
	          float maxHeightAboveTerrain);

	// Test if any alive balloon overlaps a sphere at point/radius. Returns the
	// points value (color-coded) of the popped balloon, or 0 if no hit.
	int popOverlap(const glm::vec3& point, float pointRadius);

	void draw(const glm::mat4& view, const glm::mat4& projection,
	          float time, const glm::vec3& cameraPos) const;

	// Bring every balloon back to alive (used on game restart).
	void reset();

	int aliveCount() const;
	int total() const { return (int)balloons.size(); }

private:
	Mesh sphereMesh;
	Shader shader;
	std::vector<Balloon> balloons;
};
