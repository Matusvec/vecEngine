#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

// Each face has 4 vertices with position + color (6 floats per vertex)
// 6 faces * 4 vertices = 24 vertices
float vertices[] = {
	// Front face (orange)
	-0.5f, -0.5f,  0.5f,  1.0f, 0.5f, 0.2f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.5f, 0.2f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.5f, 0.2f,
	-0.5f,  0.5f,  0.5f,  1.0f, 0.5f, 0.2f,

	// Back face (green)
	-0.5f, -0.5f, -0.5f,  0.2f, 0.8f, 0.3f,
	 0.5f, -0.5f, -0.5f,  0.2f, 0.8f, 0.3f,
	 0.5f,  0.5f, -0.5f,  0.2f, 0.8f, 0.3f,
	-0.5f,  0.5f, -0.5f,  0.2f, 0.8f, 0.3f,

	// Left face (blue)
	-0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 0.9f,
	-0.5f, -0.5f,  0.5f,  0.2f, 0.4f, 0.9f,
	-0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 0.9f,
	-0.5f,  0.5f, -0.5f,  0.2f, 0.4f, 0.9f,

	// Right face (yellow)
	 0.5f, -0.5f, -0.5f,  0.9f, 0.9f, 0.2f,
	 0.5f, -0.5f,  0.5f,  0.9f, 0.9f, 0.2f,
	 0.5f,  0.5f,  0.5f,  0.9f, 0.9f, 0.2f,
	 0.5f,  0.5f, -0.5f,  0.9f, 0.9f, 0.2f,

	// Top face (red)
	-0.5f,  0.5f,  0.5f,  0.9f, 0.2f, 0.2f,
	 0.5f,  0.5f,  0.5f,  0.9f, 0.2f, 0.2f,
	 0.5f,  0.5f, -0.5f,  0.9f, 0.2f, 0.2f,
	-0.5f,  0.5f, -0.5f,  0.9f, 0.2f, 0.2f,

	// Bottom face (purple)
	-0.5f, -0.5f,  0.5f,  0.6f, 0.2f, 0.8f,
	 0.5f, -0.5f,  0.5f,  0.6f, 0.2f, 0.8f,
	 0.5f, -0.5f, -0.5f,  0.6f, 0.2f, 0.8f,
	-0.5f, -0.5f, -0.5f,  0.6f, 0.2f, 0.8f,
};

unsigned int indices[] = {
	// Front
	 0,  1,  2,   2,  3,  0,
	// Back
	 4,  6,  5,   6,  4,  7,
	// Left
	 8,  9, 10,  10, 11,  8,
	// Right
	12, 14, 13,  14, 12, 15,
	// Top
	16, 17, 18,  18, 19, 16,
	// Bottom
	20, 22, 21,  22, 20, 23,
};

// Cube position — moved by arrow keys
glm::vec3 cubePos(0.0f, 0.0f, 0.0f);
float moveSpeed = 2.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float velocity = moveSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		cubePos.y += velocity;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		cubePos.y -= velocity;
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		cubePos.x -= velocity;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		cubePos.x += velocity;
}

unsigned int compileShader(unsigned int type, const char* source) {
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
	}
	return shader;
}

unsigned int createShaderProgram() {
	unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(2560, 1600, "vecEngine", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	unsigned int shaderProgram = createShaderProgram();

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	while (!glfwWindowShouldClose(window)) {
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		// Model: translate by arrow key position, then rotate over time
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, cubePos);
		model = glm::rotate(model, currentFrame, glm::vec3(0.5f, 1.0f, 0.0f));

		// View: camera pulled back
		glm::mat4 view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, 3.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		// Projection: perspective
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		float aspect = (height > 0) ? (float)width / (float)height : 1.0f;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);
	glfwTerminate();
	return 0;
}
