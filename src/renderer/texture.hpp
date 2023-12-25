#pragma once

#include <cinttypes>

#include "assets/asset_manager.hpp"
#include "assets/image.hpp"

#include "types.hpp"
#include "gtc/type_ptr.hpp"

#include "buffer.hpp"


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


inline uint64_t make_bindless_texture(Ref<Image> i, Sampler s = {}) {
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
