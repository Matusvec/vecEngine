#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "blackbox/holds.h"
#include "blackbox/telemetry.h"
#include "core/input.h"
#include "game/balloon.h"
#include "game/flight_controller.h"
#include "game/game.h"
#include "game/missile.h"
#include "game/plane_model.h"
#include "game/ring.h"
#include "renderer/camera.h"
#include "renderer/frustum.h"
#include "renderer/light.h"
#include "renderer/overlay.h"
#include "renderer/shader.h"
#include "renderer/shadow_map.h"
#include "renderer/skybox.h"
#include "renderer/texture.h"
#include "world/grass.h"
#include "world/rain.h"
#include "world/shrubs.h"
#include "world/terrain.h"
#include "world/water.h"

// Cached framebuffer state — refreshed only by framebufferSizeCallback so the render
// loop doesn't have to query the OS every frame.
int framebufferWidth = 2560;
int framebufferHeight = 1600;
float framebufferAspect = 2560.0f / 1600.0f;

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	framebufferWidth = width;
	framebufferHeight = height;
	framebufferAspect = (height > 0) ? (float)width / (float)height : 1.0f;
	glViewport(0, 0, width, height);
}

int main() {
	std::cerr << "\n=================== vecEngine ===================\n"
	          << "Controls:\n"
	          << "  W / S         throttle up / down\n"
	          << "  Up / Down     pitch (climb / dive)\n"
	          << "  Left / Right  bank/roll (turn by leaning)\n"
	          << "  1-9           throttle preset (1=idle, 9=max)\n"
	          << "  Shift         quick burn\n"
	          << "  LMB           fire missile\n"
	          << "  V             toggle cockpit / 3rd-person view\n"
	          << "  R             restart\n"
	          << "  SPACE         start game\n"
	          << "  ESC           quit\n"
	          << "  F             BLACKBOX: toggle aileron actuator failure\n"
	          << "\nBLACKBOX env: BLACKBOX_TAIL (N101), BLACKBOX_UDP_PORT (5005),\n"
	          << "  BLACKBOX_HOLDS (../../palantir/bridge/holds.json), BLACKBOX_FAILING_TAU (0.6)\n"
	          << "\nGoal: pop every balloon + fly through every ring.\n"
	          << "Lose if you crash into the ground.\n"
	          << ">>> PRESS SPACE TO START <<<\n\n";

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);  // 4x MSAA — anti-aliases triangle edges

	GLFWwindow* window = glfwCreateWindow(2560, 1600, "vecEngine", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Sync cached framebuffer size with the actual surface (HiDPI may differ from window size).
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
	framebufferAspect = (framebufferHeight > 0)
		? (float)framebufferWidth / (float)framebufferHeight : 1.0f;
	glViewport(0, 0, framebufferWidth, framebufferHeight);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Inner scope so GL-resource owners (Shader, Mesh) destruct before glfwTerminate kills the context.
	{
		Shader shader("shaders/basic.vert", "shaders/basic.frag");
		Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
		// Spawn well above whatever terrain happens to be at the start XZ —
		// otherwise mountains under the spawn point trip the crash check on frame 1.
		const float spawnX = 0.0f;
		const float spawnZ = 200.0f;
		const float spawnY = Terrain::heightAt(spawnX, spawnZ) + 120.0f;
		Camera camera({spawnX, spawnY, spawnZ}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f);
		Input input(window);
		FlightController controller(camera, camera.position);
		Skybox skybox;
		Overlay overlay;
		LightingSystem lighting;
		ShadowMap shadowMap(2048);
		constexpr int SHADOW_MAP_UNIT = 1;

		// Procedural grass texture: 4-octave smooth noise blended with occasional
		// dirt patches. Bigger (512²) and richer than the original block-noise so
		// surfaces have visible variation when the camera flies low.
		constexpr int grassSize = 512;
		std::vector<unsigned char> grassPixels(grassSize * grassSize * 3);
		{
			auto hash01 = [](int x, int y, int seed) -> float {
				uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u
				             + (uint32_t)seed * 2147483647u;
				h = (h ^ (h >> 13)) * 1274126177u;
				h = h ^ (h >> 16);
				return (float)(h & 0xFFFF) / 65535.0f;  // [0, 1)
			};
			auto smoothNoise = [&](float x, float y, int seed) -> float {
				int ix = (int)std::floor(x);
				int iy = (int)std::floor(y);
				float fx = x - (float)ix;
				float fy = y - (float)iy;
				fx = fx * fx * (3.0f - 2.0f * fx);  // smoothstep
				fy = fy * fy * (3.0f - 2.0f * fy);
				float a = hash01(ix,     iy,     seed);
				float b = hash01(ix + 1, iy,     seed);
				float c = hash01(ix,     iy + 1, seed);
				float d = hash01(ix + 1, iy + 1, seed);
				float ab = a + (b - a) * fx;
				float cd = c + (d - c) * fx;
				return ab + (cd - ab) * fy;
			};
			auto fbm = [&](float x, float y, int seed) -> float {
				float v = 0.0f, amp = 0.5f, freq = 1.0f;
				for (int i = 0; i < 4; ++i) {
					v += smoothNoise(x * freq, y * freq, seed + i) * amp;
					freq *= 2.0f;
					amp *= 0.5f;
				}
				return v;  // [0, ~1)
			};
			for (int y = 0; y < grassSize; ++y) {
				for (int x = 0; x < grassSize; ++x) {
					float fx = (float)x / (float)grassSize * 16.0f;  // 16 large patches per tile
					float fy = (float)y / (float)grassSize * 16.0f;

					float n = fbm(fx, fy, 0);          // primary green variation
					float patch = smoothNoise(fx * 0.4f, fy * 0.4f, 99);  // dirt mask
					float dirt = std::clamp((patch - 0.55f) / 0.20f, 0.0f, 1.0f);

					float r = 0.18f + n * 0.18f;
					float g = 0.42f + n * 0.30f;
					float b = 0.14f + n * 0.14f;

					// Mix toward dry/dirt color in patches.
					float dr = 0.46f + n * 0.14f;
					float dg = 0.38f + n * 0.10f;
					float db = 0.22f + n * 0.06f;
					r = r * (1.0f - dirt) + dr * dirt;
					g = g * (1.0f - dirt) + dg * dirt;
					b = b * (1.0f - dirt) + db * dirt;

					int idx = (y * grassSize + x) * 3;
					grassPixels[idx + 0] = (unsigned char)std::clamp((int)(r * 255.0f), 0, 255);
					grassPixels[idx + 1] = (unsigned char)std::clamp((int)(g * 255.0f), 0, 255);
					grassPixels[idx + 2] = (unsigned char)std::clamp((int)(b * 255.0f), 0, 255);
				}
			}
		}
		Texture grassTexture(grassSize, grassSize, 3, grassPixels.data());

		shader.use();
		shader.setInt("textureSampler", 0);

		Terrain terrain(/*chunkRadius=*/14, /*chunkResolution=*/16, /*chunkSize=*/32.0f);
		Water water(/*seaLevel=*/-10.0f, /*size=*/3000.0f, /*subdivisions=*/80);
		Grass grass(/*seaLevel=*/-10.0f, /*radius=*/240.0f, /*cellSize=*/2.0f);
		// Shrubbery on the upper-grass + forest band (must overlap basic.frag bands).
		Shrubs shrubs(/*minY=*/45.0f, /*maxY=*/170.0f,
		              /*radius=*/280.0f, /*cellSize=*/6.0f);
		Rain rain(/*dropCount=*/8000, /*boxRadius=*/60.0f);
		MissileSystem missiles;
		PlaneModel planeModel;
		Frustum frustum;

		// Game systems: targets to shoot, course rings to fly through, scoring.
		BalloonSystem balloons;
		balloons.seed(/*count=*/30, /*spread=*/700.0f,
		              /*minHeightAboveTerrain=*/35.0f, /*maxHeightAboveTerrain=*/120.0f);
		RingSystem rings;
		// Tight course at typical cruise altitude — close to spawn so the first
		// ring is reachable inside ~10s of forward flight from spawn.
		rings.seed(/*count=*/8, /*courseRadius=*/220.0f,
		           /*minHeight=*/90.0f, /*maxHeight=*/140.0f);
		// Tutorial ring directly in front of spawn (heading -Z), at spawn altitude.
		// Just fly straight to score it — verifies the detection chain works.
		rings.prependRing(
			glm::vec3(spawnX, spawnY, spawnZ - 80.0f),  // 80 units ahead
			glm::vec3(0.0f, 0.0f, -1.0f),               // facing the player
			/*radius=*/14.0f);
		Game game(balloons, rings, controller, window);

		// BLACKBOX wiring. Everything configurable lives in env vars so the same
		// binary can play N101 today and N104 tomorrow.
		auto envOr = [](const char* key, const char* fallback) {
			const char* v = std::getenv(key);
			return std::string(v && *v ? v : fallback);
		};
		const std::string tail = envOr("BLACKBOX_TAIL", "N101");
		const float healthyTau = 0.05f;
		const float failingTau = std::stof(envOr("BLACKBOX_FAILING_TAU", "0.6"));
		Telemetry telemetry("127.0.0.1", std::stoi(envOr("BLACKBOX_UDP_PORT", "5005")));
		Holds holds(envOr("BLACKBOX_HOLDS", "../../palantir/bridge/holds.json"));
		bool actuatorFailing = false;
		bool wasFailDown = false;
		double lastTelemetry = 0.0;
		std::cerr << "BLACKBOX: flying " << tail << ", failing tau " << failingTau << " s\n";

		// Print ring positions at startup so the player knows where to fly.
		std::cerr << "\nRing course (fly through them in order):\n";
		for (int i = 0; i < rings.total(); ++i) {
			const auto& r = rings.list()[i];
			std::cerr << "  Ring " << i << " at ("
			          << (int)r.position.x << ", "
			          << (int)r.position.y << ", "
			          << (int)r.position.z << ")\n";
		}
		std::cerr << "\n";

		bool wasFireDown = false;
		bool wasSpaceDown = false;
		bool wasViewDown = false;
		bool wasResetDown = false;

		float lastFrame = 0.0f;
		double lastReportTime = glfwGetTime();

		while (!glfwWindowShouldClose(window)) {
			float currentFrame = (float)glfwGetTime();
			float deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			input.update();
			if (input.isKeyPressed(GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, true);
			}

			// SPACE-press edge → start the game from the MENU screen.
			holds.poll(currentFrame);
			const bool held = holds.isHeld(tail);

			bool isSpaceDown = input.isKeyPressed(GLFW_KEY_SPACE);
			if (isSpaceDown && !wasSpaceDown && game.state() == Game::State::MENU) {
				if (held) std::cerr << "BLACKBOX: " << tail << " is HELD by maintenance, cannot take off\n";
				else game.startPlaying();
			}
			wasSpaceDown = isSpaceDown;

			// F-press edge → toggle the actuator failure. Foundry never sees this flag,
			// it only sees the telemetry.
			bool isFailDown = input.isKeyPressed(GLFW_KEY_F);
			if (isFailDown && !wasFailDown) {
				actuatorFailing = !actuatorFailing;
				controller.aileronTau = actuatorFailing ? failingTau : healthyTau;
				std::cerr << "BLACKBOX: aileron actuator " << (actuatorFailing ? "FAILING" : "healthy")
				          << " (tau " << controller.aileronTau << " s)\n";
			}
			wasFailDown = isFailDown;

			// V-press edge → toggle camera view (cockpit / chase).
			bool isViewDown = input.isKeyPressed(GLFW_KEY_V);
			if (isViewDown && !wasViewDown) {
				controller.toggleView();
			}
			wasViewDown = isViewDown;

			// R-press edge → restart everything (plane, score, balloons, rings,
			// missiles, terrain craters).
			bool isResetDown = input.isKeyPressed(GLFW_KEY_R);
			if (isResetDown && !wasResetDown && !held) {
				balloons.reset();
				rings.reset();
				missiles.clear();
				controller.reset(glm::vec3(spawnX, spawnY, spawnZ));
				terrain.clearCraters();
				game.restart();
			}
			wasResetDown = isResetDown;

			controller.update(deltaTime, input);

			// Fire missile on left-click edge (one shot per click, not held).
			// Block firing once the game has ended so the crashed plane stops shooting.
			bool isFireDown = input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
			if (isFireDown && !wasFireDown && game.state() == Game::State::PLAYING) {
				// Pass plane velocity so missiles don't fall behind a moving aircraft.
				missiles.fire(controller.position, controller.forward(), controller.velocity);
			}
			wasFireDown = isFireDown;

			missiles.update(deltaTime, terrain);

			// Score only counts during PLAYING — gates ring/balloon detection
			// so a frozen menu plane sitting near a ring doesn't auto-advance.
			if (game.state() == Game::State::PLAYING) {
				game.award(missiles.checkBalloonHits(balloons));
				game.award(rings.update(controller.position));
			}

			// Game tick handles crash detection + win check.
			game.update(deltaTime);

			// BLACKBOX: 5 Hz telemetry while airborne, and a held banner that wins over the HUD.
			if (game.state() == Game::State::PLAYING && currentFrame - lastTelemetry >= 0.2f) {
				telemetry.send(tail, "aileron_l", controller.aileronCmdDeg, controller.aileronMeasDeg);
				lastTelemetry = currentFrame;
			}
			if (held) {
				glfwSetWindowTitle(window, ("*** " + tail + " IS HELD BY MAINTENANCE, grounded until released ***").c_str());
			}

			terrain.update(camera.position);
			grass.update(camera.position, glfwGetTime());
			shrubs.update(camera.position, glfwGetTime());

			// Shadow pass: render terrain depth into the shadow map.
			glm::mat4 lightSpaceMatrix =
				shadowMap.computeLightSpaceMatrix(camera.position, lighting.sun.direction);

			shadowMap.beginPass();
			depthShader.use();
			depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
			terrain.drawDepth(depthShader, camera.position);
			shadowMap.endPass(framebufferWidth, framebufferHeight);

			// Main pass: bind shadow map for sampling, render the world.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 view = camera.getViewMatrix();
			glm::mat4 projection = camera.getProjectionMatrix(framebufferAspect);

			skybox.draw(view, projection, currentFrame, lighting.sun.direction);

			shadowMap.bindShadowTexture(SHADOW_MAP_UNIT);
			grassTexture.bind(0);

			shader.use();
			shader.setMat4("view", view);
			shader.setMat4("projection", projection);
			lighting.apply(shader, camera.position, lightSpaceMatrix, SHADOW_MAP_UNIT);

			frustum.update(projection * view);
			int drawn = terrain.draw(shader, frustum, camera.position);

			grass.draw(view, projection, currentFrame, camera.position,
			           lighting, lightSpaceMatrix, SHADOW_MAP_UNIT);
			shrubs.draw(view, projection, currentFrame, camera.position,
			            lighting, lightSpaceMatrix, SHADOW_MAP_UNIT);

			// Water uses alpha blending. Disable depth writes so things drawn
			// AFTER water (missiles, rain) still appear at their true depth and
			// aren't occluded by the translucent surface.
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			water.draw(view, projection, camera.position, currentFrame,
			           lighting, lightSpaceMatrix, SHADOW_MAP_UNIT);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);

			missiles.draw(view, projection);
			balloons.draw(view, projection, currentFrame, camera.position);
			rings.draw(view, projection, currentFrame, camera.position);

			// Plane mesh visible only in third-person — in cockpit view the
			// camera is inside the plane, so drawing it would obscure everything.
			if (controller.isThirdPerson()) {
				planeModel.draw(view, projection, controller.position,
				                controller.yaw, controller.pitch, controller.roll,
				                currentFrame, controller.throttle,
				                controller.pitchInput, controller.rollInput);
			}

			rain.draw(view, projection, camera.position, currentFrame);

			// State overlay LAST so it sits above the world. MENU = dark blue,
			// LOST = red, WON = green. PLAYING draws nothing.
			switch (game.state()) {
				case Game::State::MENU:
					if (held) overlay.drawTinted({0.55f, 0.04f, 0.04f}, 0.55f);
					else overlay.drawTinted({0.00f, 0.06f, 0.16f}, 0.55f);
					break;
				case Game::State::LOST:
					overlay.drawTinted({0.55f, 0.04f, 0.04f}, 0.45f);
					break;
				case Game::State::WON:
					overlay.drawTinted({0.05f, 0.45f, 0.10f}, 0.40f);
					break;
				case Game::State::PLAYING:
					break;
			}

			double now = glfwGetTime();
			if (now - lastReportTime > 1.0) {
				std::cerr << "Terrain: " << drawn << "/" << terrain.totalChunks()
				          << " chunks visible\n";
				lastReportTime = now;
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	glfwTerminate();
	return 0;
}
