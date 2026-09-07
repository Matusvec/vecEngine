#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

// Skeleton plane mesh drawn in third-person view. Composed of separate parts:
//   * static body (fuselage + wings + horizontal stab + vertical fin + cockpit)
//   * spinning propeller at the nose
//   * left + right ailerons (deflect on roll input)
//   * elevator at the tail (deflects on pitch input)
// Mesh is in local coords with nose pointing along -Z, wings along ±X.
class PlaneModel {
public:
	PlaneModel();

	void draw(const glm::mat4& view, const glm::mat4& projection,
	          const glm::vec3& position,
	          float yawDeg, float pitchDeg, float rollDeg,
	          float time, float throttle,
	          float pitchInputN, float rollInputN) const;

private:
	Mesh bodyMesh;
	Mesh propellerMesh;
	Mesh aileronMesh;   // shared by left + right (different transforms)
	Mesh elevatorMesh;
	Shader shader;
};
