#include "game/ring.h"

#include "world/terrain.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

// Procedural torus. The ring lies in the XY plane → its hole axis points along
// +Z. We orient it in the world via a model matrix that maps +Z onto the
// ring's forward direction.
Mesh makeTorusMesh(float majorRadius, float minorRadius,
                   int majorSegments, int minorSegments) {
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	const float PI = 3.14159265358979f;

	for (int i = 0; i <= majorSegments; ++i) {
		float u = (float)i / majorSegments * 2.0f * PI;
		float cu = std::cos(u), su = std::sin(u);
		for (int j = 0; j <= minorSegments; ++j) {
			float v = (float)j / minorSegments * 2.0f * PI;
			float cv = std::cos(v), sv = std::sin(v);
			float x = (majorRadius + minorRadius * cv) * cu;
			float y = (majorRadius + minorRadius * cv) * su;
			float z = minorRadius * sv;
			glm::vec3 nrm = glm::normalize(glm::vec3(cu * cv, su * cv, sv));
			verts.push_back({{x, y, z}, nrm,
			                 {(float)i / majorSegments, (float)j / minorSegments}});
		}
	}

	int row = minorSegments + 1;
	for (int i = 0; i < majorSegments; ++i) {
		for (int j = 0; j < minorSegments; ++j) {
			unsigned int a = (unsigned int)(i * row + j);
			unsigned int b = (unsigned int)((i + 1) * row + j);
			unsigned int c = (unsigned int)((i + 1) * row + j + 1);
			unsigned int d = (unsigned int)(i * row + j + 1);
			indices.push_back(a); indices.push_back(b); indices.push_back(c);
			indices.push_back(a); indices.push_back(c); indices.push_back(d);
		}
	}

	return Mesh(verts, indices);
}

float hash01(uint32_t x, uint32_t seed) {
	uint32_t h = x * 374761393u + seed * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h = h ^ (h >> 16);
	return (float)(h & 0xFFFFFF) / 16777216.0f;
}

// Build a model matrix that places a torus (whose hole axis is +Z by default)
// at `position` with its hole facing `forward`.
glm::mat4 ringModelMatrix(const glm::vec3& position, const glm::vec3& forward) {
	// Construct an orthonormal basis where +Z = forward.
	glm::vec3 fwd = glm::normalize(forward);
	glm::vec3 ref = (std::abs(fwd.y) < 0.95f)
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 right = glm::normalize(glm::cross(ref, fwd));
	glm::vec3 up = glm::cross(fwd, right);

	glm::mat4 m(1.0f);
	m[0] = glm::vec4(right, 0.0f);
	m[1] = glm::vec4(up,    0.0f);
	m[2] = glm::vec4(fwd,   0.0f);
	m[3] = glm::vec4(position, 1.0f);
	return m;
}

}  // namespace

RingSystem::RingSystem()
	: torusMesh(makeTorusMesh(12.0f, 0.8f, 32, 8)),
	  shader("shaders/ring.vert", "shaders/ring.frag") {}

void RingSystem::seed(int count, float courseRadius, float minHeight, float maxHeight) {
	rings.clear();
	rings.reserve(count);
	const float PI = 3.14159265358979f;

	// Lay rings in a loose circle around the world origin so the player has
	// a natural "course" to fly around.
	for (int i = 0; i < count; ++i) {
		float baseAngle = (float)i / count * 2.0f * PI;
		float angleJitter = (hash01(i * 13u, 11u) - 0.5f) * 0.6f;
		float angle = baseAngle + angleJitter;
		float r = courseRadius * (0.7f + hash01(i * 17u, 23u) * 0.6f);

		float x = std::cos(angle) * r;
		float z = std::sin(angle) * r;
		float ground = Terrain::heightAt(x, z);
		float h = minHeight + hash01(i * 19u, 41u) * (maxHeight - minHeight);
		float y = ground + h;

		// Forward = tangent to the circle (pointing along the course direction).
		glm::vec3 forward = glm::normalize(glm::vec3(-std::sin(angle), 0.0f, std::cos(angle)));

		Ring ring;
		ring.position = glm::vec3(x, y, z);
		ring.forward = forward;
		ring.radius = 12.0f;
		ring.passed = false;
		rings.push_back(ring);
	}
	active = 0;
	hasLastPos = false;
}

