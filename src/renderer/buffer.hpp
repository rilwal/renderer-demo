#pragma once

#include<cstdint>

#include "glad/gl.h"

// Some helper for all GL buffers

class BufferUsage {
public:
	enum Value : uint8_t {
		STATIC,
		STREAM
	};

	constexpr BufferUsage(Value value) : m_value(value) {};

	operator Value() {
		return m_value;
	}

	operator GLenum() {
		switch (m_value) {
		case STATIC: return GL_STATIC_DRAW;
		case STREAM: return GL_STREAM_DRAW;
		}
	}

private:
	Value m_value;
};
