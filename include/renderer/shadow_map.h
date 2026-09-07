#pragma once

#include <glm/glm.hpp>

// Single-cascade directional shadow map. Owns an FBO with a depth-only texture
// attachment. Caller renders shadow casters into beginPass()/endPass() with the
// depth-only shader, then samples the texture in their main-pass shaders.
class ShadowMap {
public:
	explicit ShadowMap(int resolution);
	~ShadowMap();

	ShadowMap(const ShadowMap&) = delete;
	ShadowMap& operator=(const ShadowMap&) = delete;

	// Bind the FBO + set viewport to the shadow map's size + clear depth.
	void beginPass();

	// Restore the default framebuffer + the screen viewport.
	void endPass(int screenWidth, int screenHeight);

	// Builds a stable orthographic light-space matrix tracking cameraPos. The
	// XZ anchor is snapped to a shadow-texel boundary so shadows don't shimmer
	// when the camera moves smoothly. lightDir points TOWARD the sun.
	glm::mat4 computeLightSpaceMatrix(const glm::vec3& cameraPos,
	                                  const glm::vec3& lightDir) const;

	// Bind the depth texture so a fragment shader can sample it as `shadowMap`.
	void bindShadowTexture(unsigned int unit) const;

	int resolution() const { return res; }

private:
	unsigned int FBO = 0;
	unsigned int depthTexture = 0;
	int res;
};
