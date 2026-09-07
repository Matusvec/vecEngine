#include "world/terrain.h"

#include "renderer/frustum.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {

// Hash on integer coordinates → [0, 1). Stable and cheap.
inline float hashCorner(int x, int y) {
	uint32_t h = ((uint32_t)x + 12345u) * 374761393u + ((uint32_t)y + 67890u) * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h = h ^ (h >> 16);
	return (float)(h & 0xFFFFFF) / 16777216.0f;
}

// Smooth-step interpolation to avoid grid artifacts.
inline float fade(float t) { return t * t * (3.0f - 2.0f * t); }

// 2D value noise → [-1, 1].
inline float valueNoise(float x, float y) {
	float fx = std::floor(x);
	float fy = std::floor(y);
	int ix = (int)fx;
	int iy = (int)fy;
	float tx = fade(x - fx);
	float ty = fade(y - fy);

	float v00 = hashCorner(ix,     iy);
	float v10 = hashCorner(ix + 1, iy);
	float v01 = hashCorner(ix,     iy + 1);
	float v11 = hashCorner(ix + 1, iy + 1);

	float a = v00 + (v10 - v00) * tx;
	float b = v01 + (v11 - v01) * tx;
	return ((a + (b - a) * ty) * 2.0f) - 1.0f;
}

// Fractional Brownian motion — stacked octaves of value noise.
inline float fbm(float x, float y, int octaves) {
	float v = 0.0f;
	float amp = 0.5f;
	float freq = 1.0f;
	for (int i = 0; i < octaves; ++i) {
		v += valueNoise(x * freq, y * freq) * amp;
		freq *= 2.0f;
		amp *= 0.5f;
	}
	return v;
}

// Static list of explosion craters that subtract from heightAt globally.
// Lives in this TU so heightAt() (a static method) can read it.
std::vector<Terrain::Crater> g_craters;

inline float craterDepression(float x, float z) {
	float total = 0.0f;
	for (const auto& c : g_craters) {
		float dx = x - c.center.x;
		float dz = z - c.center.z;
		float d2 = dx * dx + dz * dz;
		float r2 = c.radius * c.radius;
		if (d2 >= r2) continue;
		float d = std::sqrt(d2);
		float t = 1.0f - d / c.radius;       // [0, 1] from edge to center
		t = t * t * (3.0f - 2.0f * t);       // smoothstep falloff
		total += c.depth * t;
	}
	return total;
}

}  // namespace

// Noise-based alpine heightmap. No sin/cos — uses value-noise FBM with domain
// warping for irregular mountain placement and ridged-multifractal peaks.
// Output range roughly [-60, +330].
float Terrain::heightAt(float x, float z) {
	// Continent-scale undulation — broad valleys and uplands.
	float continent = fbm(x * 0.0014f, z * 0.0014f, 3) * 75.0f;

	// Mountain mask — separate seed offset so peaks cluster in irregular regions
	// rather than sitting on the continent shape's hills.
	float maskRaw = fbm(x * 0.0020f + 1000.0f, z * 0.0020f + 1000.0f, 2);
	float mask = std::max(0.0f, maskRaw + 0.05f) * 1.5f;
	mask = std::min(1.0f, mask);
	mask *= mask;

	// Domain warping — perturb the ridge sample coords with another low-freq noise
	// so ridge lines snake instead of running parallel.
	float warpX = fbm(x * 0.004f + 50.0f, z * 0.004f + 50.0f, 2) * 60.0f;
	float warpZ = fbm(x * 0.004f + 200.0f, z * 0.004f + 200.0f, 2) * 60.0f;

	// Ridged noise (1 - |fbm|) creates sharp ridges; 4 octaves now for finer ridge detail.
	float ridgeNoise = fbm((x + warpX) * 0.005f + 500.0f,
	                       (z + warpZ) * 0.005f + 500.0f, 4);
	float ridge = 1.0f - std::abs(ridgeNoise);
	ridge *= ridge;
	float mountains = ridge * 220.0f * mask;

	// Secondary fine ridges only on mountain surfaces — adds visible texture
	// (outcroppings, smaller scarps) without changing the macro mountain shape.
	float fineRidgeN = std::abs(valueNoise(x * 0.020f + 99.0f, z * 0.020f + 99.0f));
	float fineRidge = 1.0f - fineRidgeN;
	fineRidge *= fineRidge;
	float mountainDetail = fineRidge * 28.0f * mask;

	// Mid-frequency hills everywhere (rolling secondary terrain).
	float hills = fbm(x * 0.012f + 333.0f, z * 0.012f + 777.0f, 2) * 18.0f;

	// Fine detail to break up smooth surfaces.
	float detail = valueNoise(x * 0.04f, z * 0.04f) * 5.0f;

	float baseHeight = continent + mountains + mountainDetail + hills + detail + 25.0f;
	return baseHeight - craterDepression(x, z);
}

