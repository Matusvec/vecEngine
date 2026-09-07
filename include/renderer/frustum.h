#pragma once

#include <glm/glm.hpp>

class Frustum {
public:
	// Recompute the 6 frustum planes from a view-projection matrix.
	// Inward-facing normals: a point is inside the frustum iff it has positive
	// signed distance to every plane.
	void update(const glm::mat4& viewProjection);

	bool containsSphere(const glm::vec3& center, float radius) const;

private:
	glm::vec4 planes[6];  // 0=L, 1=R, 2=B, 3=T, 4=N, 5=F
};
