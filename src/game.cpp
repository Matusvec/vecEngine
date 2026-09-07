#include "game/game.h"

#include "game/balloon.h"
#include "game/flight_controller.h"
#include "game/ring.h"
#include "world/terrain.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

namespace {
constexpr float CRASH_CLEARANCE = 1.0f;  // plane is "in" the ground if below terrain + this
}

Game::Game(BalloonSystem& balloons, RingSystem& rings, FlightController& plane,
           GLFWwindow* window)
	: balloons(balloons), rings(rings), plane(plane), window(window) {
	// Boot in MENU — plane is frozen until the player presses SPACE.
	plane.freeze();
	writeHudTitle();
}

void Game::award(int points) {
	if (st != State::PLAYING) return;
	totalScore += points;
}

void Game::startPlaying() {
	if (st != State::MENU) return;
	st = State::PLAYING;
	plane.unfreeze();
	std::cerr << "\n>>> GAME STARTED — fly the rings, pop the balloons, don't crash <<<\n";
	writeHudTitle();
}

void Game::restart() {
	totalScore = 0;
	st = State::PLAYING;
	plane.unfreeze();
	std::cerr << "\n>>> RESTARTED <<<\n";
	writeHudTitle();
}

void Game::update(float /*dt*/) {
	// HUD always refreshes so the title reflects state changes immediately.
	double now = glfwGetTime();
	bool refreshHud = (now - lastHudTime > 0.25);

	if (st != State::PLAYING) {
		if (refreshHud) {
			writeHudTitle();
			lastHudTime = now;
		}
		return;
	}

	// Crash: plane below terrain.
	float groundY = Terrain::heightAt(plane.position.x, plane.position.z);
	if (plane.position.y < groundY + CRASH_CLEARANCE) {
		crashed();
		return;
	}

	// Win: every ring passed AND every balloon popped.
	if (rings.allPassed() && balloons.aliveCount() == 0) {
		won();
		return;
	}

	if (refreshHud) {
		writeHudTitle();
		lastHudTime = now;
	}
}

void Game::crashed() {
	st = State::LOST;
	plane.freeze();
	std::cerr << "\n*** CRASHED *** Final score: " << totalScore << "\n";
	writeHudTitle();
}

void Game::won() {
	st = State::WON;
	plane.freeze();
	std::cerr << "\n*** YOU WIN! *** Final score: " << totalScore << "\n";
	writeHudTitle();
}

void Game::writeHudTitle() {
	std::ostringstream ss;
	ss << "vecEngine — ";
	switch (st) {
		case State::MENU:
			ss << "[ PRESS SPACE TO START ]   "
			   << "W/S throttle · Up/Down pitch · Left/Right bank · LMB shoot · "
			   << "Pop balloons + fly rings · Don't crash · ESC quit";
			break;
		case State::PLAYING:
			ss << "Score " << totalScore
			   << "  |  Rings " << rings.passedCount() << "/" << rings.total()
			   << "  |  Balloons " << (balloons.total() - balloons.aliveCount())
			   << "/" << balloons.total()
			   << "  |  Spd " << (int)plane.speed()
			   << "  Thr " << (int)(plane.throttle * 100.0f) << "%";
			break;
		case State::WON:
			ss << "*** YOU WIN! Final score " << totalScore << "  (ESC to quit)";
			break;
		case State::LOST:
			ss << "*** CRASHED. Final score " << totalScore << "  (ESC to quit)";
			break;
	}
	glfwSetWindowTitle(window, ss.str().c_str());
}
