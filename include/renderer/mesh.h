#pragma once

#include "renderer/vertex.h"
#include <vector>

class Mesh {
public:
	Mesh() = default;  // empty mesh — useful as a placeholder before real data lands
	Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void draw() const;

	static Mesh createCube();

private:
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int indexCount = 0;
};
