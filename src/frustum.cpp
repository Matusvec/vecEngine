#include "renderer/frustum.h"

#include <cmath>

// Gribb-Hartmann plane extraction in clip space:
//   left   = row3 + row0     right = row3 - row0
//   bottom = row3 + row1     top   = row3 - row1
//   near   = row3 + row2     far   = row3 - row2
// glm matrices are column-major, so we transpose once and read columns as rows.
void Frustum::update(const glm::mat4& vp) {
	glm::mat4 m = glm::transpose(vp);

	planes[0] = m[3] + m[0];
	planes[1] = m[3] - m[0];
	planes[2] = m[3] + m[1];
	planes[3] = m[3] - m[1];
	planes[4] = m[3] + m[2];
	planes[5] = m[3] - m[2];

	for (int i = 0; i < 6; ++i) {
		float length = std::sqrt(
			planes[i].x * planes[i].x +
			planes[i].y * planes[i].y +
			planes[i].z * planes[i].z);
		if (length > 0.0f) planes[i] /= length;
	}
}

bool Frustum::containsSphere(const glm::vec3& center, float radius) const {
	for (int i = 0; i < 6; ++i) {
		float distance = planes[i].x * center.x
		               + planes[i].y * center.y
		               + planes[i].z * center.z
		               + planes[i].w;
		if (distance < -radius) return false;
	}
	return true;
}
