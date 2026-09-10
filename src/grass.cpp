#include "world/grass.h"

#include "renderer/light.h"
#include "renderer/vertex.h"
#include "world/terrain.h"

#include <glad/glad.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

Grass::Grass(float seaLevel, float radius, float cellSize)
	: shader("shaders/grass.vert", "shaders/grass.frag"),
	  seaLevel(seaLevel), radius(radius), cellSize(cellSize) {

	// Triangle blade: 3 verts instead of 4. Saves 25% vertex shader work and removes
	// the alpha-discard in the fragment shader (the triangle IS the silhouette).
	const Vertex blade[3] = {
		{ {-0.15f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },  // base-left
		{ { 0.15f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },  // base-right
		{ { 0.00f, 1.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f} },  // tip
	};
	const unsigned int quadIndices[3] = {0, 1, 2};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &instanceVBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(blade), blade, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	// Shared per-vertex attributes (positions 0/1/2) — same layout as Mesh uses.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
	glEnableVertexAttribArray(2);

	// Per-instance attributes (locations 3 + 4) packed into the same Blade struct.
	// Divisor 1 = advance once per instance, not per vertex.
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Blade), (void*)offsetof(Blade, position));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Blade), (void*)offsetof(Blade, randoms));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);

	glBindVertexArray(0);
}

Grass::~Grass() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &instanceVBO);
}

void Grass::regenerate(const glm::vec3& cameraPos) {
	std::vector<Blade> instances;

	auto hashFloat = [](int x, int y, int seed) -> float {
		uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2147483647u;
		h = (h ^ (h >> 13)) * 1274126177u;
		h = h ^ (h >> 16);
		return (float)(h % 10000) / 10000.0f;
	};

	// Smooth value-noise patch mask in world-XZ. Wavelength ~50 units so
	// grassy areas form recognizable meadows rather than per-blade speckle.
	auto patchMask = [&hashFloat](float wx, float wz) -> float {
		float fx = wx * 0.020f;
		float fz = wz * 0.020f;
		int ix = (int)std::floor(fx);
		int iz = (int)std::floor(fz);
		float tx = fx - (float)ix;
		float tz = fz - (float)iz;
		tx = tx * tx * (3.0f - 2.0f * tx);
		tz = tz * tz * (3.0f - 2.0f * tz);
		float a = hashFloat(ix,     iz,     42);
		float b = hashFloat(ix + 1, iz,     42);
		float c = hashFloat(ix,     iz + 1, 42);
		float d = hashFloat(ix + 1, iz + 1, 42);
		float ab = a + (b - a) * tx;
		float cd = c + (d - c) * tx;
		return ab + (cd - ab) * tz;
	};

	// Altitude window — must match the GRASS biome band in basic.frag so
	// blades only appear on textured-grass terrain (not sand, forest, rock, snow).
	constexpr float MIN_GRASS_Y = 0.0f;    // above the beach
	constexpr float MAX_GRASS_Y = 95.0f;   // below the forest line

	// Patch threshold: below this, the cell is "bare ground" (no blade).
	// Higher value = sparser meadows. ~0.50 gives roughly half-coverage clumps.
	constexpr float PATCH_THRESHOLD = 0.50f;

	int cellsRadius = (int)(radius / cellSize);
	int centerCellX = (int)std::floor(cameraPos.x / cellSize);
	int centerCellZ = (int)std::floor(cameraPos.z / cellSize);

	float radiusSq = radius * radius;
	instances.reserve((size_t)(cellsRadius * cellsRadius));

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
			if (y < MIN_GRASS_Y || y > MAX_GRASS_Y) continue;
			if (y < seaLevel + 0.5f) continue;  // belt-and-braces vs the water plane

			// Patchy distribution so meadows have visible edges — bald ground in
			// between rather than uniform grass everywhere.
			if (patchMask(x, z) < PATCH_THRESHOLD) continue;
			bool onPavement = false;
			for (const glm::vec4& r : paved)
				if (x >= r.x && z >= r.y && x <= r.z && z <= r.w) { onPavement = true; break; }
			if (onPavement) continue;

			Blade b;
			b.position = glm::vec3(x, y, z);
			// Pre-bake what used to be GPU bladeHash() calls — done once per blade ever.
			b.randoms.x = hashFloat(cx, cz, 1) * 6.2831853f;     // rotation [0, 2π)
			b.randoms.y = 0.8f + hashFloat(cx, cz, 2) * 0.5f;     // heightScale [0.8, 1.3)
			b.randoms.z = 0.85f + hashFloat(cx, cz, 3) * 0.30f;   // tintMultiplier [0.85, 1.15)
			instances.push_back(b);
		}
	}

	// One upload per regen — no per-frame upload, no per-frame CPU cull.
	// GL_STATIC_DRAW signals "set rarely, draw many times" so the driver picks
	// the best memory placement for read-many access.
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(Blade),
	             instances.data(), GL_STATIC_DRAW);
	instanceCountCached = (int)instances.size();
}

void Grass::update(const glm::vec3& cameraPos, double time) {
	int currentCellX = (int)std::floor(cameraPos.x / cellSize);
	int currentCellZ = (int)std::floor(cameraPos.z / cellSize);
	if (currentCellX == lastCellX && currentCellZ == lastCellZ) return;

	bool firstRegen = (lastRegenTime < 0.0);

	// Rate-limit: throttle both by elapsed time AND distance moved. Skip regen if
	// less than 0.3s has passed OR the camera has moved less than 5 units since
	// the last regen — except on the very first call where we always generate.
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

void Grass::draw(const glm::mat4& view, const glm::mat4& projection, float time,
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
	glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0, (GLsizei)instanceCountCached);
}
