#include "renderer/overlay.h"

#include <glad/glad.h>

Overlay::Overlay() : shader("shaders/overlay.vert", "shaders/overlay.frag") {
	// Two triangles covering NDC [-1, 1] — vertex shader outputs them directly
	// without any matrices.
	const float verts[12] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f,  1.0f,
		-1.0f,  1.0f,
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

Overlay::~Overlay() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Overlay::drawTinted(const glm::vec3& color, float alpha) const {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader.use();
	shader.setVec3("color", color);
	shader.setFloat("alpha", alpha);

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}
