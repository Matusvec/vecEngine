#include "core/input.h"

#include <GLFW/glfw3.h>

Input::Input(GLFWwindow* window) : window(window) {
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwGetCursorPos(window, &lastX, &lastY);
}

void Input::update() {
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	// First frame the cursor pos is whatever the OS handed us; treat it as the baseline
	// so we don't generate a giant initial delta.
	if (firstFrame) {
		lastX = x;
		lastY = y;
		firstFrame = false;
	}

	deltaX = (float)(x - lastX);
	deltaY = (float)(y - lastY);
	lastX = x;
	lastY = y;
}

bool Input::isKeyPressed(int key) const {
	return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Input::isMouseButtonPressed(int button) const {
	return glfwGetMouseButton(window, button) == GLFW_PRESS;
}
