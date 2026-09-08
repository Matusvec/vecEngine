// BLACKBOX flight scene built on the vecEngine modules. Not the game: no rings, balloons,
// missiles or scoring. A flat airfield, six aircraft on a ramp, one of them flown.
//
//   SPACE  take off (refused while this tail is held)     F  toggle aileron actuator failure
//   C      fleet overview camera                          R  back to the ramp
//   Mouse  orbit the camera around the aircraft (eases back behind it when idle)
//   W/S throttle (1..9 presets, 1 = idle), Up/Down pitch, Left/Right bank (steer on the ground), ESC quit
//   Land: line up, idle, let it settle; once stopped at idle it is parked again and SPACE takes off from there.
//
// Env: BLACKBOX_TAIL (N101), BLACKBOX_UDP_PORT (5005), BLACKBOX_HOLDS (../../palantir/bridge/holds.json),
//      BLACKBOX_FAILING_TAU (0.6)
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "blackbox/holds.h"
#include "blackbox/telemetry.h"
#include "core/input.h"
#include "game/flight_controller.h"
#include "game/plane_model.h"
#include "renderer/camera.h"
#include "renderer/frustum.h"
#include "renderer/light.h"
#include "renderer/mesh.h"
#include "renderer/overlay.h"
#include "renderer/shader.h"
#include "renderer/shadow_map.h"
#include "renderer/skybox.h"
#include "renderer/texture.h"
#include "world/grass.h"
#include "world/terrain.h"

