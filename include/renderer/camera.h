#pragma once

#include <glm/glm.hpp>

class Camera {
public:
	Camera(const glm::vec3& position = {0.0f, 0.0f, 3.0f},
	       const glm::vec3& worldUp = {0.0f, 1.0f, 0.0f},
	       float yaw = -90.0f,
	       float pitch = 0.0f);

	glm::mat4 getViewMatrix() const;
	glm::mat4 getProjectionMatrix(float aspect) const;

	// Recompute front/right/up from yaw + pitch. Call after modifying yaw or pitch directly.
	void updateVectors();

	glm::vec3 position;
	float yaw;          // degrees, around world-up axis
	float pitch;        // degrees, around right axis
	float roll = 0.0f;  // degrees, around the camera's forward axis (banking)
	float fov = 45.0f;  // degrees
	float nearPlane = 0.1f;
	float farPlane = 700.0f;

	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;
};
