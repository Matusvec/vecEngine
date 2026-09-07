#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <vector>

class Terrain;
class BalloonSystem;

class MissileSystem {
public:
	MissileSystem();

	// Spawns a new missile at position, traveling in direction (assumed normalized).
	// playerVelocity is added on top of the muzzle velocity so missiles fired from
	// a moving plane don't appear to "drop back" relative to the player.
	void fire(const glm::vec3& position, const glm::vec3& direction,
	          const glm::vec3& playerVelocity = glm::vec3(0.0f));

	// Advances all active missiles. On ground hit, asks the terrain to carve a crater.
	void update(float dt, Terrain& terrain);

	// Tests every active missile against the balloon set; pops + removes any
	// missile that hit a balloon. Returns the total points awarded this frame.
	int checkBalloonHits(BalloonSystem& balloons);

	void draw(const glm::mat4& view, const glm::mat4& projection) const;

	int activeMissiles() const { return (int)missiles.size(); }

	// Drop every active missile (used on game restart).
	void clear() { missiles.clear(); }

private:
	struct Missile {
		glm::vec3 position;
		glm::vec3 velocity;
		float lifetime;
	};

	std::vector<Missile> missiles;
	Mesh cube;
	Shader shader;
};
