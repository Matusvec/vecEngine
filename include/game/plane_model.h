#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <string>

// Skeleton plane mesh drawn in third-person view. Composed of separate parts:
//   * static body (fuselage + wings + horizontal stab + vertical fin + cockpit)
//   * spinning propeller at the nose
//   * left + right ailerons (deflect on roll input)
//   * elevator at the tail (deflects on pitch input)
// Mesh is in local coords with nose pointing along -Z, wings along ±X.
class PlaneModel {
public:
	// If objPath names a readable OBJ, the whole aircraft is that model (no animated parts);
	// otherwise the built-in box plane is used. yawDeg fixes models that do not face -Z.
	explicit PlaneModel(const std::string& objPath = "", float yawDeg = 0.0f);

	// Depth-only draw for the shadow pass (body only, that is what casts the shadow).
	void drawDepth(const Shader& depthShader, const glm::vec3& position,
	               float yawDeg, float pitchDeg, float rollDeg) const;

	void draw(const glm::mat4& view, const glm::mat4& projection,
	          const glm::vec3& position,
	          float yawDeg, float pitchDeg, float rollDeg,
	          float time, float throttle,
	          float pitchInputN, float rollInputN,
	          const glm::vec3& tint = glm::vec3(1.0f)) const;

private:
	Mesh bodyMesh;
	Mesh canopyMesh;
	Mesh propellerMesh;
	Mesh aileronMesh;   // shared by left + right (different transforms)
	Mesh elevatorMesh;
	Shader shader;
	bool customModel = false;
};
