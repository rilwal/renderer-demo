#pragma once

#include <cinttypes>
#include <cassert>
#include <string>
#include <vector>

#include "types.hpp"
#include "buffer.hpp"

struct VertexAttribute {
	std::string name;
	ShaderDataType data_type;
	int32_t count = 1;
	int32_t offset = 0;
};


struct VertexBufferLayout {
	std::vector<VertexAttribute> vertex_attributes;

	VertexBufferLayout() {

	}

	VertexBufferLayout(std::initializer_list<VertexAttribute> il)
		: vertex_attributes(il) { };

	inline void append(const VertexAttribute& va) { vertex_attributes.push_back(va); };

	std::vector<VertexAttribute>::iterator begin();
	std::vector<VertexAttribute>::iterator end();

};


class VertexArray {
public:
	VertexArray() {
		glCreateVertexArrays(1, &m_gl_id);
	}

	virtual ~VertexArray() {
		glDeleteVertexArrays(1, &m_gl_id);
	}

	operator const uint32_t() {
		return m_gl_id;
	}

	void bind() {
		glBindVertexArray(m_gl_id);
	}

private:
	uint32_t m_gl_id;
};


class VertexBuffer : public Buffer {
public:
	VertexBuffer(uint32_t vao_id=-1, uint32_t binding_idx = 0);
	~VertexBuffer();

	VertexBuffer(const VertexBuffer& other) = delete;
		
	void bind(int binding_index=0);

	void set_layout(VertexBufferLayout layout, uint32_t base_attrib=0);

	void resize(size_t size);

	void set_data(void* data, size_t count);

	void set_per_instance(bool per_instance) {
		if (per_instance) {
			glVertexArrayBindingDivisor(m_vao_id, m_binding_index, 1);
		}
		else {
			glVertexArrayBindingDivisor(m_vao_id, m_binding_index, 0);

		}
	}

	inline int32_t get_stride() {
		return m_stride;
	}

	template<typename T>
	inline void set_data(std::vector<T> data) {
		set_data(data.data(), sizeof(T) * data.size());
	}

	void set_subdata(void* data, size_t offset, size_t count) {
		Buffer::set_subdata(data, offset, count);
	}

	template<typename T>
	inline void set_subdata(std::vector<T> data, size_t offset) {
		set_subdata(data.data(), offset, data.size() * sizeof(T));
	}

	uint32_t vao_id() { return m_vao_id; }
	uint32_t vbo_id() { return Buffer::get_id(); }


private:
	uint32_t m_binding_index = 0;
	uint32_t m_vao_id = 0;
	bool m_per_instance = false;

	VertexBufferLayout _layout;

	int32_t m_stride = 0;

};

