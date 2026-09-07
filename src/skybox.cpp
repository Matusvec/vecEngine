#include "renderer/skybox.h"

#include <glad/glad.h>

Skybox::Skybox()
	: cube(Mesh::createCube()),
	  shader("shaders/skybox.vert", "shaders/skybox.frag") {}

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection,
                  float time, const glm::vec3& sunDir) const {
	glDepthMask(GL_FALSE);

	shader.use();
	// mat3(view) drops the translation column → camera always sits at the origin
	// of the skybox cube, so the sky never appears to move as the player flies.
	shader.setMat4("view", glm::mat4(glm::mat3(view)));
	shader.setMat4("projection", projection);
	shader.setFloat("time", time);
	shader.setVec3("sunDir", sunDir);
	cube.draw();

	glDepthMask(GL_TRUE);
}
