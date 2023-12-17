#pragma once

/*
TODO: Renderer
	TODO: Camera and projections

	TODO: Lighting
		TODO: Store light info and pass to shaders
		TODO: Shadow mapping? -> Soft shadows??

	TODO: HDR Pipeline and postprocessing
		TODO: Render to HDR texture
		TODO: Bloom
		TODO: Tonemapping
		TODO: Gamma correction
		TODO: Basic brightness, contrast etc.

	TODO: Model rendering
	
	TODO: Basic primitive rendering
		TODO: Cubes
		TODO: Spheres
		TODO: Planes

	TODO: 2D rendering functions
		TODO: Quads
		TODO: Text

	TODO: Batched rendering?
	TODO: Instanced rendering?
*/

// TODO: Kill this!
struct GLFWwindow;


#include <string>
#include <initializer_list>

#include <glm.hpp>
#include "flecs.h"

#include "types.hpp"
#include "util.hpp"
#include "ecs_componets.hpp"

#include "asset_manager.hpp"
#include "vertex_buffer.hpp"
#include "index_buffer.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "camera.hpp"


class Window {
public:
	Window(int width, int height, std::string title);
	~Window();

	glm::ivec2 get_size();

	operator GLFWwindow* () { return platform_window; }

private:
	GLFWwindow* platform_window;
};


class Renderer {
public:
	Renderer() : window(1920, 1080, "Engine") {};

	void initialize();
	void shutdown();

	bool should_close();

	void begin_frame();
	void end_frame();

	//TODO: Kill this when possible
	GLFWwindow* get_platform_window();

	int get_window_width();
	int get_window_height();

private:
	Window window;
};



// This represents a number of meshes in a single vertex buffer.
// TODO: Think about a more appropriate name? (maybe renderer lol)
// Limitations: All meshes must have the same vertex specification!!!
// Maybe template this by vertex spec somehow?
class MeshBundle {
public:
#pragma pack(push, 1)
	struct PerInstanceData {
		glm::mat4 model;
		uint32_t material_idx;
	};
#pragma pack(pop)



	MeshBundle()
		: m_vertex_array(), m_vertex_buffer(m_vertex_array), m_per_idx_buffer(m_vertex_array),
		m_command_buffer(), m_draw_query(), m_main_shader(asset_manager.GetByPath<Shader>("assets/shaders/basic.glsl")) {
		// For now we are hardcoding position,normal.
		// And MVP, Model (testing just Model!)
		// TODO: Fix that!!!

		m_vertex_buffer.set_layout({
			{"position", ShaderDataType::Vec3 },
			{"normal", ShaderDataType::Vec3},
			{"uv", ShaderDataType::Vec2},
			{"tangent", ShaderDataType::Vec3}
		});

		m_per_idx_buffer.set_layout({ { "Model", ShaderDataType::Mat4 }, {"Material", ShaderDataType::U32} }, 1, 4);

		m_draw_query = ecs.query_builder<const TransformComponent, const Model>()
			.build();

		Material def = { glm::vec3(0.8) };
		register_material(def);
	}

	~MeshBundle() {}


	MaterialHandle register_material(const Material& m) {
		m_materials.push_back(m);
		material_buffer.push_back(m.std140());
		return m_material_count++;
	}


	MeshHandle add_entry(Ref<Mesh> m) {
		uint32_t index = m_entries.size();

		uint32_t index_count = static_cast<uint32_t>(m->indices.size());
		uint32_t vertex_count = static_cast<uint32_t>(m->vertices.size());

		m_entries.push_back(Entry{ index_count, cumulative_idx_count, cumulative_vertex_count });

		cumulative_idx_count += index_count;
		cumulative_vertex_count += vertex_count;

		// Now add the indices to the index buffer
		m_index_buffer.extend(m->indices);

		std::vector<float> vertices;
		for (int i = 0; i < vertex_count; i++) {
			vertices.push_back(m->vertices[i].x);
			vertices.push_back(m->vertices[i].y);
			vertices.push_back(m->vertices[i].z);

			vertices.push_back(m->normals[i].x);
			vertices.push_back(m->normals[i].y);
			vertices.push_back(m->normals[i].z);

			if (m->uvs.size()) {
				vertices.push_back(m->uvs[i].x);
				vertices.push_back(m->uvs[i].y);
			}
			else {
				vertices.push_back(0);
				vertices.push_back(0);
			}

			if (m->tans.size()) {
				vertices.push_back(m->tans[i].x);
				vertices.push_back(m->tans[i].y);
				vertices.push_back(m->tans[i].z);
			}
			else {
				vertices.push_back(0);
				vertices.push_back(0);
				vertices.push_back(0);
			}
		}

		m_vertex_buffer.extend(vertices);

		return index;
	}


