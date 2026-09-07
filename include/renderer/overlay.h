#pragma once

#include "renderer/shader.h"

#include <glm/glm.hpp>

// Fullscreen tinted quad — used for state-driven screen wash (start screen
// dim, crash red, win green). Renders in clip space, ignores depth, blends.
class Overlay {
public:
	Overlay();
	~Overlay();
	Overlay(const Overlay&) = delete;
	Overlay& operator=(const Overlay&) = delete;

	// Draws a fullscreen quad of `color` × `alpha` over the current framebuffer.
	void drawTinted(const glm::vec3& color, float alpha) const;

private:
	Shader shader;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
};
