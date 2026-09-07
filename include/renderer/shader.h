#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
	Shader(const std::string& vertexPath, const std::string& fragmentPath);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&& other) noexcept;
	Shader& operator=(Shader&& other) noexcept;

	void use() const;

	void setMat4(const std::string& name, const glm::mat4& value) const;
	void setMat3(const std::string& name, const glm::mat3& value) const;
	void setVec3(const std::string& name, const glm::vec3& value) const;
	void setVec2(const std::string& name, const glm::vec2& value) const;
	void setFloat(const std::string& name, float value) const;
	void setInt(const std::string& name, int value) const;

	unsigned int id() const { return programID; }

private:
	int getUniformLocation(const std::string& name) const;

	unsigned int programID = 0;
	mutable std::unordered_map<std::string, int> uniformCache;
};
