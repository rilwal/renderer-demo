
#include "index_buffer.hpp"

#include "glad/gl.h"

IndexBuffer::IndexBuffer(BufferUsage usage, ShaderDataType type) 
	: m_usage(usage), m_type(type) {
	glCreateBuffers(1, &m_gl_id);
}

IndexBuffer::~IndexBuffer() {
	glDeleteBuffers(1, &m_gl_id);
}

void IndexBuffer::resize(size_t size) {
	glNamedBufferData(m_gl_id, size * getDataSize(m_type), 0, m_usage);
	m_reserved_size = size;
}

void IndexBuffer::set_data(void* data, size_t count) {
	if (count < m_reserved_size) {
		resize(count);
	}

	glNamedBufferSubData(m_gl_id, 0, count * getDataSize(m_type), data);
}

void IndexBuffer::set_subdata(void* data, size_t offset, size_t count) {
	if ((offset + count) < m_reserved_size) {
		resize(offset + count);
	}

	glNamedBufferSubData(m_gl_id, offset * getDataSize(m_type), count * getDataSize(m_type), data);
}

void IndexBuffer::bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gl_id);
}