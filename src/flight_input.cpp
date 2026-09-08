// GLFW key/mouse decoding for FlightController. Kept out of flight_controller.cpp so the
// physics compiles and tests without a window.
#include "core/input.h"
#include "game/flight_controller.h"

#include <GLFW/glfw3.h>

static FlightInput readFlightInput(const Input& input) {
	FlightInput in;
	if (input.isKeyPressed(GLFW_KEY_W)) in.throttleAxis += 1.0f;
	if (input.isKeyPressed(GLFW_KEY_S)) in.throttleAxis -= 1.0f;
	if (input.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) in.throttleAxis += 2.2f;  // emergency burn
	for (int i = 0; i < 9; ++i)
		if (input.isKeyPressed(GLFW_KEY_1 + i)) in.throttlePreset = i;
	if (input.isKeyPressed(GLFW_KEY_UP))   in.pitchAxis -= 1.0f;
	if (input.isKeyPressed(GLFW_KEY_DOWN)) in.pitchAxis += 1.0f;
	if (input.isKeyPressed(GLFW_KEY_LEFT))  in.rollAxis -= 1.0f;
	if (input.isKeyPressed(GLFW_KEY_RIGHT)) in.rollAxis += 1.0f;
	in.mouseDX = input.mouseDeltaX();
	in.mouseDY = input.mouseDeltaY();
	return in;
}

void FlightController::update(float dt, const Input& input) {
	step(dt, readFlightInput(input));
}
