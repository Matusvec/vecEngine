#include "world/shrubs.h"

#include "renderer/light.h"
#include "renderer/vertex.h"
#include "world/terrain.h"

#include <glad/glad.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

// 3 crossed quads forming a bush silhouette. Backface culling is disabled
// project-wide so each quad is two-sided automatically — no duplicate faces.
void buildBushMesh(std::vector<Vertex>& verts, std::vector<unsigned int>& indices) {
	constexpr float W = 0.85f;   // half-width
	constexpr float H = 1.60f;   // top height
	constexpr float Y0 = -0.10f; // bury the base a touch so bushes look planted
	const float angles[3] = {0.0f, 1.047197f, 2.094395f};  // 0°, 60°, 120°

	for (int q = 0; q < 3; ++q) {
		float c = std::cos(angles[q]);
		float s = std::sin(angles[q]);
		glm::vec3 nrm(-s, 0.0f, c);

		// Quad corners in object space.
		glm::vec3 bl(-W * c, Y0, -W * s);
		glm::vec3 br( W * c, Y0,  W * s);
		glm::vec3 tr( W * c, H,   W * s);
		glm::vec3 tl(-W * c, H,  -W * s);

		verts.push_back({{bl.x, bl.y, bl.z}, {nrm.x, nrm.y, nrm.z}, {0.0f, 0.0f}});
		verts.push_back({{br.x, br.y, br.z}, {nrm.x, nrm.y, nrm.z}, {1.0f, 0.0f}});
		verts.push_back({{tr.x, tr.y, tr.z}, {nrm.x, nrm.y, nrm.z}, {1.0f, 1.0f}});
		verts.push_back({{tl.x, tl.y, tl.z}, {nrm.x, nrm.y, nrm.z}, {0.0f, 1.0f}});

		unsigned int base = (unsigned int)q * 4u;
		indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
		indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
	}
}

}

Shrubs::Shrubs(float minY, float maxY, float radius, float cellSize)
	: shader("shaders/shrubs.vert", "shaders/shrubs.frag"),
	  minY(minY), maxY(maxY), radius(radius), cellSize(cellSize) {

	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	buildBushMesh(verts, indices);
	indexCount = (int)indices.size();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &instanceVBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex),
	             verts.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
	             indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)offsetof(Instance, position));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)offsetof(Instance, randoms));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);

	glBindVertexArray(0);
}

Shrubs::~Shrubs() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &instanceVBO);
}

void Shrubs::regenerate(const glm::vec3& cameraPos) {
	std::vector<Instance> instances;

	auto hashFloat = [](int x, int y, int seed) -> float {
		uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2147483647u;
		h = (h ^ (h >> 13)) * 1274126177u;
		h = h ^ (h >> 16);
		return (float)(h % 10000) / 10000.0f;
	};

	// Smooth value-noise mask so bushes form thicket-like patches rather than
	// uniform speckle. Higher frequency than grass — bushes should clump
	// tighter, not spread across whole hillsides.
	auto patchMask = [&hashFloat](float wx, float wz) -> float {
		float fx = wx * 0.035f;
		float fz = wz * 0.035f;
		int ix = (int)std::floor(fx);
		int iz = (int)std::floor(fz);
		float tx = fx - (float)ix;
		float tz = fz - (float)iz;
		tx = tx * tx * (3.0f - 2.0f * tx);
		tz = tz * tz * (3.0f - 2.0f * tz);
		float a = hashFloat(ix,     iz,     71);
		float b = hashFloat(ix + 1, iz,     71);
		float c = hashFloat(ix,     iz + 1, 71);
		float d = hashFloat(ix + 1, iz + 1, 71);
		float ab = a + (b - a) * tx;
		float cd = c + (d - c) * tx;
		return ab + (cd - ab) * tz;
	};

	// Higher threshold than grass — shrubbery is supposed to be thicket-like,
	// not a carpet. ~30% coverage inside the altitude band feels right.
	constexpr float PATCH_THRESHOLD = 0.55f;

	int cellsRadius = (int)(radius / cellSize);
	int centerCellX = (int)std::floor(cameraPos.x / cellSize);
	int centerCellZ = (int)std::floor(cameraPos.z / cellSize);

	float radiusSq = radius * radius;
	instances.reserve((size_t)(cellsRadius * cellsRadius / 2));

	for (int dz = -cellsRadius; dz <= cellsRadius; ++dz) {
		for (int dx = -cellsRadius; dx <= cellsRadius; ++dx) {
			int cx = centerCellX + dx;
			int cz = centerCellZ + dz;

			float jx = hashFloat(cx, cz, 0);
			float jz = hashFloat(cz + 9173, cx + 1429, 0);

			float x = ((float)cx + jx) * cellSize;
			float z = ((float)cz + jz) * cellSize;

			float dxw = x - cameraPos.x;
			float dzw = z - cameraPos.z;
			if (dxw * dxw + dzw * dzw > radiusSq) continue;

			float y = Terrain::heightAt(x, z);
			if (y < minY || y > maxY) continue;

			if (patchMask(x, z) < PATCH_THRESHOLD) continue;

			Instance inst;
			inst.position = glm::vec3(x, y, z);
			inst.randoms.x = hashFloat(cx, cz, 1) * 6.2831853f;       // rotation [0, 2π)
			inst.randoms.y = 0.7f + hashFloat(cx, cz, 2) * 0.7f;       // scale [0.7, 1.4)
			inst.randoms.z = 0.80f + hashFloat(cx, cz, 3) * 0.40f;     // tint [0.80, 1.20)
			instances.push_back(inst);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(Instance),
	             instances.data(), GL_STATIC_DRAW);
	instanceCountCached = (int)instances.size();
}

void Shrubs::update(const glm::vec3& cameraPos, double time) {
	int currentCellX = (int)std::floor(cameraPos.x / cellSize);
	int currentCellZ = (int)std::floor(cameraPos.z / cellSize);
	if (currentCellX == lastCellX && currentCellZ == lastCellZ) return;

	bool firstRegen = (lastRegenTime < 0.0);

	// Same throttle pattern as grass — but we use a larger cellSize so this
	// fires less often anyway.
	if (!firstRegen) {
		double dt = time - lastRegenTime;
		float dxm = cameraPos.x - lastRegenX;
		float dzm = cameraPos.z - lastRegenZ;
		float distSqMoved = dxm * dxm + dzm * dzm;
		if (dt < 0.50 || distSqMoved < 25.0f) return;
	}

	lastCellX = currentCellX;
	lastCellZ = currentCellZ;
	lastRegenTime = time;
	lastRegenX = cameraPos.x;
	lastRegenZ = cameraPos.z;
	regenerate(cameraPos);
}

void Shrubs::draw(const glm::mat4& view, const glm::mat4& projection, float time,
                  const glm::vec3& cameraPos,
                  const LightingSystem& lighting,
                  const glm::mat4& lightSpaceMatrix,
                  int shadowMapUnit) const {
	if (instanceCountCached == 0) return;

	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setFloat("time", time);
	lighting.apply(shader, cameraPos, lightSpaceMatrix, shadowMapUnit);

	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0,
	                        (GLsizei)instanceCountCached);
}
