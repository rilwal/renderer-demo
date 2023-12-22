#pragma once


#include "asset_manager.hpp"
#include "glad/gl.h"

// TODO: * Expand this to all the formats we can reasonably support
//		 * Decouple storage format from internal format?
enum class TextureFormat {
	R8,
	RG8,
	RGB8,
	RGBA8,
	BC7_UNORM
};

constexpr uint32_t get_gl_format(TextureFormat f) {
	using enum TextureFormat;

	switch (f) {
	case R8: return GL_RED;
	case RG8: return GL_RG;
	case RGB8:	return GL_RGB;
	case RGBA8:	return GL_RGBA;
	case BC7_UNORM: return GL_COMPRESSED_RGBA_BPTC_UNORM;
	}

	assert(false);
	return GL_INVALID_ENUM;
}


constexpr bool is_compressed(TextureFormat f) {
	using enum TextureFormat;
	switch (f) {
	case R8:
	case RG8:
	case RGB8:
	case RGBA8:	return false;
	case BC7_UNORM: return true;
	}

	assert(false);
	return false;
}


// Get the texture format appropriate for an image of n 8 bit channels
constexpr TextureFormat texture_format_from_channels(uint32_t n) {
	switch (n) {
	case 1: return TextureFormat::R8;
	case 2: return TextureFormat::RG8;
	case 3: return TextureFormat::RGB8;
	case 4: return TextureFormat::RGBA8;
	}
}


struct Image {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t channels = 0;
	TextureFormat format = TextureFormat::RGBA8;
	std::vector<uint8_t> data = {};

	~Image() {};
};


std::unique_ptr<Image> load_image(std::vector<uint8_t> buffer, std::string id = "");
std::unique_ptr<Image> load_image(std::filesystem::path path);
