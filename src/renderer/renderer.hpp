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
#include <functional>

#include <glm.hpp>
#include "flecs.h"

#include "types.hpp"
#include "util.hpp"
#include "ecs_componets.hpp"

#include "assets/asset_manager.hpp"
#include "vertex_buffer.hpp"
#include "index_buffer.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "camera.hpp"
#include "light.hpp"


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


// This is a tag structure to specify that a mesh is GPU Resident.
// We will use it for materials, lights and models
struct GPUResident {
	size_t addr; // This is probably an offset into a buffer, but usage depends on the object
	bool is_dirty; // This should be set if any data has been modified that requires a re-upload
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
		: m_vertex_array(), m_vertex_buffer(m_vertex_array), m_per_idx_buffer(m_vertex_array, 1),
		m_command_buffer(), m_draw_query(), m_main_shader(asset_manager.GetByPath<Shader>("assets/shaders/basic.glsl")) {


		m_vertex_buffer.set_layout({
			{"position", ShaderDataType::Vec3 },
			{"normal", ShaderDataType::Vec3},
			{"uv", ShaderDataType::Vec2},
			{"tangent", ShaderDataType::Vec3}
		});

		m_per_idx_buffer.set_layout({ { "Model", ShaderDataType::Mat4 }, {"Material", ShaderDataType::U32} }, 4);
		m_per_idx_buffer.set_per_instance(true);

		m_draw_query = ecs.query_builder<const TransformComponent, const Model>()
			.build();

		m_light_query = ecs.query_builder<const TransformComponent, const Light>()
			.build();

		
		// Any changes to GPU resident lights should cause the dirty bit to be set
		ecs.observer<const TransformComponent, const Light, flecs::pair<GPUResident, Light>>().event(flecs::OnSet | flecs::OnAdd).each(
			[](flecs::entity e, const TransformComponent t, const Light l, GPUResident& gpu_resident) {
			gpu_resident.is_dirty = true;
		});


		Material def = { glm::vec3(0.8f) };
		register_material(def);
	}

	~MeshBundle() {}


	MaterialHandle register_material(const Material& m) {
		m_materials.push_back(m);
		material_buffer.push_back(m.std140());
		return m_material_count++;
	}


	MeshHandle add_entry(Ref<Mesh> m) {
		uint32_t index = static_cast<uint32_t>(m_entries.size());

		uint32_t index_count = static_cast<uint32_t>(m->indices.size());
		uint32_t vertex_count = static_cast<uint32_t>(m->vertices.size());

		m_entries.push_back(Entry{ index_count, cumulative_idx_count, cumulative_vertex_count });

		cumulative_idx_count += index_count;
		cumulative_vertex_count += vertex_count;

		// Now add the indices to the index buffer
		m_index_buffer.extend(m->indices);

		std::vector<float> vertices;
		for (uint32_t i = 0; i < vertex_count; i++) {
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
		

		ecs.defer_begin();
		m_light_query.each([&](flecs::entity e, const TransformComponent& transform, const Light& light) {
			if (e.has<GPUResident, Light>()) {
				GPUResident& resident = *e.get_mut<GPUResident, Light>();

				if (resident.is_dirty) {
					glm::vec3 world_pos = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
					m_light_buffer.set_subdata(light.STD140(world_pos), resident.addr);
				}
			}
			else {
				// TODO: Check for locality etc. and don't make resident all lights all the time
				//			(or do, lights are small maybe??)
				//			(or don't, cache locality is worse with many lights???)
				glm::vec3 world_pos = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
				size_t addr = m_light_buffer.push_back(light.STD140(world_pos));
				e.set<GPUResident, Light>({ addr, false });

			}
		});
		ecs.defer_end();


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

		uint32_t light_uniform_block_binding_index = glGetUniformBlockIndex(m_main_shader->get_id(), "Lights");
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_light_buffer.get_id());
		glUniformBlockBinding(m_main_shader->get_id(), light_uniform_block_binding_index, 1);

		m_vertex_array.bind();
		m_command_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
		m_vertex_buffer.bind(0);
		m_per_idx_buffer.bind(1);
		m_index_buffer.bind();
		
		m_main_shader->uniforms["vp"].set<glm::mat4>(vp);
		m_main_shader->uniforms["camera_pos"].set<glm::vec3>(camera.position);
		m_main_shader->use();
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, static_cast<int32_t>(command_list.size()), 0);
		

		GL_ERROR_CHECK()
	}


	inline uint64_t get_rendered_tri_count() {
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
	Buffer m_light_buffer;

	IndexBuffer m_index_buffer;

	flecs::query<const TransformComponent, const Model> m_draw_query;
	flecs::query<const TransformComponent, const Light> m_light_query;


	Ref<Shader> m_main_shader;
};