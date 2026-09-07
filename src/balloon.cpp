#include "game/balloon.h"

#include "world/terrain.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdint>

namespace {

// Procedural UV sphere — modest tessellation, plenty for small target balls.
Mesh makeSphereMesh(float radius, int latSegments, int longSegments) {
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	const float PI = 3.14159265358979f;

	for (int lat = 0; lat <= latSegments; ++lat) {
		float theta = (float)lat / latSegments * PI;       // 0 .. PI
		float sinT = std::sin(theta);
		float cosT = std::cos(theta);
		for (int lon = 0; lon <= longSegments; ++lon) {
			float phi = (float)lon / longSegments * 2.0f * PI;
			float sinP = std::sin(phi);
			float cosP = std::cos(phi);
			glm::vec3 n(sinT * cosP, cosT, sinT * sinP);
			verts.push_back({n * radius, n,
			                 {(float)lon / longSegments, (float)lat / latSegments}});
		}
	}

	int row = longSegments + 1;
	for (int lat = 0; lat < latSegments; ++lat) {
		for (int lon = 0; lon < longSegments; ++lon) {
			unsigned int a = (unsigned int)(lat * row + lon);
			unsigned int b = (unsigned int)((lat + 1) * row + lon);
			unsigned int c = (unsigned int)((lat + 1) * row + lon + 1);
			unsigned int d = (unsigned int)(lat * row + lon + 1);
			indices.push_back(a); indices.push_back(b); indices.push_back(c);
			indices.push_back(a); indices.push_back(c); indices.push_back(d);
		}
	}

	return Mesh(verts, indices);
}

// Cheap deterministic hash → [0, 1).
float hash01(uint32_t x, uint32_t seed) {
	uint32_t h = x * 374761393u + seed * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h = h ^ (h >> 16);
	return (float)(h & 0xFFFFFF) / 16777216.0f;
}

// Bright candy palette — should pop against the gloomy sky.
const glm::vec3 PALETTE[6] = {
	{1.00f, 0.20f, 0.20f},  // red
	{1.00f, 0.55f, 0.10f},  // orange
	{1.00f, 1.00f, 0.20f},  // yellow
	{0.20f, 0.95f, 0.30f},  // green
	{0.20f, 0.55f, 1.00f},  // blue
	{0.85f, 0.30f, 0.95f},  // purple
};

}  // namespace

BalloonSystem::BalloonSystem()
	: sphereMesh(makeSphereMesh(1.0f, 12, 18)),
	  shader("shaders/balloon.vert", "shaders/balloon.frag") {}

void BalloonSystem::seed(int count, float spread, float minH, float maxH) {
	balloons.clear();
	balloons.reserve(count);
	for (int i = 0; i < count; ++i) {
		Balloon b;
		float ax = hash01(i * 7u + 1u, 31u);
		float az = hash01(i * 11u + 3u, 47u);
		float ah = hash01(i * 13u + 5u, 59u);
		float ac = hash01(i * 17u + 7u, 71u);

		float x = (ax * 2.0f - 1.0f) * spread;
		float z = (az * 2.0f - 1.0f) * spread;
		float groundY = Terrain::heightAt(x, z);
		float h = minH + ah * (maxH - minH);
		b.position = glm::vec3(x, groundY + h, z);
		b.radius = 4.0f;
		b.color = PALETTE[(int)(ac * 6.0f) % 6];
		b.alive = true;
		balloons.push_back(b);
	}
}

int BalloonSystem::popOverlap(const glm::vec3& point, float pointRadius) {
	for (auto& b : balloons) {
		if (!b.alive) continue;
		float r = b.radius + pointRadius;
		glm::vec3 d = b.position - point;
		if (glm::dot(d, d) <= r * r) {
			b.alive = false;
			return 100;  // flat 100 per pop for now
		}
	}
	return 0;
}

void BalloonSystem::draw(const glm::mat4& view, const glm::mat4& projection,
                         float time, const glm::vec3& cameraPos) const {
	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setVec3("cameraPos", cameraPos);

	const float maxDistSq = 600.0f * 600.0f;

	for (const auto& b : balloons) {
		if (!b.alive) continue;
		glm::vec3 d = b.position - cameraPos;
		if (glm::dot(d, d) > maxDistSq) continue;

		// Subtle bobbing so balloons read as floating, not pasted into the sky.
		float bob = std::sin(time * 1.4f + b.position.x * 0.05f) * 0.6f;
		glm::mat4 model = glm::translate(glm::mat4(1.0f),
			b.position + glm::vec3(0.0f, bob, 0.0f));
		model = glm::scale(model, glm::vec3(b.radius));
		shader.setMat4("model", model);
		shader.setVec3("balloonColor", b.color);
		sphereMesh.draw();
	}
}

void BalloonSystem::reset() {
	for (auto& b : balloons) b.alive = true;
}

int BalloonSystem::aliveCount() const {
	int n = 0;
	for (const auto& b : balloons) if (b.alive) ++n;
	return n;
}
