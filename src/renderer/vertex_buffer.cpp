
#include "vertex_buffer.hpp"

#include <iostream>
#include <format>

#include "util.hpp"

VertexBuffer::VertexBuffer(uint32_t vao_id)
{
	printf("Vertex Buffer Constructor\n");
	if (vao_id == -1) {
		glCreateVertexArrays(1, &vao_id);
	}

	m_vao_id = vao_id;

	glCreateBuffers(1, &m_vbo_id);

	printf("Genereated Buffers %d and %d\n", m_vao_id, m_vbo_id);
}


VertexBuffer::~VertexBuffer()
{
	printf("Destroyed VBO\n");
	glDeleteBuffers(1, &m_vbo_id);
	glDeleteVertexArrays(1, &m_vao_id);
}


void VertexBuffer::bind(int binding_index) {
	//glBindVertexArray(m_vao_id);
	GL_ERROR_CHECK();

	glVertexArrayVertexBuffer(m_vao_id, binding_index, m_vbo_id, 0, m_stride);
	GL_ERROR_CHECK();

	//glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
}


void VertexBuffer::set_layout(VertexBufferLayout layout, int binding_index, int base_attrib) {
	_layout = layout;
		bind();

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

				// for Mat4 we need to do something special!
				for (int i = 0; i < 4; i++) {

					std::cout << std::format("glVertexArrayAttribFormat(id={}, index={}, GetSize(dt)={}, GetGLType(dt)={}, false, offset={});\n",
						m_vao_id, index+i, 4, GetGLPrimitiveType(dt), offset + (4 * sizeof(float)) * i);


					glEnableVertexArrayAttrib(m_vao_id, index + i);
					glVertexArrayAttribFormat(m_vao_id, index + i, 4, GetGLPrimitiveType(dt), false, offset + (4 * sizeof(float)) * i);
					glVertexArrayAttribBinding(m_vao_id, index + i, binding_index); // TODO: ASSUMPTION THAT IFF MAT4 ¨ DIVISOR = 1 is bad
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

		glVertexArrayVertexBuffer(m_vao_id, binding_index, m_vbo_id, 0, size);
		GL_ERROR_CHECK();

		m_stride = size;
}


void VertexBuffer::set_data(void* data, size_t len) {
	glNamedBufferData(m_vbo_id, len, data, GL_STATIC_DRAW);
	GL_ERROR_CHECK();
}


std::vector<VertexAttribute>::iterator VertexBufferLayout::begin() {
	return vertex_attributes.begin();
}


std::vector<VertexAttribute>::iterator VertexBufferLayout::end() {
	return vertex_attributes.end();
}

