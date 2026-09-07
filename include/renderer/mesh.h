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

	unsigned int indexCount = 0;  // 0 means empty (e.g. a model file that failed to load)

private:
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
};