void Terrain::addCrater(const glm::vec3& center, float radius, float depth) {
	g_craters.push_back({center, radius, depth});

	// Force-regen any chunk whose bounds overlap the crater. Conservative reach:
	// crater radius + chunk diagonal so we catch edge cases.
	float reach = radius + chunkSize * 1.5f;
	float reachSq = reach * reach;
	for (Chunk& chunk : chunks) {
		if (chunk.worldChunkX == INT_MIN) continue;
		float dx = chunk.boundsCenter.x - center.x;
		float dz = chunk.boundsCenter.z - center.z;
		if (dx * dx + dz * dz < reachSq) {
			regenerateChunk(chunk, chunk.worldChunkX, chunk.worldChunkZ);
		}
	}
}

void Terrain::clearCraters() {
	g_craters.clear();
	// Rebuild every active chunk so existing craters disappear from the geometry.
	for (Chunk& chunk : chunks) {
		if (chunk.worldChunkX == INT_MIN) continue;
		regenerateChunk(chunk, chunk.worldChunkX, chunk.worldChunkZ);
	}
}

Terrain::Terrain(int chunkRadius, int chunkResolution, float chunkSize)
	: chunkRadius(chunkRadius), chunkResolution(chunkResolution), chunkSize(chunkSize) {
	int side = 2 * chunkRadius + 1;
	chunks.resize((size_t)side * (size_t)side);

	// Generate the initial ring around the world origin so first frame has terrain
	// even if the caller forgets to call update() before draw().
	update(glm::vec3(0.0f));
}

void Terrain::regenerateChunk(Chunk& chunk, int worldChunkX, int worldChunkZ) {
	const int vertsPerSide = chunkResolution + 1;
	const float spacing = chunkSize / (float)chunkResolution;
	const float normalSampleOffset = spacing * 0.5f;
	const float tileSize = 4.0f;

	const float chunkOriginX = (float)worldChunkX * chunkSize;
	const float chunkOriginZ = (float)worldChunkZ * chunkSize;

	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	verts.reserve(vertsPerSide * vertsPerSide);
	indices.reserve(6 * chunkResolution * chunkResolution);

	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();

	for (int z = 0; z < vertsPerSide; ++z) {
		for (int x = 0; x < vertsPerSide; ++x) {
			float worldX = chunkOriginX + (float)x * spacing;
			float worldZ = chunkOriginZ + (float)z * spacing;
			float worldY = heightAt(worldX, worldZ);

			float hL = heightAt(worldX - normalSampleOffset, worldZ);
			float hR = heightAt(worldX + normalSampleOffset, worldZ);
			float hD = heightAt(worldX, worldZ - normalSampleOffset);
			float hU = heightAt(worldX, worldZ + normalSampleOffset);
			glm::vec3 normal = glm::normalize(glm::vec3(
				hL - hR,
				2.0f * normalSampleOffset,
				hD - hU));

			glm::vec2 uv(worldX / tileSize, worldZ / tileSize);
			verts.push_back({{worldX, worldY, worldZ}, normal, uv});

			minY = std::min(minY, worldY);
			maxY = std::max(maxY, worldY);
		}
	}

	for (int z = 0; z < chunkResolution; ++z) {
		for (int x = 0; x < chunkResolution; ++x) {
			unsigned int tl = (unsigned int)(z * vertsPerSide + x);
			unsigned int tr = tl + 1;
			unsigned int bl = tl + (unsigned int)vertsPerSide;
			unsigned int br = bl + 1;

			indices.push_back(tl);
			indices.push_back(bl);
			indices.push_back(br);

			indices.push_back(br);
			indices.push_back(tr);
			indices.push_back(tl);
		}
	}

	float halfSize = chunkSize * 0.5f;
	float halfHeight = (maxY - minY) * 0.5f;
	chunk.boundsCenter = glm::vec3(
		chunkOriginX + halfSize,
		(minY + maxY) * 0.5f,
		chunkOriginZ + halfSize);
	chunk.boundsRadius = std::sqrt(2.0f * halfSize * halfSize + halfHeight * halfHeight);

	chunk.mesh = Mesh(verts, indices);  // move-assigns; old GL state freed automatically
	chunk.worldChunkX = worldChunkX;
	chunk.worldChunkZ = worldChunkZ;
}

