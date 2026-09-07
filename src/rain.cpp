#include "world/rain.h"

#include "renderer/vertex.h"

#include <glad/glad.h>

#include <cstddef>
#include <cstdint>
#include <vector>

Rain::Rain(int dropCount, float boxRadius)
	: shader("shaders/rain.vert", "shaders/rain.frag"), dropCount(dropCount) {

	// One streak: thin vertical quad, 0.04 wide × 0.6 tall.
	const Vertex streak[4] = {
		{ {-0.02f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
		{ { 0.02f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
		{ { 0.02f, 0.6f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
		{ {-0.02f, 0.6f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
	};
	const unsigned int idx[6] = {0, 1, 2, 2, 3, 0};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &instanceVBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(streak), streak, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
	glEnableVertexAttribArray(2);

	// Per-instance data: (x_offset_within_box, initial_phase_[0,1], z_offset_within_box).
	// Generated once at construction — drops cycle entirely in the vertex shader.
	std::vector<glm::vec3> drops((size_t)dropCount);
	{
		auto hash01 = [](uint32_t x) -> float {
			x = (x ^ (x >> 13)) * 1274126177u;
			x = x ^ (x >> 16);
			return (float)(x & 0xFFFFFF) / 16777216.0f;
		};
		for (int i = 0; i < dropCount; ++i) {
			float r1 = hash01((uint32_t)i * 374761393u);
			float r2 = hash01((uint32_t)(i + 1) * 668265263u);
			float r3 = hash01((uint32_t)(i + 2) * 1597334677u);
			drops[i] = glm::vec3(
				(r1 * 2.0f - 1.0f) * boxRadius,
				r2,
				(r3 * 2.0f - 1.0f) * boxRadius
			);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, drops.size() * sizeof(glm::vec3),
	             drops.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	glBindVertexArray(0);
}

Rain::~Rain() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &instanceVBO);
}

void Rain::draw(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, float time) const {
	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setVec3("cameraPos", cameraPos);
	shader.setFloat("time", time);

	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (GLsizei)dropCount);
}
