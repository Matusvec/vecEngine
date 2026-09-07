#include "game/plane_model.h"
#include "renderer/obj_loader.h"

#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace {

// Append an axis-aligned box with proper face normals into the running
// vertex/index buffers.
void appendBox(std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
               const glm::vec3& center, const glm::vec3& halfSize) {
	glm::vec3 c[8] = {
		center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),
		center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z),
		center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z),
		center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z),
		center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z),
		center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z),
		center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z),
		center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z),
	};

	struct Face { int a, b, c, d; glm::vec3 n; };
	const Face faces[6] = {
		{0, 1, 2, 3, { 0,  0, -1}},
		{5, 4, 7, 6, { 0,  0,  1}},
		{4, 0, 3, 7, {-1,  0,  0}},
		{1, 5, 6, 2, { 1,  0,  0}},
		{4, 5, 1, 0, { 0, -1,  0}},
		{3, 2, 6, 7, { 0,  1,  0}},
	};

	for (const Face& f : faces) {
		unsigned int base = (unsigned int)verts.size();
		verts.push_back({c[f.a], f.n, {0.0f, 0.0f}});
		verts.push_back({c[f.b], f.n, {1.0f, 0.0f}});
		verts.push_back({c[f.c], f.n, {1.0f, 1.0f}});
		verts.push_back({c[f.d], f.n, {0.0f, 1.0f}});
		idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
		idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
	}
}

Mesh buildBodyMesh() {
	std::vector<Vertex> v;
	std::vector<unsigned int> i;
	// Fuselage — main body. Nose at -Z (front), tail at +Z.
	appendBox(v, i, {0.0f,  0.00f,  0.00f}, {0.45f, 0.45f, 2.6f});
	// Wings — wide and thin.
	appendBox(v, i, {0.0f,  0.00f,  0.00f}, {2.8f,  0.09f, 0.85f});
	// Horizontal stabilizer (without elevator — that's a separate animated part).
	appendBox(v, i, {0.0f,  0.15f,  2.00f}, {1.05f, 0.07f, 0.30f});
	// Vertical fin / rudder.
	appendBox(v, i, {0.0f,  0.55f,  2.00f}, {0.06f, 0.55f, 0.45f});
	return Mesh(v, i);
}

Mesh buildCanopyMesh() {
	std::vector<Vertex> v;
	std::vector<unsigned int> i;
	appendBox(v, i, {0.0f, 0.55f, -0.60f}, {0.35f, 0.30f, 0.65f});
	return Mesh(v, i);
}

Mesh buildPropellerMesh() {
	std::vector<Vertex> v;
	std::vector<unsigned int> i;
	// Hub — small block. Two crossed blades, long along X and Y.
	// Centered at origin; transformed to the nose at draw time.
	appendBox(v, i, {0.0f, 0.0f, 0.0f}, {0.14f, 0.14f, 0.07f});
	appendBox(v, i, {0.0f, 0.0f, 0.0f}, {0.95f, 0.07f, 0.03f});  // horizontal blade
	appendBox(v, i, {0.0f, 0.0f, 0.0f}, {0.07f, 0.95f, 0.03f});  // vertical blade
	return Mesh(v, i);
}

Mesh buildAileronMesh() {
	std::vector<Vertex> v;
	std::vector<unsigned int> i;
	// Hinge at z = 0 (front edge of aileron); trailing edge at z = 0.25.
	// X centered around 0 — span of 1.0 unit (placed per-side at draw time).
	appendBox(v, i, {0.0f, 0.0f, 0.125f}, {0.50f, 0.05f, 0.125f});
	return Mesh(v, i);
}

Mesh buildElevatorMesh() {
	std::vector<Vertex> v;
	std::vector<unsigned int> i;
	// Same hinge convention as aileron — at z = 0, trailing edge at z = 0.20.
	// Wider span across the tail.
	appendBox(v, i, {0.0f, 0.0f, 0.10f}, {0.95f, 0.05f, 0.10f});
	return Mesh(v, i);
}

