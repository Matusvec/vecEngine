#include "game/missile.h"

#include "game/balloon.h"
#include "world/terrain.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {
// Muzzle velocity in plane-relative direction (player velocity is added on top).
constexpr float MISSILE_SPEED    = 280.0f;
constexpr float MISSILE_LIFETIME = 3.5f;
constexpr float CRATER_RADIUS    = 12.0f;
constexpr float CRATER_DEPTH     = 16.0f;
constexpr float MISSILE_SCALE    = 0.22f;
constexpr float SPAWN_OFFSET     = 3.0f;  // ahead of camera so it doesn't clip the player
}

MissileSystem::MissileSystem()
	: cube(Mesh::createCube()),
	  shader("shaders/missile.vert", "shaders/missile.frag") {}

void MissileSystem::fire(const glm::vec3& position, const glm::vec3& direction,
                         const glm::vec3& playerVelocity) {
	Missile m;
	m.position = position + direction * SPAWN_OFFSET;
	// Inherit the plane's velocity so the bullet visibly leaves the muzzle and
	// doesn't appear to fall behind a fast-moving plane.
	m.velocity = direction * MISSILE_SPEED + playerVelocity;
	m.lifetime = MISSILE_LIFETIME;
	missiles.push_back(m);
}

void MissileSystem::update(float dt, Terrain& terrain) {
	for (size_t i = 0; i < missiles.size();) {
		Missile& m = missiles[i];
		m.position += m.velocity * dt;
		m.lifetime -= dt;

		float groundY = Terrain::heightAt(m.position.x, m.position.z);
		bool hit = (m.position.y <= groundY);
		bool expired = (m.lifetime <= 0.0f);

		if (hit) {
			terrain.addCrater(m.position, CRATER_RADIUS, CRATER_DEPTH);
		}

		if (hit || expired) {
			// swap-and-pop so iteration index stays valid without shifting the vector
			missiles[i] = missiles.back();
			missiles.pop_back();
		} else {
			++i;
		}
	}
}

int MissileSystem::checkBalloonHits(BalloonSystem& balloons) {
	int points = 0;
	for (size_t i = 0; i < missiles.size();) {
		int p = balloons.popOverlap(missiles[i].position, 0.6f);
		if (p > 0) {
			points += p;
			// swap-and-pop — same trick as terrain-hit removal in update().
			missiles[i] = missiles.back();
			missiles.pop_back();
		} else {
			++i;
		}
	}
	return points;
}

void MissileSystem::draw(const glm::mat4& view, const glm::mat4& projection) const {
	if (missiles.empty()) return;

	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);

	for (const Missile& m : missiles) {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), m.position);
		model = glm::scale(model, glm::vec3(MISSILE_SCALE));
		shader.setMat4("model", model);
		cube.draw();
	}
}
