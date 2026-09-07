#include "game/flight_controller.h"

#include "core/input.h"
#include "renderer/camera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

FlightController::FlightController(Camera& camera, const glm::vec3& startPos)
	: position(startPos), camera(camera) {
	syncCamera();
	// Seed velocity so the plane is already moving forward when the game starts.
	velocity = forward() * (minSpeed + (maxSpeed - minSpeed) * throttle);
}

void FlightController::reset(const glm::vec3& spawnPos) {
	position = spawnPos;
	yaw = -90.0f;
	pitch = 0.0f;
	roll = 0.0f;
	throttle = 0.20f;
	pitchInput = 0.0f;
	rollInput = 0.0f;
	aileronCmdDeg = 0.0f;
	aileronMeasDeg = 0.0f;
	syncCamera();
	velocity = forward() * (minSpeed + (maxSpeed - minSpeed) * throttle);
}

void FlightController::syncCamera() {
	if (thirdPerson) {
		// Chase camera: 14 units behind the plane, 4 above. Don't roll the
		// camera — keeping the horizon level while watching the plane bank
		// reads better than spinning the world.
		glm::vec3 fwd = forward();
		camera.position = position - fwd * 14.0f + glm::vec3(0.0f, 4.0f, 0.0f);
		camera.yaw = yaw;
		camera.pitch = pitch;
		camera.roll = 0.0f;
	} else {
		// Cockpit view: camera sits exactly at the plane, full orientation.
		camera.position = position;
		camera.yaw = yaw;
		camera.pitch = pitch;
		camera.roll = roll;
	}
	camera.updateVectors();
}

float FlightController::speed() const {
	return glm::length(velocity);
}

glm::vec3 FlightController::forward() const {
	float yawRad = glm::radians(yaw);
	float pitchRad = glm::radians(pitch);
	return glm::normalize(glm::vec3(
		std::cos(yawRad) * std::cos(pitchRad),
		std::sin(pitchRad),
		std::sin(yawRad) * std::cos(pitchRad)));
}

void FlightController::update(float dt, const Input& input) {
	if (frozen) return;

	// Throttle — W/S, with optional shift for emergency burn (caps at 1).
	if (input.isKeyPressed(GLFW_KEY_W)) throttle += dt * 0.55f;
	if (input.isKeyPressed(GLFW_KEY_S)) throttle -= dt * 0.55f;
	if (input.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) throttle += dt * 1.2f;
	throttle = std::clamp(throttle, 0.0f, 1.0f);

	// Number-row presets snap throttle (1 = idle, 9 = max).
	for (int i = 0; i < 9; ++i) {
		if (input.isKeyPressed(GLFW_KEY_1 + i)) {
			throttle = (float)i / 8.0f;
		}
	}

	// Pitch: real-plane / yoke convention — push UP on the stick (UP arrow) =
	// nose down = dive; pull DOWN on the stick = nose up = climb.
	float pitchIn = 0.0f;
	if (input.isKeyPressed(GLFW_KEY_UP))   pitchIn -= 1.0f;
	if (input.isKeyPressed(GLFW_KEY_DOWN)) pitchIn += 1.0f;
	pitchInput = pitchIn;  // expose for plane-model animation
	pitch += pitchIn * pitchRate * dt;
	pitch = std::clamp(pitch, -maxPitch, maxPitch);

	// Roll: bank into a turn. Auto-level when no input so the plane doesn't
	// stay rolled forever.
	float rollIn = 0.0f;
	if (input.isKeyPressed(GLFW_KEY_LEFT))  rollIn -= 1.0f;
	if (input.isKeyPressed(GLFW_KEY_RIGHT)) rollIn += 1.0f;
	rollInput = rollIn;

	// BLACKBOX: actuator lag. The surface chases the command; roll follows the surface.
	aileronCmdDeg = rollIn * maxAileronDeg;
	// Exact first-order response for this frame, stable for any dt and matches the estimator's model.
	aileronMeasDeg += (aileronCmdDeg - aileronMeasDeg) * (1.0f - std::exp(-dt / std::max(aileronTau, 1e-3f)));
	float effectiveRoll = aileronMeasDeg / maxAileronDeg;
	if (std::fabs(effectiveRoll) > 0.02f) {
		roll += effectiveRoll * rollRate * dt;
	} else {
		// Smooth pull-back to wings-level.
		roll *= std::max(0.0f, 1.0f - autoLevelRate * dt);
	}
	roll = std::clamp(roll, -maxRoll, maxRoll);

	// Banking induces yaw — the harder you bank, the sharper you turn.
	// Sign chosen so LEFT key (negative roll) yaws LEFT and RIGHT yaws right.
	yaw += roll * yawFromRoll * dt;

	// Mouse: optional fine yaw/pitch trim (very subtle so the plane stays
	// the primary control).
	if (mouseTrim) {
		yaw   += input.mouseDeltaX() * 0.04f;
		pitch -= input.mouseDeltaY() * 0.04f;
	}
	pitch = std::clamp(pitch, -maxPitch, maxPitch);

	// Compute target velocity: forward * throttle-driven target speed.
	glm::vec3 fwd = forward();
	float targetSpeed = minSpeed + (maxSpeed - minSpeed) * throttle;
	glm::vec3 desiredVel = fwd * targetSpeed;

	// Exponential smoothing toward the desired velocity → real momentum.
	float k = std::min(1.0f, dt * speedSmoothing);
	velocity = glm::mix(velocity, desiredVel, k);

	// Stall: under stall speed, gravity wins and the nose drops.
	float currentSpeed = glm::length(velocity);
	if (currentSpeed < stallSpeed) {
		velocity.y -= gravity * dt;
		// Nose drops on stall too.
		pitch = std::max(-maxPitch, pitch - 30.0f * dt);
	}

	// Integrate position.
	position += velocity * dt;

	syncCamera();
}
