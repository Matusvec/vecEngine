#pragma once

#include <glm/glm.hpp>

class Camera;
class Input;

// Arcade-style flight model:
//   throttle (W/S) → target speed
//   pitch (UP/DOWN) → climb/dive
//   roll (LEFT/RIGHT) → banking; roll auto-induces yaw, so banking is how you turn
//   no input → roll smooths back toward 0 (auto-level)
// State persists frame to frame (real momentum), so the plane keeps moving when
// you take your hands off the controls.
class FlightController {
public:
	FlightController(Camera& camera, const glm::vec3& startPos);

	// Advances physics one step. If the plane is frozen (game over), no-op.
	void update(float dt, const Input& input);

	void freeze()   { frozen = true; }
	void unfreeze() { frozen = false; }
	bool isFrozen() const { return frozen; }

	// Reset position + orientation + velocity to a fresh spawn state. Used
	// by the R-key restart handler. Does NOT toggle frozen; caller decides.
	void reset(const glm::vec3& spawnPos);

	void toggleView() { thirdPerson = !thirdPerson; }
	bool isThirdPerson() const { return thirdPerson; }

	// Public state — the systems that follow the plane (missile origin, game
	// crash detection, score HUD) read these directly.
	glm::vec3 position;
	glm::vec3 velocity{0.0f};
	float yaw   = -90.0f;  // degrees
	float pitch = 0.0f;
	float roll  = 0.0f;
	float throttle = 0.20f; // [0..1] — start gentle so the player has time to orient

	// Last-frame control inputs in [-1, 1]. Exposed so the third-person plane
	// model can animate ailerons/elevator while a key is held.
	float pitchInput = 0.0f;
	float rollInput  = 0.0f;

	// Tunables — exposed so we can tweak feel from main if needed.
	float minSpeed = 25.0f;
	float maxSpeed = 240.0f;
	float speedSmoothing = 1.6f;   // higher = snappier throttle response
	float pitchRate = 55.0f;       // deg/sec while pitching
	float rollRate = 110.0f;       // deg/sec while rolling
	float maxRoll  = 75.0f;        // bank-angle cap
	float maxPitch = 85.0f;
	float autoLevelRate = 1.8f;    // higher = roll snaps back faster
	float yawFromRoll = 1.1f;      // larger = sharper banked turns
	float stallSpeed = 30.0f;
	float gravity = 18.0f;

	float speed() const;
	glm::vec3 forward() const;

private:
	void syncCamera();

	Camera& camera;
	bool frozen = false;
	bool thirdPerson = false;
};
