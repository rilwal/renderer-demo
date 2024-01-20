
#include "vertex_buffer.hpp"

#include <iostream>
#include <format>

#include "util.hpp"

VertexBuffer::VertexBuffer(uint32_t vao_id, uint32_t binding_idx) : Buffer(BufferUsage::STATIC)
{
	printf("Vertex Buffer Constructor\n");
	if (vao_id == -1) {
		glCreateVertexArrays(1, &vao_id);
		GL_ERROR_CHECK();
	}

	m_vao_id = vao_id;
	m_binding_index = binding_idx;

	glBindVertexArray(m_vao_id);
	GL_ERROR_CHECK();

	printf("Genereated Buffers %d and %d\n", m_vao_id, Buffer::get_id());
}


VertexBuffer::~VertexBuffer()
{
	printf("Destroyed VBO\n");
	glDeleteVertexArrays(1, &m_vao_id);
}


void VertexBuffer::bind(int binding_index) {
	glVertexArrayVertexBuffer(m_vao_id, binding_index, Buffer::get_id(), 0, m_stride);
	GL_ERROR_CHECK();
}


void VertexBuffer::set_layout(VertexBufferLayout layout, uint32_t base_attrib) {

	_layout = layout;

		int32_t size = 0;

		// First count the size of our layout
		for (const auto& attribute : layout) {
			size += (int32_t)getDataSize(attribute.data_type) * attribute.count;
		}

		// Then loop over and set everything up
		uint32_t index = base_attrib;
		uint32_t offset = 0;

		for (auto& attribute : layout) {

			const ShaderDataType& dt = attribute.data_type;

			if (dt == ShaderDataType::Mat4) {

				// for Mat4 we need to do something special!
				for (int i = 0; i < 4; i++) {

					std::cout << std::format("glVertexArrayAttribFormat(id={}, index={}, GetSize(dt)={}, GetGLType(dt)={}, false, offset={});\n",
						m_vao_id, index + i, 4, GetGLPrimitiveType(dt), offset + (4 * sizeof(float)) * i);


					glEnableVertexArrayAttrib(m_vao_id, index + i);
					glVertexArrayAttribFormat(m_vao_id, index + i, 4, GetGLPrimitiveType(dt), false, offset + (4 * sizeof(float)) * i);
					glVertexArrayAttribBinding(m_vao_id, index + i, m_binding_index); 
				}

				index += 4;
				offset += (int32_t)sizeof(glm::mat4);

			}
			else {
				std::cout << std::format("glVertexArrayAttribFormat(id={}, index={}, GetSize(dt)={}, GetGLType(dt)={}, false, offset={});\n",
					m_vao_id, index, GetGLPrimitiveCount(dt), GetGLPrimitiveType(dt), offset);

				glEnableVertexArrayAttrib(m_vao_id, index);


				if (dt == ShaderDataType::U32 || dt == ShaderDataType::I32) {
					glVertexArrayAttribIFormat(m_vao_id, index, GetGLPrimitiveCount(dt) * attribute.count, GetGLPrimitiveType(dt), offset);
				}
				else {
					glVertexArrayAttribFormat(m_vao_id, index, GetGLPrimitiveCount(dt) * values_per_primitive(dt) * attribute.count, GetGLPrimitiveType(dt), is_normalized_type(dt), offset);
				}
				
				glVertexArrayAttribBinding(m_vao_id, index, m_binding_index);

				attribute.offset = offset;

				offset += (int32_t)getDataSize(dt) * attribute.count;
				index++;
			}
		}

		m_stride = size;
}


void VertexBuffer::resize(size_t size) {
	Buffer::resize(size);
}


void VertexBuffer::set_data(void* data, size_t len) {
	Buffer::set_data(data, len);
}


std::vector<VertexAttribute>::iterator VertexBufferLayout::begin() {
	return vertex_attributes.begin();
}


std::vector<VertexAttribute>::iterator VertexBufferLayout::end() {
	return vertex_attributes.end();
}

