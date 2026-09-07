#include "world/water.h"

#include "renderer/light.h"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace {

Mesh makePlaneMesh(float size, int subdivisions) {
	int vertsPerSide = subdivisions + 1;
	float spacing = size / (float)subdivisions;
	float start = -size * 0.5f;

	std::vector<Vertex> verts;
	verts.reserve(vertsPerSide * vertsPerSide);
	for (int z = 0; z < vertsPerSide; ++z) {
		for (int x = 0; x < vertsPerSide; ++x) {
			float wx = start + (float)x * spacing;
			float wz = start + (float)z * spacing;
			verts.push_back({
				{wx, 0.0f, wz},
				{0.0f, 1.0f, 0.0f},
				{(float)x / (float)subdivisions, (float)z / (float)subdivisions}
			});
		}
	}

	std::vector<unsigned int> indices;
	indices.reserve(6 * subdivisions * subdivisions);
	for (int z = 0; z < subdivisions; ++z) {
		for (int x = 0; x < subdivisions; ++x) {
			unsigned int tl = (unsigned int)(z * vertsPerSide + x);
			unsigned int tr = tl + 1;
			unsigned int bl = tl + (unsigned int)vertsPerSide;
			unsigned int br = bl + 1;
			indices.push_back(tl); indices.push_back(bl); indices.push_back(br);
			indices.push_back(br); indices.push_back(tr); indices.push_back(tl);
		}
	}

	return Mesh(verts, indices);
}

}

Water::Water(float seaLevel, float size, int subdivisions)
	: mesh(makePlaneMesh(size, subdivisions)),
	  shader("shaders/water.vert", "shaders/water.frag"),
	  seaLevel(seaLevel) {}

void Water::draw(const glm::mat4& view, const glm::mat4& projection,
                 const glm::vec3& cameraPos, float time,
                 const LightingSystem& lighting,
                 const glm::mat4& lightSpaceMatrix,
                 int shadowMapUnit) const {
	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setFloat("time", time);
	lighting.apply(shader, cameraPos, lightSpaceMatrix, shadowMapUnit);

	// Slide the plane to sit centered under the camera at sea level. Vertices were
	// generated at y=0; this translation puts y=seaLevel and tracks player movement.
	glm::mat4 model = glm::translate(glm::mat4(1.0f),
		glm::vec3(cameraPos.x, seaLevel, cameraPos.z));
	shader.setMat4("model", model);

	mesh.draw();
}
