#pragma once

#include <cinttypes>
#include <glad/gl.h>

#include "util.hpp"

namespace gl {

	class TextureBaseType {
	public:
		enum TextureBaseTypeEnum : uint32_t {
			U8 = GL_UNSIGNED_BYTE,
			U16 = GL_UNSIGNED_SHORT,
			U32 = GL_UNSIGNED_INT,

			Float = GL_FLOAT,

			SNORM8,
			SNORM16,
			SNORM32,
		};

		TextureBaseType() : m_value() {};
		TextureBaseType(TextureBaseTypeEnum val) : m_value(val) {};
		TextureBaseType(uint32_t val) : m_value(static_cast<TextureBaseTypeEnum>(val)) {};

		operator TextureBaseTypeEnum() const {
			return m_value;
		}

		operator TextureBaseTypeEnum& () {
			return m_value;
		}

		operator uint32_t() {
			return static_cast<uint32_t>(m_value);
		}

	private:
		TextureBaseTypeEnum m_value;
	};


	class TextureFormat {
	public:
		enum TextureFormatEnum : int32_t {
			Red = GL_RED,
			RG = GL_RG,
			RGB = GL_RGB,
			RGBA = GL_RGBA,
			Depth = GL_DEPTH_COMPONENT
		};


		TextureFormat() : m_value() {};
		TextureFormat(TextureFormatEnum val) : m_value(val) {};
		TextureFormat(uint32_t val) : m_value(static_cast<TextureFormatEnum>(val)) {};

		operator TextureFormatEnum() const {
			return m_value;
		}

		operator TextureFormatEnum& () {
			return m_value;
		}

		operator uint32_t() {
			return static_cast<uint32_t>(m_value);
		}

	private:
		TextureFormatEnum m_value;

	};


	class TextureInternalFormat {
	public:
		enum TextureInternalFormatEnum : int32_t {
			R8 = GL_R8,
			R8_SNORM = GL_R8_SNORM,
			R16 = GL_R16,
			R16_SNORM = GL_R16_SNORM,
			RG8 = GL_RG8,
			RG8_SNORM = GL_RG8_SNORM,
			RG16 = GL_RG16,
			RG16_SNORM = GL_RG16_SNORM,
			R3_G3_B2 = GL_R3_G3_B2,
			RGB4 = GL_RGB4,
			RGB5 = GL_RGB5,
			RGB8 = GL_RGB8,
			RGB8_SNORM = GL_RGB8_SNORM,
			RGB10 = GL_RGB10,
			RGB12 = GL_RGB12,
			RGB16_SNORM = GL_RGB16_SNORM,
			RGBA2 = GL_RGBA2,
			RGBA4 = GL_RGBA4,
			RGB5_A1 = GL_RGB5_A1,
			RGBA8 = GL_RGBA8,
			RGBA8_SNORM = GL_RGBA8_SNORM,
			RGB10_A2 = GL_RGB10_A2,
			RGB10_A2UI = GL_RGB10_A2UI,
			RGBA12 = GL_RGBA12,
			RGBA16 = GL_RGBA16,
			SRGB8 = GL_SRGB8,
			SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
			R16F = GL_R16F,
			RG16F = GL_RG16F,
			RGB16F = GL_RGB16F,
			RGBA16F = GL_RGBA16F,
			R32F = GL_R32F,
			RG32F = GL_RG32F,
			RGB32F = GL_RGB32F,
			RGBA32F = GL_RGBA32F,
			R11F_G11F_B10F = GL_R11F_G11F_B10F,
			RGB9_E5 = GL_RGB9_E5,
			R8I = GL_R8I,
			R8UI = GL_R8UI,
			R16I = GL_R16I,
			R16UI = GL_R16UI,
			R32I = GL_R32I,
			R32UI = GL_R32UI,
			RG8I = GL_RG8I,
			RG8UI = GL_RG8UI,
			RG16I = GL_RG16I,
			RG16UI = GL_RG16UI,
			RG32I = GL_RG32I,
			RG32UI = GL_RG32UI,
			RGB8I = GL_RGB8I,
			RGB8UI = GL_RGB8UI,
			RGB16I = GL_RGB16I,
			RGB16UI = GL_RGB16UI,
			RGB32I = GL_RGB32I,
			RGB32UI = GL_RGB32UI,
			RGBA8I = GL_RGBA8I,
			RGBA8UI = GL_RGBA8UI,
			RGBA16I = GL_RGBA16I,
			RGBA16UI = GL_RGBA16UI,
			RGBA32I = GL_RGBA32I,
			RGBA32UI = GL_RGBA32UI
		};


		TextureInternalFormat() : m_value() {};
		TextureInternalFormat(TextureInternalFormatEnum val) : m_value(val) {};
		TextureInternalFormat(uint32_t val) : m_value(static_cast<TextureInternalFormatEnum>(val)) {};

		operator TextureInternalFormatEnum() const {
			return m_value;
		}

		operator TextureInternalFormatEnum& () {
			return m_value;
		}

		operator uint32_t() {
			return static_cast<uint32_t>(m_value);
		}

	private:
		TextureInternalFormatEnum m_value;

	};

		
	template<TextureBaseType::TextureBaseTypeEnum t, uint32_t channels>
	inline TextureInternalFormat get_internal_format() {
		fprintf(stderr, "Unsupported GL base type requested!\n");
		return -1;
	}

#define Format(BaseType, Channels, InternalFormat) \
	template<> inline TextureInternalFormat get_internal_format<TextureBaseType::BaseType, Channels>() { \
		return TextureInternalFormat::InternalFormat; \
	}
	
	Format(U8, 1, R8);
	Format(U8, 2, RG8);
	Format(U8, 3, RGB8);
	Format(U8, 4, RGBA8);

	Format(U16, 1, R16);
	Format(U16, 2, RG16);
	Format(U16, 3, RGB16UI);
	Format(U16, 4, RGBA32UI);

	Format(U32, 1, R32UI);
	Format(U32, 2, RG32UI);
	Format(U32, 3, RGB32UI);
	Format(U32, 4, RGBA32UI);

	Format(SNORM8, 1, R8_SNORM);
	Format(SNORM8, 2, RG8_SNORM);
	Format(SNORM8, 3, RGB8_SNORM);
	Format(SNORM8, 4, RGBA8_SNORM);

	Format(SNORM16, 1, R16_SNORM);
	Format(SNORM16, 2, RG16_SNORM);
	Format(SNORM16, 3, RGB16_SNORM);

	Format(Float, 1, R32F);
	Format(Float, 2, RG32F);
	Format(Float, 3, RGB32F);
	Format(Float, 4, RGBA32F);

#undef Format


}