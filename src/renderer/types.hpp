#pragma once

#include "glad/gl.h"
#include "glm.hpp"

/*
	TODO: add support for samplers and textures
*/

// All the data types we can use in a shader.
// These can be used for uniforms or VertexArray layouts
enum class ShaderDataType {
	Bool,
	F16, F32, F64,
	Vec2, Vec3, Vec4,
	Mat2, Mat3, Mat4,
	Color,

	I8, U8,
	I16, U16,
	I32, U32,
	//I64, U64,

	IVec2, IVec3, IVec4,

	Sampler2D,
	Texture2D = Sampler2D,

	Unknown=-1
};

#define FOREACH_SHADER_DATA_TYPE(DO) \
	DO(ShaderDataType::Bool) \
	DO(ShaderDataType::F16) \
	DO(ShaderDataType::F32) \
	DO(ShaderDataType::F64) \
	DO(ShaderDataType::Vec2) \
	DO(ShaderDataType::Vec3) \
	DO(ShaderDataType::Vec4) \
	DO(ShaderDataType::Mat2) \
	DO(ShaderDataType::Mat3) \
	DO(ShaderDataType::Color) \
	DO(ShaderDataType::I8) \
	DO(ShaderDataType::I16) \
	DO(ShaderDataType::I32) \
	DO(ShaderDataType::U8) \
	DO(ShaderDataType::U16) \
	DO(ShaderDataType::U32) \
	DO(ShaderDataType::Sampler2D)



constexpr ShaderDataType GetShaderDataType(GLenum dt) {
	switch (dt) {
		case GL_HALF_FLOAT: return ShaderDataType::F16;
		case GL_FLOAT: return ShaderDataType::F32;
		case GL_DOUBLE: return ShaderDataType::F64;

		case GL_FLOAT_VEC2: return ShaderDataType::Vec2;
		case GL_FLOAT_VEC3: return ShaderDataType::Vec3;
		case GL_FLOAT_VEC4: return ShaderDataType::Vec4;

		case GL_BOOL: return ShaderDataType::Bool;

		case GL_BYTE: return	ShaderDataType::I8;
		case GL_SHORT: return	ShaderDataType::I16;
		case GL_INT: return		ShaderDataType::I32;

		case GL_UNSIGNED_BYTE:	return ShaderDataType::U8;
		case GL_UNSIGNED_SHORT: return ShaderDataType::U16;
		case GL_UNSIGNED_INT:	return ShaderDataType::U32;

		case GL_FLOAT_MAT2: return ShaderDataType::Mat2;
		case GL_FLOAT_MAT3: return ShaderDataType::Mat3;
		case GL_FLOAT_MAT4: return ShaderDataType::Mat4;

		case GL_SAMPLER_2D: return ShaderDataType::Sampler2D;
	}

	return ShaderDataType::Unknown;
}


// Returns the primitive type behind the ShaderDataType
constexpr GLenum GetGLPrimitiveType(ShaderDataType dt) {
	switch (dt) {
		case ShaderDataType::F16:	return GL_HALF_FLOAT;
		case ShaderDataType::F32:	return GL_FLOAT;
		case ShaderDataType::F64:	return GL_DOUBLE;

		case ShaderDataType::Vec2:	return GL_FLOAT;
		case ShaderDataType::Vec3:	return GL_FLOAT;
		case ShaderDataType::Vec4:	return GL_FLOAT;

		case ShaderDataType::Mat2:	return GL_FLOAT;
		case ShaderDataType::Mat3:	return GL_FLOAT;
		case ShaderDataType::Mat4:	return GL_FLOAT;

		case ShaderDataType::Color:	return GL_FLOAT;

		case ShaderDataType::IVec2:	return GL_INT;
		case ShaderDataType::IVec3:	return GL_INT;
		case ShaderDataType::IVec4:	return GL_INT;

		case ShaderDataType::I8:	return GL_BYTE;
		case ShaderDataType::I16:	return GL_SHORT;
		case ShaderDataType::I32:	return GL_INT;

		case ShaderDataType::U8:	return GL_UNSIGNED_BYTE;
		case ShaderDataType::U16:	return GL_UNSIGNED_SHORT;
		case ShaderDataType::U32:	return GL_UNSIGNED_INT;

		case ShaderDataType::Bool: return GL_BOOL;

		case ShaderDataType::Sampler2D: return GL_UNSIGNED_INT;

	}

	assert(false && "Unknown Data Type");
	return -1;
}

