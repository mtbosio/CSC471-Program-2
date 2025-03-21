#include "Texture.h"
#include "GLSL.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

Texture::Texture() :
	filename(""),
	tid(0)
{
	
}

Texture::~Texture()
{
	
}

void Texture::init()
{
	// Load texture
	int w, h, ncomps;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename.c_str(), &w, &h, &ncomps, 0);
	if(!data) {
		cerr << filename << " not found" << endl;
	}
	if (ncomps != 3 && ncomps != 4) {
		cerr << filename << " must have 3 or 4 components (RGB or RGBA)" << endl;
	}
	if((w & (w - 1)) != 0 || (h & (h - 1)) != 0) {
		cerr << filename << " must be a power of 2" << endl;
	}
	width = w;
	height = h;

	// Generate a texture buffer object
	glGenTextures(1, &tid);
	// Bind the current texture to be the newly generated texture object
	glBindTexture(GL_TEXTURE_2D, tid);
	// Load the actual texture data
	// Base level is 0, number of channels is 3, and border is 0.
	GLenum format = (ncomps == 4) ? GL_RGBA : GL_RGB;
	GLenum internalFormat = (ncomps == 4) ? GL_SRGB_ALPHA : GL_RGB;
	
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	
	// Generate image pyramid
	glGenerateMipmap(GL_TEXTURE_2D);
	// Set texture wrap modes for the S and T directions
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Set filtering mode for magnification and minimification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// Unbind
	glBindTexture(GL_TEXTURE_2D, 0);
	// Free image, since the data is now on the GPU
	stbi_image_free(data);
}

void Texture::setWrapModes(GLint wrapS, GLint wrapT)
{
	// Must be called after init()
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}

void Texture::bind(GLint handle)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, tid);
	glUniform1i(handle, unit);
}

void Texture::unbind()
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::loadCubeMap(const std::vector<std::string>& faces) {
	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tid);

	int width, height, nrChannels;
	
	for (unsigned int i = 0; i < faces.size(); i++) {
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			flipImageVertically(data, width, height, nrChannels); // Flip the texture
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		} else {
			std::cerr << "Failed to load cubemap texture: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Texture::flipImageVertically(unsigned char* image, int width, int height, int channels) {
    int rowSize = width * channels;  
    unsigned char* tempRow = new unsigned char[rowSize];  

    for (int i = 0; i < height / 2; i++) {
        unsigned char* row1 = image + i * rowSize;
        unsigned char* row2 = image + (height - i - 1) * rowSize;
        memcpy(tempRow, row1, rowSize);
        memcpy(row1, row2, rowSize);
        memcpy(row2, tempRow, rowSize);
    }

    delete[] tempRow;
}