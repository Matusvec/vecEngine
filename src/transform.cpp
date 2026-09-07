#include "world/transform.h"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform::getModelMatrix() const {
	glm::mat4 m(1.0f);
	m = glm::translate(m, position);
	m = glm::rotate(m, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	m = glm::rotate(m, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	m = glm::rotate(m, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
	m = glm::scale(m, scale);
	return m;
}