constexpr size_t GetGLPrimitiveSize(GLenum t) {
	switch (t) {
		case GL_HALF_FLOAT: return 2;
		case GL_FLOAT: return 4;
		case GL_DOUBLE: return 8;

		case GL_BYTE: return 1;
		case GL_SHORT: return 2;
		case GL_INT: return 4;
	
		case GL_UNSIGNED_BYTE: return 1;
		case GL_UNSIGNED_SHORT: return 2;
		case GL_UNSIGNED_INT: return 4;
	}

	assert(false && "UNKNOWN GL TYPE");
	return 0;
}


// Returns the primitive count behind the ShaderDataType
constexpr int32_t GetGLPrimitiveCount(ShaderDataType dt) {
	switch (dt) {
	case ShaderDataType::F16: return 1;
	case ShaderDataType::F32:	return 1;
	case ShaderDataType::F64:	return 1;

	case ShaderDataType::Vec2:	return 2;
	case ShaderDataType::Vec3:	return 3;
	case ShaderDataType::Vec4:	return 4;

	case ShaderDataType::Mat2:	return 2 * 2;
	case ShaderDataType::Mat3:	return 3 * 3;
	case ShaderDataType::Mat4:	return 4 * 4;

	case ShaderDataType::Color:	return 3;

	case ShaderDataType::IVec2:	return 2;
	case ShaderDataType::IVec3:	return 3;
	case ShaderDataType::IVec4:	return 4;

	case ShaderDataType::I8:	return 1;
	case ShaderDataType::I16:	return 1;
	case ShaderDataType::I32:	return 1;

	case ShaderDataType::U8:	return 1;
	case ShaderDataType::U16:	return 1;
	case ShaderDataType::U32:	return 1;

	case ShaderDataType::Sampler2D: return 1;

	}
	assert(false && "Unknown Data Type");
	return -1;
}

template<ShaderDataType dt>
constexpr auto CreateGLType() {
	assert(false && "Unknown Data Type");
	return float();
}

#define SHADER_TYPE(x, y) template<> constexpr auto CreateGLType<x>() { return y(); }
SHADER_TYPE(ShaderDataType::F16, uint16_t);
SHADER_TYPE(ShaderDataType::F32, float);
SHADER_TYPE(ShaderDataType::F64, double);

SHADER_TYPE(ShaderDataType::I8,  int8_t);
SHADER_TYPE(ShaderDataType::I16, int16_t);
SHADER_TYPE(ShaderDataType::I32, int32_t);

SHADER_TYPE(ShaderDataType::U8,	 uint8_t);
SHADER_TYPE(ShaderDataType::U16, uint16_t);
SHADER_TYPE(ShaderDataType::U32, uint32_t);

SHADER_TYPE(ShaderDataType::Vec2, glm::vec2);
SHADER_TYPE(ShaderDataType::Vec3, glm::vec3);
SHADER_TYPE(ShaderDataType::Vec4, glm::vec4);


SHADER_TYPE(ShaderDataType::Mat2, glm::mat2);
SHADER_TYPE(ShaderDataType::Mat3, glm::mat3);
SHADER_TYPE(ShaderDataType::Mat4, glm::mat4);

SHADER_TYPE(ShaderDataType::Color, glm::vec4);

SHADER_TYPE(ShaderDataType::IVec2, glm::ivec2);
SHADER_TYPE(ShaderDataType::IVec3, glm::ivec3);
SHADER_TYPE(ShaderDataType::IVec4, glm::ivec4);

