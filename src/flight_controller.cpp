#include "game/flight_controller.h"

#include "renderer/camera.h"

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
	velocity = onGround() ? glm::vec3(0.0f) : forward() * (minSpeed + (maxSpeed - minSpeed) * throttle);
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

void FlightController::step(float dt, const FlightInput& in) {
	if (frozen) return;
	const bool ground = onGround();

	// Throttle — W/S, with optional shift for emergency burn (caps at 1).
	throttle += in.throttleAxis * dt * 0.55f;
	if (in.throttlePreset >= 0) throttle = (float)in.throttlePreset / 8.0f;  // number row: 1 = idle, 9 = max
	throttle = std::clamp(throttle, 0.0f, 1.0f);

	// Pitch: real-plane / yoke convention — push (UP arrow) = nose down, pull = nose up.
	pitchInput = in.pitchAxis;  // expose for plane-model animation
	pitch += in.pitchAxis * pitchRate * dt;
	if (ground) {
		// Wheels on the surface: the nose cannot dig in. Below rotate speed it settles level;
		// past it the plane rotates by itself unless the pilot is holding the stick.
		float target = (speed() >= rotateSpeed && in.pitchAxis >= 0.0f) ? rotatePitch : 0.0f;
		if (in.pitchAxis <= 0.0f || speed() < rotateSpeed) pitch += (target - pitch) * std::min(1.0f, 3.0f * dt);
		pitch = std::max(pitch, 0.0f);
	}
	pitch = std::clamp(pitch, -maxPitch, maxPitch);

	// Roll: bank into a turn. Auto-level when no input so the plane doesn't
	// stay rolled forever.
	rollInput = in.rollAxis;

	// BLACKBOX: actuator lag. The surface chases the command; roll follows the surface.
	aileronCmdDeg = in.rollAxis * maxAileronDeg;
	// Exact first-order response for this frame, stable for any dt and matches the estimator's model.
	aileronMeasDeg += (aileronCmdDeg - aileronMeasDeg) * (1.0f - std::exp(-dt / std::max(aileronTau, 1e-3f)));
	float effectiveRoll = aileronMeasDeg / maxAileronDeg;
	if (std::fabs(effectiveRoll) > 0.02f && !ground) {
		roll += effectiveRoll * rollRate * dt;
	} else {
		// Smooth pull-back to wings-level (fast on the wheels).
		roll *= std::max(0.0f, 1.0f - (ground ? 6.0f : autoLevelRate) * dt);
	}
	// A wingtip may never go below the surface: cap the bank by the height available.
	auto capBank = [&]() {
		float surfaceClearance = position.y - (groundY - gearHeight);
		float bankCap = glm::degrees(std::asin(std::clamp(surfaceClearance / halfSpan, 0.0f, 1.0f)));
		roll = std::clamp(roll, -std::min(maxRoll, bankCap), std::min(maxRoll, bankCap));
	};
	capBank();

	// Banking induces yaw — the harder you bank, the sharper you turn.
	// Sign chosen so LEFT key (negative roll) yaws LEFT and RIGHT yaws right.
	yaw += roll * yawFromRoll * dt;
	// On the wheels the roll keys steer instead, scaled by how fast we are rolling.
	if (ground) yaw += in.rollAxis * groundSteerRate * dt * std::min(1.0f, speed() / 15.0f);

	// Mouse: optional fine yaw/pitch trim (very subtle so the plane stays
	// the primary control).
	if (mouseTrim) {
		yaw   += in.mouseDX * 0.04f;
		pitch -= in.mouseDY * 0.04f;
	}
	pitch = std::clamp(pitch, -maxPitch, maxPitch);

	// Target speed. Airborne: thrust, plus a dive adds speed and a climb bleeds it
	// (gravityGain 0 keeps the old model). On the wheels: thrust only, no floor.
	glm::vec3 fwd = forward();
	float pitchRad = glm::radians(pitch);
	float targetSpeed = ground
		? maxSpeed * throttle
		: std::max(minSpeed, maxSpeed * throttle - std::sin(pitchRad) * gravityGain);
	glm::vec3 desiredVel = fwd * std::max(0.0f, targetSpeed);

	// Exponential smoothing toward the desired velocity → real momentum.
	float k = std::min(1.0f, dt * speedSmoothing);
	velocity = glm::mix(velocity, desiredVel, k);

	if (ground) {
		// Rolling friction on top of the throttle response, so idle actually stops.
		velocity *= std::max(0.0f, 1.0f - groundFriction * dt);
		if (throttle < 0.05f && glm::length(velocity) < 0.5f) velocity = glm::vec3(0.0f);
		velocity.y = std::max(velocity.y, 0.0f);
	} else {
		// Stall: under stall speed, gravity wins and the nose drops.
		float currentSpeed = glm::length(velocity);
		if (currentSpeed < stallSpeed) {
			velocity.y -= gravity * dt;
			pitch = std::max(-maxPitch, pitch - 30.0f * dt);
		}
	}

	// Integrate position; the surface is a hard floor.
	position += velocity * dt;
	if (position.y < groundY) {
		position.y = groundY;
		velocity.y = 0.0f;
		pitch = std::max(pitch, 0.0f);  // touchdown: nose comes level, it does not plough in
	}
	capBank();  // again at the new altitude, so a descending wingtip cannot end the frame underground

	syncCamera();
}
