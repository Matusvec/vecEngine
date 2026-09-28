#pragma once

struct GLFWwindow;

class Input {
public:
	explicit Input(GLFWwindow* window);

	// Poll the latest mouse position and recompute deltas. Call once per frame
	// before any consumer reads mouseDeltaX/Y or isKeyPressed.
	void update();

	bool isKeyPressed(int key) const;
	bool isMouseButtonPressed(int button) const;
	float mouseDeltaX() const { return deltaX; }
	float mouseDeltaY() const { return deltaY; }

private:
	GLFWwindow* window;
	double lastX = 0.0;
	double lastY = 0.0;
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	bool firstFrame = true;
};
