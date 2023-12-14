#pragma once

#include <cinttypes>
#include <cassert>
#include <string>
#include <vector>

#include "types.hpp"

struct VertexAttribute {
	std::string name;
	ShaderDataType data_type;
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

class VertexBuffer {
public:
	VertexBuffer(uint32_t vao_id=-1);
	~VertexBuffer();

	VertexBuffer(const VertexBuffer& other) = delete;
		
	void bind(int binding_index=0);
	void set_layout(VertexBufferLayout layout, int binding_index=0, int base_attrib=0);
	void set_data(void* data, size_t len);

	inline int32_t get_stride() {
		return m_stride;
	}

	template<typename T>
	inline void set_data(std::vector<T> data) {
		set_data(data.data(), sizeof(T) * data.size());
	}

	uint32_t vao_id() { return m_vao_id; }
	uint32_t vbo_id() { return m_vbo_id; }


	static void bind_multiple(const VertexBuffer& a, const VertexBuffer& b) {
		glVertexArrayVertexBuffer(a.m_vao_id, 0, a.m_vbo_id, 0, a.m_stride);
		glVertexArrayVertexBuffer(b.m_vao_id, 1, b.m_vbo_id, 0, b.m_stride);
	}


private:
	uint32_t m_vao_id;
	uint32_t m_vbo_id;

	VertexBufferLayout _layout;

	int32_t m_stride = 0;

};