	void add_model_to_entity(flecs::entity& e, const Model& m) {
		e.set<Model>(m);
	}

	// TODO: take in a camera and generate vp
	inline void render(flecs::world& ecs, const Camera& camera) {
		std::vector<RenderCommand> command_list;
		std::vector<PerInstanceData> per_instance_data; // interleave mvp, model etc..

		const glm::mat4 vp = camera.projection * camera.view();

		m_rendered_tri_count = 0;

		uint32_t i = 0; // For instance count

		uint32_t base_instance = 0;

		m_draw_query.each([&](flecs::entity e, const TransformComponent& transform, const Model& model) {
			

			for (const auto& [mesh_handle, material_handle] : model.meshes) {
				auto& mesh = m_entries[mesh_handle];
				auto& material = m_materials[material_handle];

				if (!material.blend) {
					command_list.push_back({
						mesh.num_vertices,
						1,
						mesh.first_idx,
						mesh.base_vertex,
						i++
						});

					per_instance_data.push_back({ transform, material_handle });
					m_rendered_tri_count += mesh.num_vertices / 3;
				}
			}
		});


		static bool show_shader_config = true;

		if (ImGui::Begin("Renderer Stats")) {

			int total_memory = 0;
			int available_memory = 0;

			glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_memory);
			glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available_memory);


			ImGui::LabelText("Number of triangles: ", "%llu", m_rendered_tri_count);
			ImGui::LabelText("Number of  commands: ", "%llu", command_list.size());
			ImGui::LabelText("Available video mem:", "%d / %d MB", available_memory / 1024, total_memory / 1024);

			if (ImGui::Button("Show Shader Config")) {
				show_shader_config = true;
			}
		}

		if (show_shader_config) m_main_shader->show_imgui();

		ImGui::End();

		m_command_buffer.set_data(command_list);
		m_per_idx_buffer.set_data(per_instance_data);

		uint32_t uniform_block_binding_index = glGetUniformBlockIndex(m_main_shader->get_id(), "Materials");
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, material_buffer.get_id());
		glUniformBlockBinding(m_main_shader->get_id(), uniform_block_binding_index, 0);

		m_vertex_array.bind();
		m_command_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
		m_vertex_buffer.bind(0);
		m_per_idx_buffer.bind(1);
		m_index_buffer.bind();
		
		m_main_shader->uniforms["vp"].set<glm::mat4>(vp);
		m_main_shader->uniforms["camera_pos"].set<glm::vec3>(camera.position);
		m_main_shader->use();
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, command_list.size(), 0);
		

		GL_ERROR_CHECK()
	}

	inline uint32_t get_rendered_tri_count() {
		return m_rendered_tri_count;
	}

	struct Entry {
		uint32_t num_vertices;
		uint32_t first_idx;
		int32_t base_vertex;
	};

	Entry get_entry(uint32_t index) {
		return m_entries[index];
	}

private:
	uint32_t m_mesh_count = 0;
	uint32_t m_material_count = 0;


	uint64_t m_rendered_tri_count = 0;

	uint32_t cumulative_idx_count = 0; // the cumulative idx count until now
	int32_t cumulative_vertex_count = 0; // the cumulative vertex count

	std::vector<Entry> m_entries = {};
	std::vector<Material> m_materials = {};

	VertexArray m_vertex_array;

	VertexBuffer m_vertex_buffer;
	VertexBuffer m_per_idx_buffer;

	Buffer m_command_buffer;
	Buffer material_buffer;

	IndexBuffer m_index_buffer;

	flecs::query<const TransformComponent, const Model> m_draw_query;

	Ref<Shader> m_main_shader;
};