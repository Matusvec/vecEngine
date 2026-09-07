#include "renderer/camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

Camera::Camera(const glm::vec3& position, const glm::vec3& worldUp, float yaw, float pitch)
	: position(position), yaw(yaw), pitch(pitch), worldUp(worldUp) {
	updateVectors();
}

glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
	return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::updateVectors() {
	float yawRad = glm::radians(yaw);
	float pitchRad = glm::radians(pitch);

	glm::vec3 newFront;
	newFront.x = std::cos(yawRad) * std::cos(pitchRad);
	newFront.y = std::sin(pitchRad);
	newFront.z = std::sin(yawRad) * std::cos(pitchRad);

	front = glm::normalize(newFront);
	right = glm::normalize(glm::cross(front, worldUp));
	up    = glm::normalize(glm::cross(right, front));

	// Roll: rotate up + right around the front axis. Banking tilts the horizon
	// without changing where the plane is pointing.
	if (roll != 0.0f) {
		float r = glm::radians(roll);
		float cr = std::cos(r);
		float sr = std::sin(r);
		glm::vec3 newUp    = up * cr + right * sr;
		glm::vec3 newRight = right * cr - up * sr;
		up = glm::normalize(newUp);
		right = glm::normalize(newRight);
	}
}
