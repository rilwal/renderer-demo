#pragma once

#include <cinttypes>

#include "asset_manager.hpp"
#include "types.hpp"

class Texture {
public:
	Texture();
	~Texture();

	inline uint32_t get_id() { return gl_id; }

private:
	uint32_t gl_id;
};


class Texture2D : public Texture, IAsset {
public:
	Texture2D();

	void create_empty (int width, int height, int channels, uint32_t gl_pixel_format);
	void load_from_file(const char* filename) override;
	void reload() override;
	void unload() override;


private:
	uint32_t gl_pixel_format;

	int width;
	int height;
	int channels;
};

class Framebuffer {
public:
	Framebuffer(uint32_t width=1920, uint32_t height=1080);
	~Framebuffer();

	inline uint32_t get_id() { return gl_id; }

	inline uint32_t get_width() { return width; }
	inline uint32_t get_height() { return height; }

private:
	Texture2D color;
	uint32_t gl_id;

	uint32_t width, height;
};