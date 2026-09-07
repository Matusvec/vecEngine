#include "renderer/texture.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

Texture::Texture(const std::string& path) {
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

	if (!data) {
		std::cerr << "Failed to load texture '" << path << "': "
		          << (stbi_failure_reason() ? stbi_failure_reason() : "unknown error")
		          << std::endl;
		// Fall back to 1x1 magenta so missing textures are visually obvious in-game.
		const unsigned char magenta[3] = {255, 0, 255};
		*this = Texture(1, 1, 3, magenta);
		return;
	}

	*this = Texture(width, height, channels, data);
	stbi_image_free(data);
}

Texture::Texture(int width, int height, int channels, const unsigned char* data) {
	unsigned int format = (channels == 4) ? GL_RGBA : GL_RGB;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture() {
	glDeleteTextures(1, &textureID);
}

Texture::Texture(Texture&& other) noexcept : textureID(other.textureID) {
	other.textureID = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {
		glDeleteTextures(1, &textureID);
		textureID = other.textureID;
		other.textureID = 0;
	}
	return *this;
}

void Texture::bind(int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, textureID);
}
