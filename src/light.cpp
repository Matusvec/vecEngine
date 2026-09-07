#include "renderer/light.h"

#include "renderer/shader.h"

void LightingSystem::apply(const Shader& shader,
                           const glm::vec3& cameraPos,
                           const glm::mat4& lightSpaceMatrix,
                           int shadowMapUnit) const {
	shader.setVec3("sunDir",         sun.direction);
	shader.setVec3("sunColor",       sun.color);
	shader.setFloat("sunAmbient",    sun.ambient);
	shader.setFloat("sunSpecular",   sun.specularStrength);
	shader.setFloat("sunShininess",  sun.shininess);
	shader.setVec3("cameraPos",      cameraPos);
	shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
	shader.setInt("shadowMap",       shadowMapUnit);
}
