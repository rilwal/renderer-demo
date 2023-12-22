
#include "image.hpp"

#include "image/dds.hpp"
#include "image/png.hpp"


std::unique_ptr<Image> load_image(std::vector<uint8_t> buffer, std::string id) {
	if (is_dds(buffer)) {
		return load_dds(buffer, id);
	}
	else if (is_png(buffer)) {
		return load_png(buffer, id);
	}
	else {
		std::cerr << std::format("Unknown image format for image {}", id);
		assert(false);
	}
}


std::unique_ptr<Image> load_image(std::filesystem::path path) {
	return load_image(load_file(path), path.string());
}
