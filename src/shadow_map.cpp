#include "renderer/shadow_map.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {
// World-space half-extent of the orthographic light frustum. Should at least
// cover the visible terrain ring around the player; bigger = softer/coarser
// shadows, smaller = sharper but tighter coverage.
constexpr float ORTHO_EXTENT = 320.0f;

// Distance from the anchor point to where we place the light "camera". Must
// exceed the tallest expected geometry above the anchor so casters near the
// near plane aren't clipped (terrain peaks ~ +330).
constexpr float LIGHT_DISTANCE = 600.0f;
constexpr float NEAR_PLANE     = 1.0f;
constexpr float FAR_PLANE      = 1300.0f;
}

ShadowMap::ShadowMap(int resolution) : res(resolution) {
	glGenFramebuffers(1, &FBO);
	glGenTextures(1, &depthTexture);

	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
	             res, res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// Border = 1.0 (max depth) so fragments outside the shadow frustum read as
	// "lit" rather than "shadowed".
	float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	                       GL_TEXTURE_2D, depthTexture, 0);
	// Depth-only FBO — no color attachment.
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap() {
	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &depthTexture);
}

void ShadowMap::beginPass() {
	glViewport(0, 0, res, res);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::endPass(int screenWidth, int screenHeight) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenWidth, screenHeight);
}

glm::mat4 ShadowMap::computeLightSpaceMatrix(const glm::vec3& cameraPos,
                                             const glm::vec3& lightDir) const {
	// Snap the anchor XZ to whole-texel units so single-texel changes happen
	// only when the camera crosses a texel boundary — kills shadow-edge crawl.
	float texelSize = (ORTHO_EXTENT * 2.0f) / (float)res;
	glm::vec3 anchor(
		std::floor(cameraPos.x / texelSize) * texelSize,
		0.0f,
		std::floor(cameraPos.z / texelSize) * texelSize);

	glm::vec3 lightPos = anchor + glm::normalize(lightDir) * LIGHT_DISTANCE;
	glm::mat4 lightView = glm::lookAt(lightPos, anchor, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightProj = glm::ortho(-ORTHO_EXTENT, ORTHO_EXTENT,
	                                 -ORTHO_EXTENT, ORTHO_EXTENT,
	                                 NEAR_PLANE, FAR_PLANE);
	return lightProj * lightView;
}

void ShadowMap::bindShadowTexture(unsigned int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
}