SHADER_TYPE(ShaderDataType::Bool, int32_t);

SHADER_TYPE(ShaderDataType::Sampler2D, uint32_t);
#undef SHADER_TYPE

template<ShaderDataType dt>
constexpr auto CreatePrim() {
	assert(false && "Unknown Data Type");
	return float();
}

#define SHADER_PRIMITIVE(x, y) template<> constexpr auto CreatePrim<x>() { return y(); }
SHADER_PRIMITIVE(ShaderDataType::F16, uint16_t);
SHADER_PRIMITIVE(ShaderDataType::F32, float);
SHADER_PRIMITIVE(ShaderDataType::F64, double);

SHADER_PRIMITIVE(ShaderDataType::I8,  int8_t);
SHADER_PRIMITIVE(ShaderDataType::I16, int16_t);
SHADER_PRIMITIVE(ShaderDataType::I32, int32_t);

SHADER_PRIMITIVE(ShaderDataType::U8,  uint8_t);
SHADER_PRIMITIVE(ShaderDataType::U16, uint16_t);
SHADER_PRIMITIVE(ShaderDataType::U32, uint32_t);

SHADER_PRIMITIVE(ShaderDataType::Vec2, float);
SHADER_PRIMITIVE(ShaderDataType::Vec3, float);
SHADER_PRIMITIVE(ShaderDataType::Vec4, float);

SHADER_PRIMITIVE(ShaderDataType::Mat2, float);
SHADER_PRIMITIVE(ShaderDataType::Mat3, float);
SHADER_PRIMITIVE(ShaderDataType::Mat4, float);


SHADER_PRIMITIVE(ShaderDataType::Color, float);

SHADER_PRIMITIVE(ShaderDataType::IVec2, int32_t);
SHADER_PRIMITIVE(ShaderDataType::IVec3, int32_t);
SHADER_PRIMITIVE(ShaderDataType::IVec4, int32_t);

SHADER_PRIMITIVE(ShaderDataType::Bool, int32_t);
#undef SHADER_PRIMITIVE


constexpr const char* GetDataTypeName(ShaderDataType t){
	switch (t) {
	case ShaderDataType::I8:	return "I8";
	case ShaderDataType::I16:	return "I16";
	case ShaderDataType::I32:	return "I32";
	case ShaderDataType::U8:	return "U8";
	case ShaderDataType::U16:	return "U16";
	case ShaderDataType::U32:	return "U32";
	case ShaderDataType::F16:	return "F16";
	case ShaderDataType::F32:	return "F32";
	case ShaderDataType::F64:	return "F64";
	case ShaderDataType::Vec2:	return "Vec2";
	case ShaderDataType::Vec3:	return "Vec3";
	case ShaderDataType::Vec4:	return "Vec4";
	case ShaderDataType::Mat2:	return "Mat2";
	case ShaderDataType::Mat3:	return "Mat3";
	case ShaderDataType::Mat4:	return "Mat4";
	case ShaderDataType::Color:	return "Color";
	case ShaderDataType::IVec2:	return "IVec2";
	case ShaderDataType::IVec3:	return "IVec3";
	case ShaderDataType::IVec4:	return "IVec4";
	case ShaderDataType::Bool:	return "Bool";
	case ShaderDataType::Sampler2D: return "Tex2D";

	default: return "Unknown";
	}
}




// Reutrns the C type corresponding to a ShaderDataType
#define GetCType(x) decltype(CreateGLType<x>())

#define GetCPrimitiveType(x) decltype(CreatePrim<x>())

// Returns the size of a ShaderDataType
constexpr size_t getDataSize(ShaderDataType dt) {
	return GetGLPrimitiveSize(GetGLPrimitiveType(dt)) * GetGLPrimitiveCount(dt);
}



using TextureHandle = uint32_t;
using MeshHandle = uint32_t;
using MaterialHandle = uint32_t;

constexpr MaterialHandle default_material = 0;
