#pragma once

#include <cstdint>
#include <glad/gl.h>

#include "gl/gl.hpp"

class Framebuffer {
public:
	Framebuffer(uint32_t width, uint32_t height) : m_width(width), m_height(height) {
		uint32_t color_attachment = gl::create_texture(GL_TEXTURE_2D);
		gl::texture_parameter(color_attachment, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl::texture_parameter(color_attachment, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl::texture_parameter(color_attachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		gl::texture_parameter(color_attachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		gl::bind_texture(GL_TEXTURE_2D, color_attachment);
		gl::texture_image(GL_TEXTURE_2D, 0, gl::TextureInternalFormat::RGBA16F, { m_width, m_height }, gl::TextureFormat::RGBA, gl::TextureBaseType::Float, 0);

		uint32_t depth_attachment = gl::create_texture(GL_TEXTURE_2D);
		gl::texture_parameter(depth_attachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl::texture_parameter(depth_attachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl::texture_parameter(depth_attachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl::texture_parameter(depth_attachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		gl::bind_texture(GL_TEXTURE_2D, depth_attachment);
		gl::texture_image(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, { m_width, m_height }, GL_DEPTH_COMPONENT, gl::TextureBaseType::Float, 0);

		glCreateFramebuffers(1, &m_gl_id);
		glNamedFramebufferTexture(m_gl_id, GL_COLOR_ATTACHMENT0, color_attachment, 0);
		glNamedFramebufferTexture(m_gl_id, GL_DEPTH_ATTACHMENT, depth_attachment, 0);
	}

	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_gl_id);
	}

	void clear_color(std::array<float, 4> color) {
		glClearNamedFramebufferfv(m_gl_id, GL_COLOR, 0, &color[0]);
	}

	void clear_depth() {
		float zero = 0;
		glClearNamedFramebufferfv(m_gl_id, GL_DEPTH, 0, &zero);
	}

	void unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

private:
	uint32_t m_gl_id = 0;

	uint32_t m_width, m_height;
};