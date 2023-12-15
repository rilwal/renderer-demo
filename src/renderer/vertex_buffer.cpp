
#include "vertex_buffer.hpp"

#include <iostream>
#include <format>

#include "util.hpp"

VertexBuffer::VertexBuffer(uint32_t vao_id) : Buffer(BufferUsage::STATIC)
{
	printf("Vertex Buffer Constructor\n");
	if (vao_id == -1) {
		glCreateVertexArrays(1, &vao_id);
		GL_ERROR_CHECK();
	}

	m_vao_id = vao_id;

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


void VertexBuffer::set_layout(VertexBufferLayout layout, int binding_index, int base_attrib) {

	_layout = layout;

		int32_t size = 0;

		// First count the size of our layout
		for (const auto& attribute : layout) {
			size += (int32_t)getDataSize(attribute.data_type);
		}

		// Then loop over and set everything up
		size_t index = base_attrib;
		int32_t offset = 0;

		for (auto& attribute : layout) {

			const ShaderDataType& dt = attribute.data_type;

			if (dt == ShaderDataType::Mat4) {
				glVertexArrayBindingDivisor(m_vao_id, binding_index, 1);
				GL_ERROR_CHECK();

				// for Mat4 we need to do something special!
				for (int i = 0; i < 4; i++) {

					std::cout << std::format("glVertexArrayAttribFormat(id={}, index={}, GetSize(dt)={}, GetGLType(dt)={}, false, offset={});\n",
						m_vao_id, index+i, 4, GetGLPrimitiveType(dt), offset + (4 * sizeof(float)) * i);


					glEnableVertexArrayAttrib(m_vao_id, index + i);
					GL_ERROR_CHECK();

					glVertexArrayAttribFormat(m_vao_id, index + i, 4, GetGLPrimitiveType(dt), false, offset + (4 * sizeof(float)) * i);
					GL_ERROR_CHECK();

					glVertexArrayAttribBinding(m_vao_id, index + i, binding_index); // TODO: ASSUMPTION THAT IFF MAT4 ¨ DIVISOR = 1 is bad
					GL_ERROR_CHECK();
				}

				index += 4;
				offset += (int32_t)sizeof(glm::mat4);

			}
			else {
				std::cout << std::format("glVertexArrayAttribFormat(id={}, index={}, GetSize(dt)={}, GetGLType(dt)={}, false, offset={});\n",
					m_vao_id, index, GetGLPrimitiveCount(dt), GetGLPrimitiveType(dt), offset);

				glEnableVertexArrayAttrib(m_vao_id, index);
				GL_ERROR_CHECK();

				glVertexArrayAttribFormat(m_vao_id, index, GetGLPrimitiveCount(dt), GetGLPrimitiveType(dt), false, offset);
				GL_ERROR_CHECK();

				glVertexArrayAttribBinding(m_vao_id, index, binding_index);
				GL_ERROR_CHECK();

				attribute.offset = offset;

				offset += (int32_t)getDataSize(dt);
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

