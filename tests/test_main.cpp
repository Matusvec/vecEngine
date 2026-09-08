// Unit tests for vecEngine.
//
// Strategy: every test exercises pure logic that does NOT need an active GL
// context — terrain noise, frustum math, camera basis vectors. The test
// binary links the affected source files plus glad.c (so GL function-pointer
// symbols resolve) but never calls into the loader, so it runs headless.

#include "renderer/camera.h"
#include "renderer/frustum.h"
#include "world/terrain.h"
#include "blackbox/holds.h"
#include "renderer/obj_loader.h"
#include "game/flight_controller.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace {

int totalChecks  = 0;
int failedChecks = 0;

void check(bool cond, const char* name) {
	++totalChecks;
	if (cond) {
		std::printf("  [PASS] %s\n", name);
	} else {
		++failedChecks;
		std::printf("  [FAIL] %s\n", name);
	}
}

bool nearly(float a, float b, float eps = 1e-4f) {
	return std::fabs(a - b) < eps;
}

}  // namespace

// =============================================================================
// Test 1: Terrain::heightAt is deterministic.
// The procedural-noise data structure must produce the same value for the same
// input every time, otherwise crater placement, grass placement, ring seeding,
// and crash detection would all drift between frames.
// =============================================================================
void test_heightAt_deterministic() {
	std::printf("Test 1: Terrain::heightAt is deterministic\n");
	float a = Terrain::heightAt(123.4f, -567.8f);
	float b = Terrain::heightAt(123.4f, -567.8f);
	float c = Terrain::heightAt(0.0f, 0.0f);
	float d = Terrain::heightAt(0.0f, 0.0f);
	check(nearly(a, b, 1e-5f), "same (x,z) returns the same height");
	check(nearly(c, d, 1e-5f), "same (0,0) returns the same height twice");
}

// =============================================================================
// Test 2: Terrain::heightAt produces variation across the world.
// If the noise function returned a constant we'd silently lose all terrain
// shape — this guarantees the FBM stack is actually doing something.
// =============================================================================
void test_heightAt_varies() {
	std::printf("Test 2: Terrain::heightAt varies across the world\n");
	float h00   = Terrain::heightAt(   0.0f,    0.0f);
	float h500  = Terrain::heightAt( 500.0f,  500.0f);
	float h_500 = Terrain::heightAt(-500.0f,  500.0f);

	check(std::fabs(h00 - h500)   > 1.0f, "(0,0) differs from (500,500) by >1 unit");
	check(std::fabs(h00 - h_500)  > 1.0f, "(0,0) differs from (-500,500) by >1 unit");
	check(std::fabs(h500 - h_500) > 1.0f, "(500,500) differs from (-500,500) by >1 unit");
}

// =============================================================================
// Test 3: Frustum::containsSphere returns the right answer.
// This is the core of terrain LOD culling. A bug here either renders
// invisible chunks (perf hit) or culls visible ones (popping holes in the world).
// =============================================================================
void test_frustum_culling() {
	std::printf("Test 3: Frustum::containsSphere\n");
	Frustum f;
	glm::mat4 proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 5.0f),  // eye
		glm::vec3(0.0f, 0.0f, 0.0f),  // look-at
		glm::vec3(0.0f, 1.0f, 0.0f));
	f.update(proj * view);

	check( f.containsSphere(glm::vec3(0.0f, 0.0f,    0.0f), 1.0f),
	       "sphere at look-at target is inside");
	check(!f.containsSphere(glm::vec3(0.0f, 0.0f, 1000.0f), 1.0f),
	       "sphere far behind camera is outside");
	check(!f.containsSphere(glm::vec3(0.0f, 1000.0f,   0.0f), 1.0f),
	       "sphere far above camera is outside");
}

