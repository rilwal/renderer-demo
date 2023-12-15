#pragma once

#include <cstdint>
#include <vector>

#include "glm.hpp"

#include "buffer.hpp"
#include "types.hpp"

// This file provides a basic high level abstraction for an index buffer
// TODO: Make a generic buffer interface for all types of buffers?

class IndexBuffer : public Buffer {
public:
	IndexBuffer(BufferUsage usage=BufferUsage::STATIC, ShaderDataType sdt=ShaderDataType::U32);
	~IndexBuffer();

	void resize(size_t count);
	
	void set_data(void* data, size_t count);

	template <typename T>
	void set_data(std::vector<T> data) {
		//assert(T == GetCType(m_type));
		Buffer::set_data(data.data(), data.size() * sizeof(T));
	}

	void set_subdata(void* data, size_t offset, size_t count);

	template <typename T>
	void set_subdata(std::vector<T> data, size_t offset) {
		//assert(T == GetCType(m_type));
		Buffer::set_subdata(data.data(), offset, data.size());
	}

	void bind();

private:
	ShaderDataType m_type;
};
