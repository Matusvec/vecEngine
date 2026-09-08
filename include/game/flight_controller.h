#pragma once
#include <glm/glm.hpp>

class Camera;
class Input;

// One frame of pilot input, already decoded from whatever device produced it.
// Keeps the physics step free of GLFW so any app (or a test) can drive it.
struct FlightInput {
	float throttleAxis = 0.0f;  // -1 (S) .. +1 (W); +2 with shift burn
	float pitchAxis = 0.0f;     // +1 = pull (nose up), -1 = push (nose down)
	float rollAxis = 0.0f;      // -1 left .. +1 right
	float mouseDX = 0.0f, mouseDY = 0.0f;
	int throttlePreset = -1;    // 0..8 from the number row, -1 = none
};

// Arcade-style flight model:
//   throttle (W/S) → target speed
//   pitch (UP/DOWN) → climb/dive
//   roll (LEFT/RIGHT) → banking; roll auto-induces yaw, so banking is how you turn
//   no input → roll smooths back toward 0 (auto-level)
// State persists frame to frame (real momentum), so the plane keeps moving when
// you take your hands off the controls.
//
// Ground: apps with a landable surface set groundY (the altitude `position.y` rests at
// on the wheels). On the ground the plane rolls with friction, cannot bank, steers with
// the roll keys, and rotates by itself once past rotateSpeed. Near the ground the bank
// angle is capped so a wingtip never goes below the surface. Default groundY is far
// below everything, which is the old behaviour.
class FlightController {
public:
	FlightController(Camera& camera, const glm::vec3& startPos);

	// Reads GLFW keys/mouse and advances one step. Defined in flight_input.cpp.
	void update(float dt, const Input& input);
	// Advances physics one step from decoded input. If frozen, no-op.
	void step(float dt, const FlightInput& in);

	void freeze()   { frozen = true; }
	void unfreeze() { frozen = false; }
	bool isFrozen() const { return frozen; }
	bool onGround() const { return position.y <= groundY + 0.05f; }

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

	// BLACKBOX: left aileron actuator model. Roll is driven by the MEASURED
	// deflection, which chases the commanded deflection with a first-order lag.
	// Healthy tau ~0.05 s is imperceptible; failing tau ~0.6 s feels sluggish.
	// ponytail: first-order lag only, no rate limit, add one if the detector needs it
	float aileronCmdDeg = 0.0f;
	float aileronMeasDeg = 0.0f;
	float maxAileronDeg = 20.0f;
	float aileronTau = 0.05f;

	// Tunables — exposed so we can tweak feel from main if needed.
	float minSpeed = 25.0f;        // airborne speed floor; set 0 for a plane that can stall at idle
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
	float gravityGain = 0.0f;      // m/s of target speed gained per unit of sin(dive); 0 = old model
	bool mouseTrim = true;         // BLACKBOX turns this off, the sim flies on keys only

	// Ground model (see class comment).
	float groundY = -1.0e9f;       // resting altitude of `position` on the wheels
	float gearHeight = 1.2f;       // position.y minus the surface when resting
	float halfSpan = 2.8f;         // wingtip reach from the centreline
	float rotateSpeed = 40.0f;     // ground speed at which the nose comes up by itself
	float rotatePitch = 8.0f;
	float groundFriction = 0.6f;   // per-second speed decay when rolling at idle
	float groundSteerRate = 45.0f; // deg/sec of yaw from the roll keys on the ground

	float speed() const;
	glm::vec3 forward() const;

private:
	void syncCamera();
	Camera& camera;
	bool frozen = false;
	bool thirdPerson = false;
};