namespace {
int fbWidth = 1920, fbHeight = 1200;
float fbAspect = 1.6f;
void onResize(GLFWwindow*, int w, int h) {
	fbWidth = w; fbHeight = h; fbAspect = h > 0 ? (float)w / h : 1.0f;
	glViewport(0, 0, w, h);
}
std::string envOr(const char* key, const char* fallback) {
	const char* v = std::getenv(key);
	return v && *v ? v : fallback;
}
bool pressedEdge(const Input& input, int key, bool& was) {
	bool now = input.isKeyPressed(key);
	bool edge = now && !was;
	was = now;
	return edge;
}
constexpr float GROUND = 30.0f;                 // flat world height, grass band in basic.frag
const std::vector<std::string> FLEET = {"N101", "N102", "N103", "N104", "N105", "N106"};
glm::vec3 rampSlot(int i) { return {-75.0f + 30.0f * i, GROUND + 1.2f, 0.0f}; }  // a line of six, 30 m apart
const glm::vec3 HELD_TINT{1.0f, 0.25f, 0.25f};

// Flat rectangle in the XZ plane, normal up, world-space UVs so the ground texture tiles.
Mesh makeQuad(glm::vec3 center, float halfX, float halfZ, float uvScale = 0.05f) {
	std::vector<Vertex> v;
	glm::vec3 n{0.0f, 1.0f, 0.0f};
	for (auto [sx, sz] : {std::pair{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}) {
		glm::vec3 pos = center + glm::vec3(sx * halfX, 0.0f, sz * halfZ);
		v.push_back({pos, n, {pos.x * uvScale, pos.z * uvScale}});
	}
	return Mesh(v, {0, 2, 1, 0, 3, 2});
}
}  // namespace

int main() {
	const std::string tail = envOr("BLACKBOX_TAIL", "N101");
	const float failingTau = std::stof(envOr("BLACKBOX_FAILING_TAU", "0.6"));
	const float healthyTau = 0.05f;
	int flownIndex = 0;
	for (int i = 0; i < (int)FLEET.size(); ++i) if (FLEET[i] == tail) flownIndex = i;

	std::cerr << "=== BLACKBOX on vecEngine === flying " << tail << ", failing tau " << failingTau << " s\n"
	          << "SPACE take off  F actuator failure  C fleet camera  R ramp  ESC quit\n";

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	if (const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor())) {  // fill the monitor, still a window
		fbWidth = mode->width;
		fbHeight = mode->height;
	}
	GLFWwindow* window = glfwCreateWindow(fbWidth, fbHeight, "BLACKBOX", nullptr, nullptr);
	glfwMaximizeWindow(window);
	if (!window) { std::cerr << "no window\n"; glfwTerminate(); return 1; }
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, onResize);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "no GLAD\n"; return 1; }
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	onResize(window, fbWidth, fbHeight);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	Terrain::setFlat(GROUND);
	{
		Shader shader("shaders/basic.vert", "shaders/basic.frag");
		Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
		Camera camera(rampSlot(flownIndex), {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f);
		camera.farPlane = 3000.0f;  // the ground plane reaches the horizon, fog does the rest
		Input input(window);
		FlightController controller(camera, rampSlot(flownIndex));
		controller.mouseTrim = false;
		controller.toggleView();  // third person from the start
		controller.freeze();      // parked
		controller.throttle = 0.0f;  // engine off on the ramp
		// Calmer trainer-like handling than the arcade game. Units are metres and seconds.
		// ponytail: tuned by feel, not aero; add lift/drag if the video needs real energy management
		controller.minSpeed = 0.0f;         // idle in the air means you stall, like a real trainer
		controller.gravityGain = 120.0f;    // a 30 deg dive adds ~60 m/s of target speed, a climb bleeds it
		controller.groundY = GROUND + 1.2f; // wheels on the flat world; landings, ground roll, no wings in the dirt
		controller.maxSpeed = 150.0f;       // ~290 kt
		controller.speedSmoothing = 0.5f;   // throttle takes a couple of seconds to bite
		controller.pitchRate = 20.0f;       // deg/s
		controller.maxPitch = 25.0f;
		controller.rollRate = 70.0f;        // deg/s at full aileron
		controller.maxRoll = 60.0f;
		controller.autoLevelRate = 1.0f;
		controller.yawFromRoll = 0.45f;     // 60 deg bank -> 27 deg/s turn
		controller.stallSpeed = 40.0f;
		Skybox skybox;
		Overlay overlay;
		LightingSystem lighting;
		ShadowMap shadowMap(2048);
		constexpr int SHADOW_UNIT = 1;
		Frustum frustum;
		// Box plane by default. Opt into a model with BLACKBOX_MODEL=assets/plane.obj (BLACKBOX_MODEL_YAW turns its nose to -Z).
		PlaneModel planeModel(envOr("BLACKBOX_MODEL", ""), std::stof(envOr("BLACKBOX_MODEL_YAW", "90")), envOr("BLACKBOX_PROP", ""));
		Shader markingShader("shaders/plane.vert", "shaders/plane.frag");  // untextured, tinted, fogged: fine for tarmac
		Mesh farGround = makeQuad({0.0f, GROUND - 0.3f, 0.0f}, 3000.0f, 3000.0f);
		// Grass airfield with one paved runway ahead of the ramp (aircraft face -Z), centreline dashes and edge lines.
		Mesh runway = makeQuad({0.0f, GROUND + 0.05f, -680.0f}, 22.0f, 650.0f);
		std::vector<Mesh> markings;
		for (float z = -60.0f; z > -1300.0f; z -= 40.0f) markings.push_back(makeQuad({0.0f, GROUND + 0.10f, z}, 0.6f, 9.0f));
		markings.push_back(makeQuad({-21.0f, GROUND + 0.10f, -680.0f}, 0.5f, 650.0f));
		markings.push_back(makeQuad({ 21.0f, GROUND + 0.10f, -680.0f}, 0.5f, 650.0f));
		Telemetry telemetry("127.0.0.1", std::stoi(envOr("BLACKBOX_UDP_PORT", "5005")));
		Holds holds(envOr("BLACKBOX_HOLDS", "../../palantir/bridge/holds.json"));

		// ponytail: a 64x64 two-tone noise is enough ground texture for a flat airfield
		constexpr int texSize = 64;
		std::vector<unsigned char> pixels(texSize * texSize * 3);
		for (int i = 0; i < texSize * texSize; ++i) {
			unsigned h = (unsigned)i * 2654435761u; h ^= h >> 13;
			float n = 0.85f + 0.15f * (float)(h & 255) / 255.0f;
			pixels[i * 3 + 0] = (unsigned char)(70 * n);
			pixels[i * 3 + 1] = (unsigned char)(120 * n);
			pixels[i * 3 + 2] = (unsigned char)(50 * n);
		}
		Texture groundTexture(texSize, texSize, 3, pixels.data());
		shader.use();
		shader.setInt("textureSampler", 0);
		Terrain terrain(14, 16, 32.0f);
		Grass grass(-10.0f, 240.0f, 2.0f);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // mouse orbits the camera, never leaves the window
		float orbitYaw = 0.0f, orbitPitch = 0.0f;  // mouse offsets from the default chase view, drift back when idle
		bool parked = true, actuatorFailing = false, overview = false;
		bool wasSpace = false, wasFail = false, wasCam = false, wasReset = false, wasHeld = false;
		float lastFrame = 0.0f;
		double lastTelemetry = 0.0, lastTitle = 0.0;

		while (!glfwWindowShouldClose(window)) {
			float now = (float)glfwGetTime();
			float dt = now - lastFrame;
			lastFrame = now;
			input.update();
			if (input.isKeyPressed(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

			holds.poll(now);
			const bool held = holds.isHeld(tail);

			if (pressedEdge(input, GLFW_KEY_SPACE, wasSpace) && parked) {
				if (held) {
					std::cerr << "BLACKBOX: " << tail << " is HELD by maintenance, take-off refused\n";
				} else {
					parked = false;
					controller.throttle = 0.8f;  // full-ish power; the ground model rotates the nose once fast enough
					controller.unfreeze();
					std::cerr << "BLACKBOX: " << tail << " rolling\n";
				}
			}
			// Landed and stopped with the engine at idle: back to parked, SPACE takes off again from here.
			if (!parked && controller.onGround() && controller.speed() < 0.5f && controller.throttle < 0.05f) {
				parked = true;
				controller.freeze();
				std::cerr << "BLACKBOX: " << tail << " stopped\n";
			}
			// Maintenance released the hold: the actuator was replaced, so the failure clears with it.
			if (wasHeld && !held && actuatorFailing) {
				actuatorFailing = false;
				controller.aileronTau = healthyTau;
				std::cerr << "BLACKBOX: " << tail << " released, actuator healthy\n";
			}
			wasHeld = held;
			if (pressedEdge(input, GLFW_KEY_R, wasReset)) {
				parked = true;
				controller.reset(rampSlot(flownIndex));
				controller.freeze();
				controller.throttle = 0.0f;
			}
			if (pressedEdge(input, GLFW_KEY_F, wasFail)) {
				actuatorFailing = !actuatorFailing;
				controller.aileronTau = actuatorFailing ? failingTau : healthyTau;
				std::cerr << "BLACKBOX: aileron actuator " << (actuatorFailing ? "FAILING" : "healthy")
				          << " (tau " << controller.aileronTau << " s)\n";
			}
			if (pressedEdge(input, GLFW_KEY_C, wasCam)) overview = !overview;

			controller.update(dt, input);
			// Third-person orbit camera: mouse swings it around the aircraft, and it eases back behind when idle.
			orbitYaw += input.mouseDeltaX() * 0.12f;
			orbitPitch = std::clamp(orbitPitch - input.mouseDeltaY() * 0.12f, -50.0f, 60.0f);
			orbitYaw -= orbitYaw * std::min(1.0f, dt * 0.6f);
			orbitPitch -= orbitPitch * std::min(1.0f, dt * 0.6f);
			{
				float camYaw = controller.yaw + orbitYaw;
				float camPitch = std::clamp(controller.pitch * 0.4f - 10.0f + orbitPitch, -80.0f, 80.0f);
				float cy = glm::radians(camYaw), cp = glm::radians(camPitch);
				glm::vec3 fwd{std::cos(cy) * std::cos(cp), std::sin(cp), std::sin(cy) * std::cos(cp)};
				camera.position = controller.position - fwd * 18.0f;
				camera.position.y = std::max(camera.position.y, GROUND + 1.5f);
				camera.yaw = camYaw;
				camera.pitch = camPitch;
				camera.roll = 0.0f;
				camera.updateVectors();
			}
			if (overview) {  // pull-back shot: high and behind the ramp, looking down the line of aircraft
				camera.position = {0.0f, GROUND + 90.0f, 170.0f};
				camera.yaw = -90.0f;
				camera.pitch = -28.0f;
				camera.roll = 0.0f;
				camera.updateVectors();
			}

			if (!parked && now - lastTelemetry >= 0.2) {
				telemetry.send(tail, "aileron_l", controller.aileronCmdDeg, controller.aileronMeasDeg);
				lastTelemetry = now;
			}
			if (now - lastTitle > 0.25) {
				std::string title = "BLACKBOX " + tail + (parked ? "  [ON RAMP, SPACE to take off]" : controller.onGround() ? "  [ROLLING, idle to stop]" : "  [AIRBORNE]");
				if (held) title += parked ? "  *** HELD BY MAINTENANCE, grounded ***" : "  *** HELD BY MAINTENANCE, return to ramp (R) ***";
				if (actuatorFailing) title += "  *** AILERON ACTUATOR FAILING, F to heal ***";
				title += "  spd " + std::to_string((int)controller.speed()) + "  thr " + std::to_string((int)(controller.throttle * 100)) + "%";
				glfwSetWindowTitle(window, title.c_str());
				lastTitle = now;
			}

			terrain.update(camera.position);
			grass.update(camera.position, now);

			glm::mat4 lightSpace = shadowMap.computeLightSpaceMatrix(camera.position, lighting.sun.direction);
			shadowMap.beginPass();
			depthShader.use();
			depthShader.setMat4("lightSpaceMatrix", lightSpace);
			// flat ground cannot shadow itself; only the aircraft cast
			for (int i = 0; i < (int)FLEET.size(); ++i) {
				if (i == flownIndex) planeModel.drawDepth(depthShader, controller.position, controller.yaw, controller.pitch, controller.roll);
				else planeModel.drawDepth(depthShader, rampSlot(i), -90.0f, 0.0f, 0.0f);
			}
			shadowMap.endPass(fbWidth, fbHeight);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glm::mat4 view = camera.getViewMatrix();
			glm::mat4 projection = camera.getProjectionMatrix(fbAspect);
			skybox.draw(view, projection, now, lighting.sun.direction);
			shadowMap.bindShadowTexture(SHADOW_UNIT);
			groundTexture.bind(0);
			shader.use();
			shader.setMat4("view", view);
			shader.setMat4("projection", projection);
			lighting.apply(shader, camera.position, lightSpace, SHADOW_UNIT);
			frustum.update(projection * view);
			terrain.draw(shader, frustum, camera.position);
			shader.setMat4("model", glm::mat4(1.0f));
			farGround.draw();

			markingShader.use();
			markingShader.setMat4("view", view);
			markingShader.setMat4("projection", projection);
			markingShader.setMat4("model", glm::mat4(1.0f));
			markingShader.setVec3("tint", {0.30f, 0.30f, 0.32f});
			runway.draw();
			markingShader.setVec3("tint", {1.2f, 1.2f, 1.1f});
			for (const Mesh& m : markings) m.draw();

			grass.draw(view, projection, now, camera.position, lighting, lightSpace, SHADOW_UNIT);

			// The fleet: five parked aircraft plus the flown one. Held tails are red.
			for (int i = 0; i < (int)FLEET.size(); ++i) {
				glm::vec3 tint = holds.isHeld(FLEET[i]) ? HELD_TINT : glm::vec3(1.0f);
				if (i == flownIndex) {
					planeModel.draw(view, projection, controller.position, controller.yaw, controller.pitch, controller.roll,
					                now, controller.throttle, controller.pitchInput,
					                controller.aileronMeasDeg / controller.maxAileronDeg,  // the real, lagged surface
					                tint);
				} else {
					planeModel.draw(view, projection, rampSlot(i), -90.0f, 0.0f, 0.0f, now, 0.0f, 0.0f, 0.0f, tint);
				}
			}

			// Parked: blue wash, or red when held. Airborne and held: a lighter red wash, so a hold issued
			// mid-flight is visible on screen, not just in the window title.
			if (held) overlay.drawTinted(glm::vec3{0.55f, 0.04f, 0.04f}, parked ? 0.25f : 0.12f);
			else if (parked) overlay.drawTinted(glm::vec3{0.0f, 0.06f, 0.16f}, 0.25f);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}
	glfwTerminate();
	return 0;
}
