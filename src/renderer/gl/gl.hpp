#pragma once

/*
	The goal of this file is to attempt to make a more sane C++ binding to the OpenGL APIs that I use.
*/

#include <cinttypes>

#include "util.hpp"

#include "texture_internal_format.hpp"

// Use a namespace instead of a prefix
namespace gl {
	// TODO: include gl in the cpp
	#include <glad/gl.h>

	enum class TextureParameterEnum : uint32_t {
		DepthStencilMode = GL_DEPTH_STENCIL_TEXTURE_MODE,
		BaseLevel = GL_TEXTURE_BASE_LEVEL,
		CompareFunc = GL_TEXTURE_COMPARE_FUNC,
		CompareMode = GL_TEXTURE_COMPARE_MODE,
		LODBias = GL_TEXTURE_LOD_BIAS,
		MinFilter = GL_TEXTURE_MIN_FILTER,
		MagFilter = GL_TEXTURE_MAG_FILTER,
		MinLOD = GL_TEXTURE_MIN_LOD,
		MaxLOD = GL_TEXTURE_MAX_LOD,
		MaxLevel = GL_TEXTURE_MAX_LEVEL,
		SwizzleR = GL_TEXTURE_SWIZZLE_R,
		SwizzleG = GL_TEXTURE_SWIZZLE_G,
		SwizzleB = GL_TEXTURE_SWIZZLE_B,
		SwizzleA = GL_TEXTURE_SWIZZLE_A,
		WrapS = GL_TEXTURE_WRAP_S,
		WrapT = GL_TEXTURE_WRAP_T,
		WrapR = GL_TEXTURE_WRAP_R,
		BorderColor = GL_TEXTURE_BORDER_COLOR,
		Swizzle = GL_TEXTURE_SWIZZLE_RGBA
	};

	enum class TextureTargetEnum : uint32_t {
		Texture1D = GL_TEXTURE_1D,
		Texture2D = GL_TEXTURE_2D,
		Texture3D = GL_TEXTURE_3D,
		Rectangle = GL_TEXTURE_RECTANGLE,
		Cube = GL_TEXTURE_CUBE_MAP,
		Array1D = GL_TEXTURE_1D_ARRAY,
		Array2D = GL_TEXTURE_2D_ARRAY,
		CubeArray = GL_TEXTURE_CUBE_MAP_ARRAY
	};




	using TextureParameter = Enum<TextureParameterEnum>;
	using TextureTarget = Enum<TextureTargetEnum>;

	inline uint32_t create_texture(TextureTarget t) {
		uint32_t tmp = 0;
		glCreateTextures(t, 1, &tmp);
		return tmp;
	}

	inline void texture_parameter(uint32_t texture, TextureParameter parameter, float value) {
		glTextureParameterf(texture, parameter, value);
	}

	inline void texture_parameter(uint32_t texture, TextureParameter parameter, int value) {
		glTextureParameteri(texture, parameter, value);
	}

	template<uint32_t i>
	inline void texture_parameter(uint32_t texture, TextureParameter parameter, std::array<float, i> data) {
		glTexParameterfv(texture, parameter, &data[0]);
	}

	inline void bind_texture(TextureTarget target, uint32_t texture) {
		glBindTexture(target, texture);
	}

	inline void texture_image(TextureTarget target, int32_t level, TextureInternalFormat internal_format, glm::ivec2 size, TextureFormat format, TextureBaseType type, void* data) {
		glTexImage2D(target, level, internal_format, size.x, size.y, 0, format, type, data);
	}

}