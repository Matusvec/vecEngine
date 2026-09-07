#pragma once

#include "renderer/shader.h"

#include <glm/glm.hpp>

class Rain {
public:
	// dropCount  — total drop instances (one draw call instanced)
	// boxRadius  — drops scattered within ±boxRadius in XZ around the camera
	// boxHeight  — vertical extent of the rain volume (also wrap-around length)
	Rain(int dropCount, float boxRadius);
	~Rain();

	Rain(const Rain&) = delete;
	Rain& operator=(const Rain&) = delete;

	void draw(const glm::mat4& view, const glm::mat4& projection,
	          const glm::vec3& cameraPos, float time) const;

private:
	Shader shader;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int instanceVBO = 0;
	int dropCount;
};
