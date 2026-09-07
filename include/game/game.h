#pragma once

struct GLFWwindow;

class BalloonSystem;
class RingSystem;
class FlightController;

// Top-level game coordinator: owns score + state, runs win/lose checks,
// updates the window-title HUD. Doesn't draw — pure logic.
class Game {
public:
	enum class State { MENU, PLAYING, WON, LOST };

	Game(BalloonSystem& balloons, RingSystem& rings, FlightController& plane,
	     GLFWwindow* window);

	// Per-frame tick. dt unused for now (state changes are event-driven), but
	// kept so we can throttle HUD writes, time bonuses, etc.
	void update(float dt);

	// Awarded by main when the corresponding event fires.
	void award(int points);

	// MENU → PLAYING transition triggered by main when SPACE is pressed.
	void startPlaying();

	// Reset score and state to a fresh PLAYING run. Caller is responsible for
	// resetting the underlying systems (balloons, rings, plane, terrain).
	void restart();

	State state() const { return st; }
	int score() const { return totalScore; }

private:
	void crashed();
	void won();
	void writeHudTitle();

	BalloonSystem& balloons;
	RingSystem& rings;
	FlightController& plane;
	GLFWwindow* window;

	State st = State::MENU;
	int totalScore = 0;
	double lastHudTime = 0.0;
};