void RingSystem::reset() {
	for (auto& r : rings) r.passed = false;
	active = 0;
	hasLastPos = false;
}

void RingSystem::prependRing(const glm::vec3& position, const glm::vec3& forward, float radius) {
	Ring r;
	r.position = position;
	r.forward = glm::normalize(forward);
	r.radius = radius;
	r.passed = false;
	rings.insert(rings.begin(), r);
	active = 0;
	hasLastPos = false;
}

int RingSystem::update(const glm::vec3& playerPos) {
	if (active >= (int)rings.size()) {
		lastPlayerPos = playerPos;
		hasLastPos = true;
		return 0;
	}

	Ring& ring = rings[active];

	// Very generous detection: 2.5× the visual radius. With ring radius 12 that's
	// a 30-unit detection sphere — almost impossible to miss flying anywhere close.
	const float detectR = ring.radius * 2.5f;
	const float detectR2 = detectR * detectR;

	int score = 0;

	glm::vec3 d = playerPos - ring.position;
	float distSq = glm::dot(d, d);
	bool insideNow = distSq <= detectR2;

	// Segment check catches very fast frames where the plane jumps past the
	// sphere in a single tick.
	bool segmentHit = false;
	if (!insideNow && hasLastPos) {
		glm::vec3 segDir = playerPos - lastPlayerPos;
		float segLen2 = glm::dot(segDir, segDir);
		if (segLen2 > 1e-6f) {
			glm::vec3 toRing = ring.position - lastPlayerPos;
			float t = glm::clamp(glm::dot(toRing, segDir) / segLen2, 0.0f, 1.0f);
			glm::vec3 closest = lastPlayerPos + segDir * t;
			glm::vec3 cd = closest - ring.position;
			segmentHit = glm::dot(cd, cd) <= detectR2;
		}
	}

	// Diagnostic — every ~30 frames, log distance to the active ring so the
	// user can see whether they're actually getting close.
	static int diagFrame = 0;
	if ((++diagFrame % 30) == 0) {
		std::cerr << "[ring] active=" << active
		          << " ring@(" << (int)ring.position.x << ","
		          << (int)ring.position.y << ","
		          << (int)ring.position.z << ")"
		          << " player@(" << (int)playerPos.x << ","
		          << (int)playerPos.y << ","
		          << (int)playerPos.z << ")"
		          << " dist=" << (int)std::sqrt(distSq)
		          << " (need <" << (int)detectR << ")\n";
	}

	if (insideNow || segmentHit) {
		ring.passed = true;
		++active;
		score = 200;
		std::cerr << "*** RING " << active << "/" << rings.size()
		          << " PASSED — +200 ***\n";
	}

	hasLastPos = true;
	lastPlayerPos = playerPos;
	return score;
}

void RingSystem::draw(const glm::mat4& view, const glm::mat4& projection,
                      float time, const glm::vec3& cameraPos) const {
	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setVec3("cameraPos", cameraPos);
	shader.setFloat("time", time);

	const float maxDistSq = 800.0f * 800.0f;

	for (int i = 0; i < (int)rings.size(); ++i) {
		const Ring& ring = rings[i];
		glm::vec3 d = ring.position - cameraPos;
		if (glm::dot(d, d) > maxDistSq) continue;

		// 0 = passed (dim grey), 1 = active (bright cyan), 2 = upcoming (faded gold)
		int state = ring.passed ? 0 : (i == active ? 1 : 2);
		shader.setInt("ringState", state);

		shader.setMat4("model", ringModelMatrix(ring.position, ring.forward));
		torusMesh.draw();
	}
}
