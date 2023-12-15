
#include "index_buffer.hpp"

#include "glad/gl.h"

IndexBuffer::IndexBuffer(BufferUsage usage, ShaderDataType type) 
	: Buffer(usage), m_type(type) {
}

IndexBuffer::~IndexBuffer() {
}

void IndexBuffer::resize(size_t count) {
	Buffer::resize(count * getDataSize(m_type));
} 

void IndexBuffer::set_data(void* data, size_t count) {
	Buffer::set_data(data, count * getDataSize(m_type));
}

void IndexBuffer::set_subdata(void* data, size_t offset, size_t count) {
	Buffer::set_subdata(data, offset * getDataSize(m_type), count * getDataSize(m_type));

}

void IndexBuffer::bind() {
	Buffer::bind(GL_ELEMENT_ARRAY_BUFFER);
}