#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"

#include <glm/glm.hpp>

class Skybox {
public:
	Skybox();

	// Renders the procedural sky. Strips translation from view internally so the sky
	// stays anchored to the camera. Disables depth writes so the world overdraws cleanly.
	// Caller still owns whatever shader they want to use afterwards (this rebinds its own).
	// time drives cloud drift; sunDir places the bright cloud lobe + sun glow.
	void draw(const glm::mat4& view, const glm::mat4& projection,
	          float time, const glm::vec3& sunDir) const;

private:
	Mesh cube;
	Shader shader;
};
