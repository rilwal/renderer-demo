
#include "texture.hpp"

#include <glad/gl.h>
#include <cstdio>

#include "util.hpp"

Texture::Texture() {
	glGenTextures(1, &gl_id);
	printf("Generated a texture: %d\n", gl_id);
}

Texture::~Texture() {
	printf("Deleting texture %d\n", gl_id);
	glDeleteTextures(1, &gl_id);
}


Texture2D::Texture2D() : IAsset("Texture2D") {

}

void Texture2D::create_empty(int width, int height, int channels, uint32_t gl_pixel_format) {
	glBindTexture(GL_TEXTURE_2D, get_id());
	glTexImage2D(GL_TEXTURE_2D, 0, gl_pixel_format, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

}


void Texture2D::load_from_file(const char* filename) {
	assert(false);
}

void Texture2D::reload() {
	assert(false);
}

void Texture2D::unload() {
	assert(false);
}

// TODO: Move to own file
Framebuffer::Framebuffer(uint32_t width, uint32_t height) : width(width), height(height) {

	glGenFramebuffers(1, &gl_id);

	// TODO: TEMPORARY, ALLOW USER TO DEFINE PARAMETERS
	glBindFramebuffer(GL_FRAMEBUFFER, gl_id);

	// Make a Texture2D
	color.create_empty(width, height, 3, GL_RGBA16F);

	// And a renderbuffer with depth and stencil
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color.get_id(), 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	GL_ERROR_CHECK();


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

Framebuffer::~Framebuffer() {
	glDeleteFramebuffers(1, &gl_id);
}