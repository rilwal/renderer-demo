#pragma once

#include <cinttypes>

#include "asset_manager.hpp"
#include "types.hpp"





class Texture {
public:
	Texture();
	~Texture();

	virtual uint32_t get_id() { return m_gl_id; }
	virtual void allocate() = 0;

protected:
	uint32_t m_gl_id = 0;
};




class Texture2D : public Texture, public IAsset {
public:
	Texture2D();

	void create_empty (int width, int height, int channels, uint32_t gl_pixel_format);
	
	
	// Asset Overrides
	void load_from_file(const char* filename) override;
	void reload() override;
	void unload() override;


	// Texture Overrides
	inline uint32_t get_id() override;
	void allocate() override;


	// Image data related stuff
	inline uint32_t get_width() { return m_width; }
	inline uint32_t get_height() { return m_height; }
	inline glm::ivec2 get_size() { return { m_width, m_height }; }
	
	// OpenGL Texture related stuff
	void upload_data();


	// Data manipulation related stuff
	uint32_t get_pixel(glm::ivec2 pos);
	uint32_t get_pixel(uint32_t x, uint32_t y);

	void write_color(uint32_t x, uint32_t y, uint32_t color);


private:
	uint32_t gl_pixel_format;

	uint32_t* m_color_data;

	bool m_should_reload = false;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_channels;
};

class Framebuffer {
public:
	Framebuffer(uint32_t width=1920, uint32_t height=1080);
	~Framebuffer();

	inline uint32_t get_id() { return m_gl_id; }

	inline uint32_t get_width() { return width; }
	inline uint32_t get_height() { return height; }

private:
	Texture2D color;
	uint32_t m_gl_id;

	uint32_t width, height;
};

void show_texture_inspector();