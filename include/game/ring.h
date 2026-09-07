#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <vector>

// Sequence of waypoint rings the player flies through to score. Each ring has
// a position, a forward axis (the direction the player must cross), and a
// passed flag. The "next active" ring is highlighted; the rest are dim.
class RingSystem {
public:
	struct Ring {
		glm::vec3 position;
		glm::vec3 forward;  // axis the player must cross; normalized
		float radius = 12.0f;
		bool passed = false;
	};

	RingSystem();

	// Procedurally generate a course around the world.
	void seed(int count, float courseRadius, float minHeight, float maxHeight);

	// Inject a "tutorial" first ring at a specific position so the player
	// can fly straight through it to verify the system works.
	void prependRing(const glm::vec3& position, const glm::vec3& forward, float radius);

	// Test for player crossing the active ring. Returns points awarded this
	// frame (200 per ring), and advances the active index on success.
	int update(const glm::vec3& playerPos);

	// Mark every ring un-passed and reset the course back to ring 0.
	void reset();

	void draw(const glm::mat4& view, const glm::mat4& projection,
	          float time, const glm::vec3& cameraPos) const;

	int activeIndex() const { return active; }
	int passedCount() const { return active; }
	int total() const { return (int)rings.size(); }
	bool allPassed() const { return active >= (int)rings.size(); }

	// Public so the game class can teleport the camera near the next ring etc.
	const std::vector<Ring>& list() const { return rings; }

private:
	Mesh torusMesh;
	Shader shader;
	std::vector<Ring> rings;
	int active = 0;
	glm::vec3 lastPlayerPos{0.0f};
	bool hasLastPos = false;
};
