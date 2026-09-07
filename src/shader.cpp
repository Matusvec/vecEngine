#include "renderer/shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

namespace {

std::string readFile(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open shader file: " << path << std::endl;
		return {};
	}
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

unsigned int compileShader(unsigned int type, const std::string& source, const std::string& path) {
	unsigned int shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "Shader compilation failed (" << path << "):\n" << infoLog << std::endl;
	}
	return shader;
}

}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
	std::string vertexSrc = readFile(vertexPath);
	std::string fragmentSrc = readFile(fragmentPath);

	unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc, vertexPath);
	unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc, fragmentPath);

	programID = glCreateProgram();
	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);
	glLinkProgram(programID);

	int success;
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(programID, 512, nullptr, infoLog);
		std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

Shader::~Shader() {
	glDeleteProgram(programID);
}

Shader::Shader(Shader&& other) noexcept
	: programID(other.programID), uniformCache(std::move(other.uniformCache)) {
	other.programID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
	if (this != &other) {
		glDeleteProgram(programID);
		programID = other.programID;
		uniformCache = std::move(other.uniformCache);
		other.programID = 0;
	}
	return *this;
}

void Shader::use() const {
	glUseProgram(programID);
}

// Caches glGetUniformLocation results — the lookup crosses into the driver and is one of
// the more expensive per-uniform-set calls in OpenGL. -1 (not found) is cached too so we
// don't keep re-querying nonexistent names.
int Shader::getUniformLocation(const std::string& name) const {
	auto it = uniformCache.find(name);
	if (it != uniformCache.end()) return it->second;
	int loc = glGetUniformLocation(programID, name.c_str());
	uniformCache.emplace(name, loc);
	return loc;
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
	glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const {
	glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
	glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
	glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setFloat(const std::string& name, float value) const {
	glUniform1f(getUniformLocation(name), value);
}

void Shader::setInt(const std::string& name, int value) const {
	glUniform1i(getUniformLocation(name), value);
}