void Terrain::update(const glm::vec3& cameraPos) {
	int desiredCenterX = (int)std::floor(cameraPos.x / chunkSize);
	int desiredCenterZ = (int)std::floor(cameraPos.z / chunkSize);

	if (desiredCenterX == currentCenterX && desiredCenterZ == currentCenterZ) return;

	currentCenterX = desiredCenterX;
	currentCenterZ = desiredCenterZ;

	const int side = 2 * chunkRadius + 1;
	for (int wz = currentCenterZ - chunkRadius; wz <= currentCenterZ + chunkRadius; ++wz) {
		for (int wx = currentCenterX - chunkRadius; wx <= currentCenterX + chunkRadius; ++wx) {
			// Modular slot: when the player moves +1 chunk, only the trailing edge
			// chunks have a stale (slot ≠ desired) coord and get regenerated.
			int slotX = ((wx % side) + side) % side;
			int slotZ = ((wz % side) + side) % side;
			Chunk& chunk = chunks[(size_t)slotZ * (size_t)side + (size_t)slotX];
			if (chunk.worldChunkX != wx || chunk.worldChunkZ != wz) {
				regenerateChunk(chunk, wx, wz);
			}
		}
	}
}

int Terrain::draw(const Shader& shader, const Frustum& frustum, const glm::vec3& cameraPos) const {
	shader.setMat4("model", glm::mat4(1.0f));

	// Anything past fog-end (~1500) is fully fogged. 1600² as squared cutoff —
	// cheaper than the frustum check, so do it first to short-circuit.
	const float maxDistSq = 1600.0f * 1600.0f;

	int drawn = 0;
	for (const Chunk& chunk : chunks) {
		if (chunk.worldChunkX == INT_MIN) continue;  // never generated; defensive

		glm::vec3 toChunk = chunk.boundsCenter - cameraPos;
		if (glm::dot(toChunk, toChunk) > maxDistSq) continue;

		if (frustum.containsSphere(chunk.boundsCenter, chunk.boundsRadius)) {
			chunk.mesh.draw();
			++drawn;
		}
	}
	return drawn;
}

void Terrain::drawDepth(const Shader& depthShader, const glm::vec3& cameraPos) const {
	depthShader.setMat4("model", glm::mat4(1.0f));

	// Cover a bit more than the shadow ortho extent (320) so chunks straddling
	// the boundary still cast — distance is XZ only since the light frustum is
	// vertically thick enough to swallow the height range.
	const float shadowReachSq = 380.0f * 380.0f;

	for (const Chunk& chunk : chunks) {
		if (chunk.worldChunkX == INT_MIN) continue;
		float dx = chunk.boundsCenter.x - cameraPos.x;
		float dz = chunk.boundsCenter.z - cameraPos.z;
		if (dx * dx + dz * dz > shadowReachSq) continue;
		chunk.mesh.draw();
	}
}