// Plane-orientation matrix matching the camera convention.
//   yaw  = -90  → nose along -Z (mesh default)
//   yaw  =   0  → nose along +X
glm::mat4 orientPlane(const glm::vec3& position, float yawDeg, float pitchDeg, float rollDeg) {
	glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
	m = glm::rotate(m, glm::radians(-(yawDeg + 90.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
	m = glm::rotate(m, glm::radians(pitchDeg),          glm::vec3(1.0f, 0.0f, 0.0f));
	m = glm::rotate(m, glm::radians(rollDeg),           glm::vec3(0.0f, 0.0f, -1.0f));
	return m;
}

}  // namespace

namespace {
// Texture named by map_Kd in the OBJ's sibling .mtl, resolved next to the OBJ. ponytail: first material only.
std::unique_ptr<Texture> materialTexture(const std::string& objPath) {
	std::string dir = objPath.substr(0, objPath.find_last_of('/') + 1);
	std::ifstream mtl(objPath.substr(0, objPath.find_last_of('.')) + ".mtl");
	std::string line, key, value;
	while (std::getline(mtl, line)) {
		std::istringstream s(line);
		if (s >> key >> value && key == "map_Kd") return std::make_unique<Texture>(dir + value);
	}
	return nullptr;
}
}  // namespace

PlaneModel::PlaneModel(const std::string& objPath, float yawDeg, const std::string& propellerPath)
	: bodyMesh(buildBodyMesh()),
	  canopyMesh(buildCanopyMesh()),
	  propellerMesh(buildPropellerMesh()),
	  aileronMesh(buildAileronMesh()),
	  elevatorMesh(buildElevatorMesh()),
	  shader("shaders/plane.vert", "shaders/plane.frag") {
	std::vector<Vertex> verts;
	std::vector<unsigned int> idx;
	if (!objPath.empty() && parseObj(objPath, /*targetWidth=*/5.6f, yawDeg, verts, idx)) {
		bodyMesh = Mesh(verts, idx);
		customModel = true;
		albedo = materialTexture(objPath);
		// Nose = the front-most sliver of the fitted body (it faces -Z after the turn).
		float minZ = 1e9f, maxZ = -1e9f;
		for (const auto& v : verts) { minZ = std::min(minZ, v.position.z); maxZ = std::max(maxZ, v.position.z); }
		float sumY = 0.0f; int n = 0;
		for (const auto& v : verts) if (v.position.z < minZ + 0.04f * (maxZ - minZ)) { sumY += v.position.y; ++n; }
		nose = {0.0f, n ? sumY / n : 0.0f, minZ + 0.05f};
	}
	if (!propellerPath.empty() && parseObj(propellerPath, /*targetWidth=*/1.6f, 0.0f, verts, idx)) {
		// Stand the disc up: the axis with the smallest extent becomes Z, the spin axis.
		glm::vec3 lo = verts[0].position, hi = verts[0].position;
		for (const auto& v : verts) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
		glm::vec3 ext = hi - lo;
		glm::mat3 up(1.0f);
		if (ext.y <= ext.x && ext.y <= ext.z) up = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0)));
		else if (ext.x <= ext.y && ext.x <= ext.z) up = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0)));
		for (auto& v : verts) { v.position = up * v.position; v.normal = glm::normalize(up * v.normal); }
		// Re-fit so the disc diameter (now in X) is the target and the hub sits at the origin.
		lo = hi = verts[0].position;
		for (const auto& v : verts) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
		float scale = 1.6f / std::max(hi.x - lo.x, 1e-4f);
		glm::vec3 center = (lo + hi) * 0.5f;
		for (auto& v : verts) v.position = (v.position - center) * scale;
		propellerMesh = Mesh(verts, idx);
		customPropeller = true;
		propellerAlbedo = materialTexture(propellerPath);
	}
	shader.use();
	shader.setInt("albedo", 0);
}

