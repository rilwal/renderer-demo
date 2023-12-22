
#include "dds.hpp"

#include "../image.hpp"


bool is_dds(std::vector<uint8_t> buffer) {
	uint32_t magic = *(uint32_t*)buffer.data();
	return magic == g_dds_magic;
}

// ID is to be used in error output when the data comes from a buffer,
// to help use identify what files etc. are causing issues.
std::unique_ptr<Image> load_dds(std::vector<uint8_t> buffer, std::string id) {
	auto image = std::make_unique<Image>();
	uint32_t magic = *(uint32_t*)buffer.data();
	if (magic != g_dds_magic) {
		fprintf(stderr, "Invalid DDS Magic in %s!\n", id.c_str());
		return image;
	}

	DDS_Header header = *(DDS_Header*)&buffer[sizeof(magic)];
	DDS_Header_DX10 header_dx10 = {};

	if (header.pixel_format.flags[DDS_PixelFormatFlags::Fourcc]) {
		if (header.pixel_format.four_cc == DDS_FourCC::DX10) {
			header_dx10 = *(DDS_Header_DX10*)&buffer[sizeof(magic) + sizeof(DDS_Header)];
		}
	}

	if (header_dx10.format != DDS_Format::BC7_UNORM) {
		fprintf(stderr, "Error Parsing DDS: %s!\nCurrently only BC7 UNORM format is supported\n", id.c_str());
	}

	image->width = header.width;
	image->height = header.height;
	image->channels = 4; // TODO: Figure this out based on the pixel format!!!
	image->format = TextureFormat::BC7_UNORM;

	// BC7 is one byte per pixel!
	size_t data_length = image->width * image->height * 1;
	image->data.resize(data_length);
	memcpy(image->data.data(), &buffer[sizeof(magic) + sizeof(DDS_Header) + sizeof(DDS_Header_DX10)], data_length);

	return image;
}

std::unique_ptr<Image> load_dds(std::filesystem::path path) {
	return load_dds(load_file(path), path.string());
}