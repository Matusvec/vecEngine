#pragma once

#include "renderer/mesh.h"

#include <glm/glm.hpp>

#include <climits>
#include <vector>

class Frustum;
class Shader;

class Terrain {
public:
	// chunkRadius      — chunks in each direction from the player; total chunks = (2r+1)²
	// chunkResolution  — grid cells per chunk side; vertex count is +1 per side
	// chunkSize        — world-space side length of one chunk
	Terrain(int chunkRadius, int chunkResolution, float chunkSize);

	// Recenters the chunk ring around the player's current chunk. Cheap when
	// the player hasn't crossed a chunk boundary; otherwise regenerates the
	// chunks whose modular slot now points to a new world position.
	void update(const glm::vec3& cameraPos);

	// Draws every chunk whose bounding sphere is in the frustum AND within
	// fog-visible distance of the camera (skips chunks beyond ~1600 units that
	// would render to invisible fog color anyway).
	int draw(const Shader& shader, const Frustum& frustum, const glm::vec3& cameraPos) const;

	// Depth-only draw for shadow pass. Skips frustum culling (the shadow
	// orthographic frustum is different from the camera's) and only checks
	// that chunks lie within the shadow extent around the camera.
	void drawDepth(const Shader& depthShader, const glm::vec3& cameraPos) const;

	int totalChunks() const { return (int)chunks.size(); }

	static float heightAt(float x, float z);

	// Carve a crater into the terrain. Center XZ is the crater center; radius is the
	// XZ falloff distance; depth is the maximum sink at the center. Affects heightAt
	// globally (so grass placement also lowers automatically) and immediately
	// regenerates any chunk within reach so the new geometry shows up.
	struct Crater {
		glm::vec3 center;
		float radius;
		float depth;
	};
	void addCrater(const glm::vec3& center, float radius, float depth);

	// Wipe every recorded crater and rebuild affected chunks. Used on restart.
	void clearCraters();

private:
	struct Chunk {
		Mesh mesh;
		glm::vec3 boundsCenter{0.0f};
		float boundsRadius = 0.0f;
		int worldChunkX = INT_MIN;  // sentinel: "this slot has no real data yet"
		int worldChunkZ = INT_MIN;
	};

	void regenerateChunk(Chunk& chunk, int worldChunkX, int worldChunkZ);

	int chunkRadius;
	int chunkResolution;
	float chunkSize;
	int currentCenterX = INT_MIN;
	int currentCenterZ = INT_MIN;
	std::vector<Chunk> chunks;
};
