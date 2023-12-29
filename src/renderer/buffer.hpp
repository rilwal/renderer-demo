#pragma once

#include <cstdint>
#include <iostream>

#include "glad/gl.h"

#include "util.hpp"

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
		return GL_INVALID_ENUM;
	}

private:
	Value m_value;
};


class Buffer {
public:
	Buffer(BufferUsage usage=BufferUsage::STATIC) : m_usage(usage) {
		glCreateBuffers(1, &m_gl_id);

		std::cerr << "Allocated Buffer " << m_gl_id << "\n";

		GL_ERROR_CHECK();
	}

	virtual ~Buffer() {
		std::cerr << "Deleted Buffer " << m_gl_id << "\n";

		glDeleteBuffers(1, &m_gl_id);
		GL_ERROR_CHECK();

		m_gl_id = 0;
	}



	// This function sets m_size of the buffer to zero without actually re-allocating anything
	// this means future uses of push_back will overwrite old data, but it's not really going
	// to delete any of the data, and old data might be accessed still.
	void soft_clear() {
		m_size = 0;
	}


	// This will clear the buffer, and reallocate it to 1K
	void clear(size_t size=1024) {
		glDeleteBuffers(1, &m_gl_id);
		m_gl_id = 0;
		glCreateBuffers(1, &m_gl_id);
		glNamedBufferData(m_gl_id, size, nullptr, m_usage);
		m_reserved_size = size;
	}


	void resize(size_t size) {
		if (m_reserved_size < size) {
			std::cerr << "Resize buffer " << m_gl_id << ": " << m_reserved_size << " -> " << size << "\n";

			if (m_size != 0) {
				// We need to make a copy if we already have some data
				uint32_t new_buffer = 0;
				glCreateBuffers(1, &new_buffer);
				glNamedBufferData(new_buffer, size, 0, m_usage);
				glCopyNamedBufferSubData(m_gl_id, new_buffer, 0, 0, m_size);
				glDeleteBuffers(1, &m_gl_id);
				m_gl_id = new_buffer;
			}
			else {
				//Otherwise, just set the size
				glNamedBufferData(m_gl_id, size, 0, m_usage);
				GL_ERROR_CHECK();
			}

			m_reserved_size = size;
		}
	}

	void set_data(const void* data, size_t size) {
		if (size > m_reserved_size) {
			uint32_t new_size = m_reserved_size || 1;

			while (new_size < size) {
				new_size *= 2;
			}

			resize(new_size);
		}

		glNamedBufferSubData(m_gl_id, 0, size, data);
		GL_ERROR_CHECK();

		m_size = size;
	}

	void set_subdata(const void* data, size_t offset, size_t size) {
		size_t min_size = offset + size;

		if (min_size > m_reserved_size) {
			uint32_t new_size = m_reserved_size || 1;

			while (new_size < min_size) {
				new_size *= 2;
			}

			resize(new_size);
		}

		glNamedBufferSubData(m_gl_id, offset, size, data);

		if (min_size > m_size) m_size = min_size;
		GL_ERROR_CHECK();

	}

	void bind(uint32_t bind_point) {
		glBindBuffer(bind_point, m_gl_id);
		GL_ERROR_CHECK();
	}

	void bind(uint32_t bind_point, uint32_t index) {
		glBindBufferBase(bind_point, index, m_gl_id);
		GL_ERROR_CHECK();
	}

	// Append some data to a buffer!
	size_t extend(const void* data, size_t len) {
		size_t offset = m_size;
		set_subdata(data, offset, len);
		return offset;
	}

	// Push an element to the back of a buffer,
	//	and return it's index
	template <typename T>
	size_t push_back(const T& element) {
		size_t offset = m_size;
		set_subdata(&element, offset, sizeof(T));
		if (offset >= std::numeric_limits<uint32_t>::max()) fprintf(stderr, "Buffer index too high?\n");
		return offset;
	}

	template <typename T>
	void set_data(const std::vector<T> data) {
		return set_data(data.data(), data.size() * sizeof(T));
	}

	template <typename T>
	size_t extend(const std::vector<T> data) {
		return extend(data.data(), data.size() * sizeof(T));
	}

	template<typename T> 
	void set_subdata(const T& val, size_t offset) {
		set_subdata(&val, offset, sizeof(T));
	}

	uint32_t get_id() { return m_gl_id; }

private:
	uint32_t m_gl_id = 0;
	size_t m_reserved_size = 0; // Size of the buffer on GPU, in bytes
	size_t m_size = 0;			// Amount of bytes used in the buffer

	BufferUsage m_usage;
};