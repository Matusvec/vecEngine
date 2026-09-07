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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

int main() {
	std::printf("Running vecEngine unit tests...\n\n");

	test_heightAt_deterministic();
	test_heightAt_varies();
	test_frustum_culling();
	test_camera_basis_vectors();
	test_holds_parsing();

	std::printf("\n%d/%d checks passed\n", totalChecks - failedChecks, totalChecks);
	return failedChecks == 0 ? 0 : 1;
}
