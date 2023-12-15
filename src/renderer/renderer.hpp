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

#include <glm.hpp>
#include "flecs.h"

#include "util.hpp"
#include "ecs_componets.hpp"

#include "asset_manager.hpp"
#include "vertex_buffer.hpp"
#include "index_buffer.hpp"
#include "shader.hpp"
#include "mesh.hpp"


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


struct MeshTag { };

struct Dirty {};

// This represents a number of meshes in a single vertex buffer.
// TODO: Think about a more appropriate name? (maybe renderer lol)
// Limitations: All meshes must have the same vertex specification!!!
// Maybe template this by vertex spec somehow?
class MeshBundle {
public:
	using Handle = uint32_t;

	MeshBundle(flecs::world& ecs)
		: m_vertex_array(), m_vertex_buffer(m_vertex_array), m_per_idx_buffer(m_vertex_array),
		m_command_buffer(), m_draw_query(), m_main_shader(asset_manager.GetByPath<Shader>("assets/shaders/basic.glsl")),
		m_z_prepass_shader(asset_manager.GetByPath<Shader>("assets/shaders/z_prepass.glsl")) {
		// For now we are hardcoding position,normal.
		// And MVP, Model (testing just Model!)
		// TODO: Fix that!!!

		m_vertex_buffer.set_layout({ {"position", ShaderDataType::Vec3 }, {"normal", ShaderDataType::Vec3} });
		m_per_idx_buffer.set_layout({ { "Model", ShaderDataType::Mat4 } }, 1, 2);

		m_draw_query = ecs.query_builder<const Position, const Rotation, const Scale, const MeshComponent>()
			.group_by<const MeshTag>()
			.build();

		ecs.observer<const Position, const Rotation, const Scale, const MeshComponent>().event(flecs::OnSet).each(
			[](flecs::entity e, const Position&, const Rotation&, const Scale&, const MeshComponent&) {
				e.add<Dirty>();
			});
	}

	~MeshBundle() {}

	inline uint32_t add_entry(Ref<Mesh> m) {
		uint32_t index = m_entries.size();

		uint32_t index_count = static_cast<uint32_t>(m->indices.size());
		uint32_t vertex_count = static_cast<uint32_t>(m->vertices.size());

		m_entries.push_back(Entry{ index_count, cumulative_idx_count, cumulative_vertex_count });

		cumulative_idx_count += index_count;
		cumulative_vertex_count += vertex_count;

		// Now add the indices to the index buffer
		m_index_buffer.extend(m->indices);

		// Using tuples here as it seems easier to extend to other vertex specs in the future
		std::vector<glm::vec3> vertices;
		for (int i = 0; i < vertex_count; i++) {
			vertices.push_back(m->vertices[i]);
			vertices.push_back(m->normals[i]);
		}

		m_vertex_buffer.extend(vertices);

		flecs::entity tag = ecs.entity();
		m_model_tags[index] = tag;

		return index;
	}

	void add_mesh_to_entity(flecs::entity& e, uint32_t mesh_handle) {
		e.add<MeshTag>(m_model_tags[mesh_handle]);
		e.set<MeshComponent>({ mesh_handle });
	}

	// TODO: take in a camera and generate vp
	inline void render(flecs::world& ecs, const glm::mat4 vp) {
		std::vector<RenderCommand> command_list;
		//std::vector<glm::mat4> matrix_data; // interleave mvp, model etc..

		m_rendered_tri_count = 0;

		uint32_t i = 0; // For instance count
		uint32_t changed_count = 0;

		uint32_t batch_count = 0;
		uint32_t current_batch = -1;
		uint32_t base_instance = 0;

		RenderCommand current_command = {};
		Entry* current_mesh = nullptr;

		ecs.defer_begin();
		m_draw_query.each([&](flecs::entity e, const Position& p, const Rotation& r, const Scale& s, const MeshComponent& m) {
			if (m != current_batch) {
				// start a new batch!
				if (current_mesh) {
					command_list.push_back({
						current_mesh->num_vertices,
						batch_count,
						current_mesh->first_idx,
						current_mesh->base_vertex,
						base_instance
						});
				}

				base_instance = i;
				current_batch = m;
				batch_count = 0;
				current_mesh = &m_entries[m];
			}

			if (e.has<Dirty>()) {
				changed_count++;

				glm::mat4 model = p.mat4() * r.mat4() * s.mat4();

				m_per_idx_buffer.set_subdata(&model, i * sizeof(glm::mat4), sizeof(glm::mat4));

				e.remove<Dirty>();
			}

			batch_count++;
			m_rendered_tri_count += current_mesh->num_vertices / 3;
			i++;
			});
		ecs.defer_end();


		// start a new batch!
		if (current_mesh) {
			command_list.push_back({
				current_mesh->num_vertices,
				batch_count,
				current_mesh->first_idx,
				current_mesh->base_vertex,
				base_instance
				});
		}

		static bool z_prepass = false;

		if (ImGui::Begin("Renderer Stats")) {
			static uint32_t last_entity_change_count = 0;

			if (changed_count != 0)
				last_entity_change_count = changed_count;

			ImGui::LabelText("Number of triangles: ", "%llu", m_rendered_tri_count);
			ImGui::LabelText("Last entity update #: ", "%llu", last_entity_change_count);
			ImGui::LabelText("Number of  commands: ", "%llu", command_list.size());

			ImGui::Checkbox("Z Prepass", &z_prepass);

		}

		ImGui::End();

		m_command_buffer.set_data(command_list);
		//m_per_idx_buffer.set_data(matrix_data);

		m_vertex_array.bind();
		m_command_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
		m_vertex_buffer.bind(0);
		m_per_idx_buffer.bind(1);
		m_index_buffer.bind();

		if (z_prepass) {
			m_z_prepass_shader->uniforms["vp"].set(vp);
			m_main_shader->uniforms["vp"].set(vp);

			glColorMask(0, 0, 0, 0);
			glDepthMask(1);
			glDepthFunc(GL_LESS);

			m_z_prepass_shader->use();
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, command_list.size(), 0);
			GL_ERROR_CHECK()

				glColorMask(1, 1, 1, 1);
			glDepthFunc(GL_EQUAL);
			glDepthMask(0);

			m_main_shader->use();
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, command_list.size(), 0);

			glDepthMask(1);
		}
		else {
			m_main_shader->uniforms["vp"].set(vp);
			m_main_shader->use();
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, command_list.size(), 0);
		}

		GL_ERROR_CHECK()
	}

	inline uint32_t get_rendered_tri_count() {
		return m_rendered_tri_count;
	}


private:
	struct Entry {
		uint32_t num_vertices;
		uint32_t first_idx;
		int32_t base_vertex;
	};

	std::map<uint32_t, flecs::entity> m_model_tags;

	uint64_t m_rendered_tri_count = 0;

	uint32_t cumulative_idx_count = 0; // the cumulative idx count until now
	int32_t cumulative_vertex_count = 0; // the cumulative vertex count

	std::vector<Entry> m_entries = {};

	VertexArray m_vertex_array;

	VertexBuffer m_vertex_buffer;
	VertexBuffer m_per_idx_buffer;

	Buffer m_command_buffer;

	IndexBuffer m_index_buffer;

	flecs::query<const Position, const Rotation, const Scale, const MeshComponent> m_draw_query;

	Ref<Shader> m_main_shader;
	Ref<Shader> m_z_prepass_shader;
};