void PlaneModel::drawDepth(const Shader& depthShader, const glm::vec3& position,
                           float yawDeg, float pitchDeg, float rollDeg) const {
	depthShader.setMat4("model", orientPlane(position, yawDeg, pitchDeg, rollDeg));
	bodyMesh.draw();
}

void PlaneModel::draw(const glm::mat4& view, const glm::mat4& projection,
                      const glm::vec3& position,
                      float yawDeg, float pitchDeg, float rollDeg,
                      float time, float throttle,
                      float pitchInputN, float rollInputN,
                      const glm::vec3& tint) const {
	glm::mat4 planeM = orientPlane(position, yawDeg, pitchDeg, rollDeg);

	shader.use();
	shader.setVec3("tint", tint);
	shader.setInt("useTexture", albedo ? 1 : 0);
	if (albedo) albedo->bind(0);
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);

	// Body — static under the plane's orientation.
	shader.setMat4("model", planeM);
	bodyMesh.draw();
	if (!customModel) {
		// Canopy: dark tinted glass, then back to the body tint for the moving parts.
		shader.setVec3("tint", tint * glm::vec3(0.18f, 0.22f, 0.28f));
		canopyMesh.draw();
		shader.setVec3("tint", tint);
	}

	// Propeller at the nose, spinning around the plane's forward axis. Stopped when the engine is off.
	float spinRate = throttle > 0.01f ? 22.0f + throttle * 50.0f : 0.0f;  // rad/sec → up to ~11 rev/sec
	glm::mat4 propM = planeM
		* glm::translate(glm::mat4(1.0f), nose)
		* glm::rotate(glm::mat4(1.0f), time * spinRate, glm::vec3(0.0f, 0.0f, 1.0f));
	shader.setMat4("model", propM);
	shader.setInt("useTexture", propellerAlbedo ? 1 : 0);
	if (propellerAlbedo) propellerAlbedo->bind(0);
	propellerMesh.draw();
	if (customModel) return;  // a loaded body is one piece, no control surfaces
	shader.setInt("useTexture", 0);

	// Ailerons: deflect opposite to each other on roll input. Sign chosen so
	// banking right (rollInputN > 0) shows right aileron UP, left aileron DOWN
	// — like a real plane.
	const float aileronDefl = 22.0f;  // max deflection in degrees

	// Right aileron at (+X side of the wing, at trailing edge z=0.85).
	glm::mat4 rAilM = planeM
		* glm::translate(glm::mat4(1.0f), glm::vec3( 2.00f, 0.0f, 0.85f))
		* glm::rotate(glm::mat4(1.0f),
		              glm::radians(-rollInputN * aileronDefl),
		              glm::vec3(1.0f, 0.0f, 0.0f));
	shader.setMat4("model", rAilM);
	aileronMesh.draw();

	// Left aileron — opposite sign so the two move in anti-phase.
	glm::mat4 lAilM = planeM
		* glm::translate(glm::mat4(1.0f), glm::vec3(-2.00f, 0.0f, 0.85f))
		* glm::rotate(glm::mat4(1.0f),
		              glm::radians( rollInputN * aileronDefl),
		              glm::vec3(1.0f, 0.0f, 0.0f));
	shader.setMat4("model", lAilM);
	aileronMesh.draw();

	// Elevator at the trailing edge of the horizontal stabilizer.
	// Pull-up (pitchInputN > 0 = climb) → elevator deflects UP.
	const float elevDefl = 22.0f;
	glm::mat4 elevM = planeM
		* glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.15f, 2.30f))
		* glm::rotate(glm::mat4(1.0f),
		              glm::radians(-pitchInputN * elevDefl),
		              glm::vec3(1.0f, 0.0f, 0.0f));
	shader.setMat4("model", elevM);
	elevatorMesh.draw();
}
