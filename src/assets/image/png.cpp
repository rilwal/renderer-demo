
#include "png.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../image.hpp"

constexpr uint32_t g_png_magic = 0x89504e47;


bool is_png(std::vector<uint8_t> buffer) {
	return stbi_info_from_memory(buffer.data(), buffer.size(), nullptr, nullptr, nullptr);
}


// ID is to be used in error output when the data comes from a buffer,
// to help use identify what files etc. are causing issues.
std::unique_ptr<Image> load_png(std::vector<uint8_t> buffer, std::string id) {
	std::unique_ptr<Image> image = std::make_unique<Image>();
	
	int x = 0, y = 0, channels = 0;

	uint8_t* data = stbi_load_from_memory(buffer.data(), buffer.size(), &x, &y, &channels, 0);

	image->width = x;
	image->height = y;
	image->channels = channels;
	image->format = texture_format_from_channels(image->channels);

	size_t data_size = static_cast<size_t>(x) * static_cast<size_t>(y) * static_cast<size_t>(channels);
	// Assumes 8 bits per channel, which I think STBI will always uphold?
	image->data.resize(data_size);
	memcpy(image->data.data(), data, data_size);
	stbi_image_free(data);

	return image;
}

std::unique_ptr<Image> load_png(std::filesystem::path path) {
	return load_png(load_file(path), path.string());
}