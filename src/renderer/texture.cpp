
#include "texture.hpp"

#include <glad/gl.h>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#include "util.hpp"

Texture::Texture() {
}

Texture::~Texture() {

}


Texture2D::Texture2D() : IAsset("Texture2D") {

}

void Texture2D::create_empty(int width, int height, int channels, uint32_t gl_pixel_format) {
	allocate();

	glBindTexture(GL_TEXTURE_2D, m_gl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, gl_pixel_format, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

	m_width = width;
	m_height = height;
	m_channels = channels;
}


void Texture2D::load_from_file(const char* filename) {
	// TODO: Properly support different image formats
	std::vector<uint8_t> data = load_file(filename);
	m_channels = 4;

	int width = 0;
	int height = 0;

	m_color_data = (uint32_t*)stbi_load_from_memory(data.data(), static_cast<int32_t>(data.size()), &width, &height, 0, 4);
	
	m_width = static_cast<uint32_t>(width);
	m_height = static_cast<uint32_t>(height);
	//create_empty(m_width, m_height, m_channels, GL_RGB);
	
	allocate();
	upload_data();

}

void Texture2D::reload() {
	m_should_reload = true;
}

void Texture2D::unload() {
	if (m_gl_id) {
		stbi_image_free(m_color_data);

		glDeleteTextures(1, &m_gl_id);
		m_gl_id = 0;
	}
}

inline uint32_t Texture2D::get_id()
{
	if (m_should_reload) {
		stbi_image_free(m_color_data);
		load_from_file(path.c_str());
	}
	return m_gl_id;
}


void Texture2D::upload_data() {
	glBindTexture(GL_TEXTURE_2D, m_gl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, m_channels == 4 ? GL_RGBA : GL_RGB, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_color_data);
	auto error = glGetError();
	GL_ERROR_CHECK();

}

uint32_t Texture2D::get_pixel(glm::ivec2 pos) {
	return get_pixel(pos.x, pos.y);
}

uint32_t Texture2D::get_pixel(uint32_t x, uint32_t y) {
	if (x >= m_width || y >= m_height) return 0;

	return m_color_data[x + y * m_width];
}

void Texture2D::write_color(uint32_t x, uint32_t y, uint32_t color) {
	m_color_data[x + y * m_width] = color;
	upload_data();
}


void Texture2D::allocate() {
	if (!m_gl_id) {
		glCreateTextures(GL_TEXTURE_2D, 1, &m_gl_id);

		glBindTexture(GL_TEXTURE_2D, m_gl_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}


// TODO: Move to own file
Framebuffer::Framebuffer(uint32_t width, uint32_t height) : width(width), height(height) {

	glGenFramebuffers(1, &m_gl_id);

	// TODO: TEMPORARY, ALLOW USER TO DEFINE PARAMETERS
	glBindFramebuffer(GL_FRAMEBUFFER, m_gl_id);

	// Make a Texture2D
	color.create_empty(width, height, 4, GL_RGBA16F);

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
	glDeleteFramebuffers(1, &m_gl_id);
}

void show_texture_inspector() {
	if (ImGui::Begin("Texture Inspector")) {
		if (ImGui::BeginTable("Textures", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			constexpr float preview_max_size = 64.f;
			constexpr float large_preview_max_size = 512.f;

			ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, preview_max_size);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed);


			for (auto& texture_wr : asset_lib<Texture2D>) {
				if (auto texture = texture_wr.lock()) {
					ImGui::Tag id(&texture);

					float aspect_ratio = (float)texture->get_width() / texture->get_height();
					ImGui::TableNextRow();

					glm::vec2 preview_size = glm::vec2(preview_max_size);
					if (aspect_ratio < 1) {
						preview_size.x *= aspect_ratio;
					}
					else {
						preview_size.y /= aspect_ratio;
					}

					ImGui::TableSetColumnIndex(0);
					ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texture->get_id())), preview_size, glm::vec2(0, 0), glm::vec2(1, 1));

					if (ImGui::IsItemHovered()) {
						// Show a popup!
						if (ImGui::BeginTooltip()) {

							glm::vec2 preview_size = glm::vec2(large_preview_max_size);
							if (aspect_ratio < 1) {
								preview_size.x *= aspect_ratio;
							}
							else {
								preview_size.y /= aspect_ratio;
							}

							ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texture->get_id())), preview_size, glm::vec2(0, 0), glm::vec2(1, 1));

							ImGui::EndTooltip();
						}
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%s", texture->path.c_str());

				}
			}

			ImGui::EndTable();
		}
	}

	ImGui::End();
}
