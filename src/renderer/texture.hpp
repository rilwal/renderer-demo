#pragma once

#include <cinttypes>

#include "assets/asset_manager.hpp"
#include "assets/image.hpp"

#include "types.hpp"
#include "gtc/type_ptr.hpp"




enum class TextureFilter : uint32_t {
	Nearest = GL_NEAREST,
	NearestMipmap = GL_NEAREST_MIPMAP_NEAREST,
	Bilinear = GL_LINEAR,
	BilinearMipmap = GL_NEAREST_MIPMAP_LINEAR,
	Trilinear = GL_LINEAR_MIPMAP_LINEAR
};

enum class TextureWrap : uint32_t {
	ClampEdge = GL_CLAMP_TO_EDGE,
	MirrorRepeat = GL_MIRRORED_REPEAT,
	Repeat = GL_REPEAT,
	MirrorClampEdge = GL_MIRROR_CLAMP_TO_EDGE,
	ClampBorder = GL_CLAMP_TO_BORDER
};

struct Sampler {
	Enum<TextureFilter> min_filter = TextureFilter::Bilinear;
	Enum<TextureFilter> mag_filter = TextureFilter::Bilinear;

	Enum<TextureWrap> wrap_s = TextureWrap::Repeat;
	Enum<TextureWrap> wrap_t = TextureWrap::Repeat;
};


inline uint64_t make_bindless_texture(std::unique_ptr<Image> i, Sampler s = {}) {
	uint32_t texture_id = 0;
	uint32_t sampler_id = 0;

	glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
	glCreateSamplers(1, &sampler_id);

	glBindTexture(GL_TEXTURE_2D, texture_id);

	if (is_compressed(i->format)) {
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, get_gl_format(i->format), i->width, i->height, 0, i->data.size(), i->data.data());
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, get_gl_format(i->format), i->width, i->height, 0, get_gl_format(i->format), GL_UNSIGNED_BYTE, i->data.data());
	}

	glGenerateMipmap(GL_TEXTURE_2D);

	glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, s.min_filter);
	glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, s.mag_filter);
	glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, s.wrap_s);
	glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, s.wrap_t);

	glm::vec4 border_color = { 0, 0, 0, 0 };
	glSamplerParameterfv(sampler_id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(border_color));

	uint64_t handle = glGetTextureSamplerHandleARB(texture_id, sampler_id);
	glMakeTextureHandleResidentARB(handle); // TODO: Investigate the effect of making too many textures resident?

	return handle;
}


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