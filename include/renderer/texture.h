#pragma once

#include <string>

class Texture {
public:
	// Upload tightly-packed pixel data directly to the GPU. data layout is row-major
	// from top-left, RGB (channels=3) or RGBA (channels=4). Mipmaps are generated.
	Texture(int width, int height, int channels, const unsigned char* data);

	// Load a PNG / JPG / TGA / BMP from disk via stb_image. Image is flipped
	// vertically on load so UV (0,0) maps to the bottom-left, matching OpenGL's
	// texture-coordinate convention. Falls back to a 1×1 magenta texture if the
	// file is missing — visually obvious in-game.
	explicit Texture(const std::string& path);

	~Texture();

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	void bind(int unit = 0) const;

	unsigned int id() const { return textureID; }

private:
	unsigned int textureID = 0;
};