// =============================================================================
// Test 4: Camera::updateVectors produces correct front vector for known angles.
// Yaw/pitch convention is the foundation of every renderer: missile fire
// direction, plane orientation, view matrix. Sign errors here are why your
// LEFT key turned right last week.
// =============================================================================
void test_camera_basis_vectors() {
	std::printf("Test 4: Camera::updateVectors basis vectors\n");

	// yaw = -90 → front along -Z (looking forward into screen).
	Camera cam(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	check(nearly(cam.front.x,  0.0f, 1e-3f), "yaw=-90 -> front.x ~ 0");
	check(nearly(cam.front.y,  0.0f, 1e-3f), "yaw=-90 -> front.y ~ 0");
	check(nearly(cam.front.z, -1.0f, 1e-3f), "yaw=-90 -> front.z ~ -1");

	// yaw = 0 → front along +X.
	Camera cam2(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
	check(nearly(cam2.front.x, 1.0f, 1e-3f), "yaw=0 -> front.x ~ 1");
	check(nearly(cam2.front.z, 0.0f, 1e-3f), "yaw=0 -> front.z ~ 0");

	// Pitch up by 90 → front straight up.
	Camera cam3(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 90.0f);
	check(nearly(cam3.front.y, 1.0f, 1e-3f), "pitch=90 -> front.y ~ 1");
}


// =============================================================================
// Test 5 (BLACKBOX): Holds reads the bridge file and re-reads after the interval.
// =============================================================================
void test_holds_parsing() {
	std::printf("Test: Holds parsing\n");
	const char* path = "/tmp/vecengine_holds_test.json";
	{ std::ofstream f(path); f << "{\"held\": [\"N101\", \"N104\"], \"updated\": \"x\"}"; }
	Holds holds(path);
	holds.poll(0.0);
	check(holds.isHeld("N101") && holds.isHeld("N104") && !holds.isHeld("N103"), "parses held tails");

	{ std::ofstream f(path); f << "{\"held\": []}"; }
	holds.poll(1.0);
	check(holds.isHeld("N101"), "does not re-read before the interval");
	holds.poll(3.0);
	check(!holds.isHeld("N101"), "re-reads after the interval and clears");

	Holds missing("/tmp/does_not_exist_vecengine.json");
	missing.poll(0.0);
	check(!missing.isHeld("N101"), "missing file means nothing held");
}


// =============================================================================
// Test 6: OBJ parser handles quads, missing normals, and normalises size/centre.
// =============================================================================
void test_obj_parser() {
	std::printf("Test: OBJ parser\n");
	const char* path = "/tmp/vecengine_test.obj";
	{ std::ofstream f(path); f << "# box 2 wide, 1 tall, 4 long, offset from origin\n"
	    "v 10 0 0\nv 12 0 0\nv 12 1 0\nv 10 1 0\nv 10 0 4\nv 12 0 4\nv 12 1 4\nv 10 1 4\n"
	    "f 1 2 3 4\nf 5 6 7 8\nf -8 -7 -3 -4\n"; }
	std::vector<Vertex> v; std::vector<unsigned int> idx;
	check(parseObj(path, 5.6f, 0.0f, v, idx), "parses a file");
	check(idx.size() == 3 * 2 * 3, "three quads fan into six triangles");
	float minX = 1e9f, maxX = -1e9f, cx = 0.0f;
	for (const auto& vert : v) { minX = std::min(minX, vert.position.x); maxX = std::max(maxX, vert.position.x); cx += vert.position.x; }
	check(nearly(maxX - minX, 5.6f, 1e-3f), "scaled to the target width");
	check(nearly(cx / (float)v.size(), 0.0f, 1e-3f), "recentred on the origin");
	check(nearly(std::fabs(v[0].normal.z), 1.0f, 1e-3f), "flat normal generated for a face without vn");
	check(!parseObj("/tmp/does_not_exist.obj", 1.0f, 0.0f, v, idx), "missing file returns false");
}

// ---------------------------------------------------------------------------
// FlightController ground model (the BLACKBOX app's flat airfield settings)
// ---------------------------------------------------------------------------
FlightController groundedPlane(Camera& cam, float groundY) {
	FlightController c(cam, {0.0f, groundY, 0.0f});
	c.mouseTrim = false;
	c.minSpeed = 0.0f; c.maxSpeed = 150.0f; c.speedSmoothing = 0.5f; c.gravityGain = 120.0f;
	c.maxPitch = 25.0f; c.maxRoll = 60.0f; c.stallSpeed = 40.0f;
	c.groundY = groundY;
	c.velocity = glm::vec3(0.0f);
	c.throttle = 0.0f;
	return c;
}

void test_ground_model() {
	std::printf("[ground model]\n");
	const float G = 31.2f;
	Camera cam({0.0f, G, 0.0f}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f);

	// Idle on the ground with speed: rolls to a stop, never leaves the surface.
	{
		FlightController c = groundedPlane(cam, G);
		c.velocity = c.forward() * 30.0f;
		for (int i = 0; i < 600; ++i) c.step(1.0f / 60.0f, {});
		check(c.speed() < 0.01f, "idle on the ground rolls to a full stop within 10 s");
		check(c.onGround() && nearly(c.position.y, G, 1e-3f), "stays on the surface while rolling");
	}
	{
		// Full throttle from rest: rotates by itself and gets airborne.
		FlightController c = groundedPlane(cam, G);
		c.throttle = 0.8f;
		for (int i = 0; i < 600; ++i) c.step(1.0f / 60.0f, {});
		check(!c.onGround() && c.position.y > G + 5.0f, "throttle up from rest takes off within 10 s");
	}
	{
	// Banking hard just above the ground: the wingtip never goes below the surface.
	FlightController c = groundedPlane(cam, G);
	c.position.y = G + 0.5f; c.throttle = 0.8f; c.velocity = c.forward() * 100.0f;
	FlightInput bank; bank.rollAxis = 1.0f; bank.pitchAxis = -1.0f;  // roll right, push nose down
	float minTip = 1e9f;
	for (int i = 0; i < 300; ++i) {
		c.step(1.0f / 60.0f, bank);
		float tip = c.position.y - c.halfSpan * std::fabs(std::sin(glm::radians(c.roll)));
		minTip = std::min(minTip, tip);
	}
	check(minTip >= G - c.gearHeight - 1e-3f, "wingtip stays above the surface while banking low");
	}
	{
		// Airborne at idle: speed decays below stall and the nose drops (no invisible speed floor).
		FlightController c = groundedPlane(cam, G);
		c.position.y = G + 500.0f; c.velocity = c.forward() * 100.0f;
		for (int i = 0; i < 900; ++i) c.step(1.0f / 60.0f, {});
		check(c.speed() < 100.0f && c.pitch < 0.0f, "idle in the air bleeds speed and the nose drops");
	}
}

int main() {
	std::printf("Running vecEngine unit tests...\n\n");

	test_heightAt_deterministic();
	test_heightAt_varies();
	test_frustum_culling();
	test_camera_basis_vectors();
	test_holds_parsing();
	test_obj_parser();
	test_ground_model();

	std::printf("\n%d/%d checks passed\n", totalChecks - failedChecks, totalChecks);
	return failedChecks == 0 ? 0 : 1;
}
