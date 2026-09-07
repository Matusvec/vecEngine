// BLACKBOX flight scene built on the vecEngine modules. Not the game: no rings, balloons,
// missiles or scoring. A flat airfield, six aircraft on a ramp, one of them flown.
//
//   SPACE  take off (refused while this tail is held)     F  toggle aileron actuator failure
//   C      fleet overview camera                          R  back to the ramp
//   W/S throttle, Up/Down pitch, Left/Right bank, ESC quit
//
// Env: BLACKBOX_TAIL (N101), BLACKBOX_UDP_PORT (5005), BLACKBOX_HOLDS (../../palantir/bridge/holds.json),
//      BLACKBOX_FAILING_TAU (0.6)
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "blackbox/holds.h"
#include "blackbox/telemetry.h"
#include "core/input.h"
#include "game/flight_controller.h"
#include "game/plane_model.h"
#include "renderer/camera.h"
#include "renderer/frustum.h"
#include "renderer/light.h"
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
	GLFWwindow* window = glfwCreateWindow(fbWidth, fbHeight, "BLACKBOX", nullptr, nullptr);
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
		Input input(window);
		FlightController controller(camera, rampSlot(flownIndex));
		controller.mouseTrim = false;
		controller.toggleView();  // third person from the start
		controller.freeze();      // parked
		Skybox skybox;
		Overlay overlay;
		LightingSystem lighting;
		ShadowMap shadowMap(2048);
		constexpr int SHADOW_UNIT = 1;
		Frustum frustum;
		PlaneModel planeModel;
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

		bool parked = true, actuatorFailing = false, overview = false;
		bool wasSpace = false, wasFail = false, wasCam = false, wasReset = false;
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
					controller.pitch = 12.0f;   // rotate and climb; the pilot takes it from here
					controller.throttle = 0.6f;
					controller.unfreeze();
					std::cerr << "BLACKBOX: " << tail << " airborne\n";
				}
			}
			if (pressedEdge(input, GLFW_KEY_R, wasReset)) {
				parked = true;
				controller.reset(rampSlot(flownIndex));
				controller.freeze();
			}
			if (pressedEdge(input, GLFW_KEY_F, wasFail)) {
				actuatorFailing = !actuatorFailing;
				controller.aileronTau = actuatorFailing ? failingTau : healthyTau;
				std::cerr << "BLACKBOX: aileron actuator " << (actuatorFailing ? "FAILING" : "healthy")
				          << " (tau " << controller.aileronTau << " s)\n";
			}
			if (pressedEdge(input, GLFW_KEY_C, wasCam)) overview = !overview;

			controller.update(dt, input);
			if (!parked && controller.position.y < GROUND + 1.2f) {  // flat world: never go below the ramp
				controller.position.y = GROUND + 1.2f;
				if (controller.pitch < 0.0f) controller.pitch = 0.0f;
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
				std::string title = "BLACKBOX " + tail + (parked ? "  [ON RAMP, SPACE to take off]" : "  [AIRBORNE]");
				if (held) title += "  *** HELD BY MAINTENANCE, grounded ***";
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
			terrain.drawDepth(depthShader, camera.position);
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

			if (parked) overlay.drawTinted(held ? glm::vec3{0.55f, 0.04f, 0.04f} : glm::vec3{0.0f, 0.06f, 0.16f}, 0.25f);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}
	glfwTerminate();
	return 0;
}
