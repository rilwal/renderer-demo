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


inline bool within(float a, float b, float c) {
	return b >= a && b <= c;
}


// Adapted from https://bruop.github.io/frustum_culling/
inline bool test_aabb(glm::mat4 mvp, glm::vec3 min, glm::vec3 max) {
	glm::vec4 corners[8] = {
		{min.x, min.y, min.z, 1.0}, // x y z
		{max.x, min.y, min.z, 1.0}, // X y z
		{min.x, max.y, min.z, 1.0}, // x Y z
		{max.x, max.y, min.z, 1.0}, // X Y z

		{min.x, min.y, max.z, 1.0}, // x y Z
		{max.x, min.y, max.z, 1.0}, // X y Z
		{min.x, max.y, max.z, 1.0}, // x Y Z
		{max.x, max.y, max.z, 1.0}, // X Y Z
	};

	bool inside = false;

	for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {
		// Transform vertex
		glm::vec4 corner = mvp * corners[corner_idx];
		// Check vertex against clip space bounds
		inside = inside ||
			within(-corner.w, corner.x, corner.w) &&
			within(-corner.w, corner.y, corner.w) &&
			within(0.0f, corner.z, corner.w);
	}
	return inside;

}



// This is a tag structure to specify that a mesh is GPU Resident.
// We will use it for materials, lights and models
struct GPUResident {
	size_t addr; // This is probably an offset into a buffer, but usage depends on the object
};

struct Dirty {}; // Tag struct for dirty bois



// A buffer that keeps data in sync between the GPU and the ECS
template <typename Entity_Type, typename GPU_Type>
class ECSGPUBuffer {
public:
	// TODO: Investigate STATIC vs STREAM for this kind of data
	ECSGPUBuffer(std::function<GPU_Type(const flecs::entity&, const Entity_Type&)> convert) : m_buffer(BufferUsage::STATIC), m_convert(convert) {
		m_non_resident_query = ecs.query_builder<const WorldTransform, const Entity_Type>()
			.term<GPUResident, Entity_Type>().not_()
			.build();

		m_dirty_query = ecs.query_builder<const WorldTransform, const Entity_Type, const flecs::pair<GPUResident, Entity_Type>>().term<Dirty, Entity_Type>().build();

		// Any changes to GPU resident lights should cause them to be marked dirty
		m_observer = ecs.observer<const WorldTransform, const Entity_Type>().term<GPUResident, Entity_Type>().event(flecs::OnSet).each(
			[](flecs::entity e, const TransformComponent&, const Entity_Type&) {
				e.add<Dirty, Entity_Type>();
			});
	}

	~ECSGPUBuffer() {
		m_observer.destruct();
	}

	// Update the buffer!
	void update() {
		ecs.defer_begin();
		m_non_resident_query.each([&](flecs::entity e, const TransformComponent& transform, const Entity_Type& value) {
			// TODO: Check for locality etc. and don't make resident all lights all the time
			//			(or do, lights are small maybe??)
			//			(or don't, cache locality is worse with many lights???)
			glm::vec3 world_pos = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
			size_t addr = m_buffer.push_back(m_convert(e, value));
			e.set<GPUResident, Entity_Type>({ addr });
		});
		ecs.defer_end();


		ecs.defer_begin();
		m_dirty_query.each([&](flecs::entity e, const TransformComponent& transform, const Entity_Type& value, const GPUResident& resident) {
			glm::vec3 world_pos = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
			m_buffer.set_subdata(m_convert(e, value), resident.addr);

			e.remove<Dirty, Entity_Type>();
		});
		ecs.defer_end();
	}


	size_t get_address(flecs::entity e) {
		return e.has<GPUResident, Entity_Type>() ? e.get<GPUResident, Entity_Type>()->addr : 0;
	}

	size_t get_index(flecs::entity e) {
		return get_address(e) / sizeof(GPU_Type);
	}


	void bind(Ref<Shader> s, std::string name, int32_t storage_block_location) {
		uint32_t storage_block_index = glGetProgramResourceIndex(s->get_id(), GL_SHADER_STORAGE_BLOCK, name.c_str());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, storage_block_location, m_buffer.get_id());
		glShaderStorageBlockBinding(s->get_id(), storage_block_index, storage_block_location);
	}


private:
	std::function<GPU_Type(const flecs::entity&, const Entity_Type&)> m_convert; // convert to GPU Type
	Buffer m_buffer;
	flecs::query<const WorldTransform, const Entity_Type> m_non_resident_query;
	flecs::query<const WorldTransform, const Entity_Type, const flecs::pair<GPUResident, Entity_Type>> m_dirty_query;
	flecs::observer m_observer;
};



inline Light::Light_STD140 light_convert(const flecs::entity& e, const Light& light) {
	TransformComponent transform = *e.get<WorldTransform>();
	glm::vec3 world_pos = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
	return light.STD140(world_pos);
}

inline glm::mat4 transform_convert(const flecs::entity& e, const TransformComponent& transform) {
	return transform;	
}


// This represents a number of meshes in a single vertex buffer.
// TODO: Think about a more appropriate name? (maybe renderer lol)
// Limitations: All meshes must have the same vertex specification!!!
// Maybe template this by vertex spec somehow?
class MeshBundle {
public:
#pragma pack(push, 1)
	struct PerInstanceData {
		uint32_t model_idx;
		uint32_t material_idx;
	};
#pragma pack(pop)

	MeshBundle()
		: m_vertex_array(), m_vertex_buffer(m_vertex_array), m_per_idx_buffer(m_vertex_array, 1),
		m_command_buffer(BufferUsage::STREAM), m_draw_query(), m_main_shader(asset_manager.GetByPath<Shader>("assets/shaders/basic.glsl")), material_buffer(BufferUsage::STATIC),
		lights_buffer(light_convert), transform_buffer(transform_convert)
	{
		m_vertex_buffer.set_layout({
			{"position", ShaderDataType::Vec3 },
			{"normal", ShaderDataType::Vec3},
			{"uv", ShaderDataType::Vec2},
			{"tangent", ShaderDataType::Vec3}
		});

		m_per_idx_buffer.set_layout({ { "ModelIDX", ShaderDataType::U32 }, {"MaterialIDX", ShaderDataType::U32} }, 4);
		m_per_idx_buffer.set_per_instance(true);

		m_draw_query = ecs.query_builder<const WorldTransform, const Model>()
			.build();

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

		glm::vec3 min = {}, max = {}; // AABB

		// Now add the indices to the index buffer
		m_index_buffer.extend(m->indices);

		std::vector<float> vertices;
		for (uint32_t i = 0; i < vertex_count; i++) {
			if (m->vertices[i].x < min.x) min.x = m->vertices[i].x;
			if (m->vertices[i].y < min.y) min.y = m->vertices[i].y;
			if (m->vertices[i].z < min.z) min.z = m->vertices[i].z;

			if (m->vertices[i].x > max.x) max.x = m->vertices[i].x;
			if (m->vertices[i].y > max.y) max.y = m->vertices[i].y;
			if (m->vertices[i].z > max.z) max.z = m->vertices[i].z;

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

		m_entries.push_back(Entry{ index_count, cumulative_idx_count, cumulative_vertex_count, min, max });

		cumulative_idx_count += index_count;
		cumulative_vertex_count += vertex_count;

		m_vertex_buffer.extend(vertices);

		return index;
	}



	inline void render(const Camera& camera) {
		std::vector<RenderCommand> command_list;
		std::vector<PerInstanceData> per_instance_data; // interleave mvp, model etc..

		const glm::mat4 vp = camera.projection * camera.view();

		m_rendered_tri_count = 0;

		lights_buffer.update();
		transform_buffer.update();

		m_main_shader->uniforms["vp"].set<glm::mat4>(vp);
		m_main_shader->uniforms["camera_pos"].set<glm::vec3>(camera.position);
		m_main_shader->use();


		uint32_t storage_blocks = 0;


		uint32_t material_storage_block_index = glGetProgramResourceIndex(m_main_shader->get_id(), GL_SHADER_STORAGE_BLOCK, "Materials");
		int32_t material_storage_block_location = storage_blocks++;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, material_storage_block_location, material_buffer.get_id());
		glShaderStorageBlockBinding(m_main_shader->get_id(), material_storage_block_index, material_storage_block_location);

		lights_buffer.bind(m_main_shader, "Lights", storage_blocks++);
		transform_buffer.bind(m_main_shader, "Transforms", storage_blocks++);

		uint32_t i = 0; // For instance count

		uint32_t base_instance = 0;

		m_draw_query.each([&](flecs::entity e, const TransformComponent& transform, const Model& model) {
			const auto& [mesh_handle, material_handle] = model.mesh;
			auto& mesh = m_entries[mesh_handle];
			auto& material = m_materials[material_handle];

			if (!material.blend) {
				//glm::mat4 mvp = (glm::mat4)transform * vp;

				command_list.push_back({
					mesh.num_vertices,
					1,
					mesh.first_idx,
					mesh.base_vertex,
					i++
				});

				per_instance_data.push_back({ static_cast<uint32_t>(transform_buffer.get_index(e)), material_handle});
				m_rendered_tri_count += mesh.num_vertices / 3;

			}
		});

		
		m_command_buffer.set_data(command_list);
		m_per_idx_buffer.set_data(per_instance_data);		

		m_vertex_array.bind();
		m_command_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
		m_vertex_buffer.bind(0);
		m_per_idx_buffer.bind(1);
		m_index_buffer.bind();

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, static_cast<int32_t>(command_list.size()), 0);
		

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

		GL_ERROR_CHECK()
	}


	inline uint64_t get_rendered_tri_count() {
		return m_rendered_tri_count;
	}

	struct Entry {
		uint32_t num_vertices;
		uint32_t first_idx;
		int32_t base_vertex;
		glm::vec3 aabb_min;
		glm::vec3 aabb_max;
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

	Buffer m_per_instance_ssb;
	Buffer m_command_buffer;
	Buffer material_buffer;

	IndexBuffer m_index_buffer;

	flecs::query<const WorldTransform, const Model> m_draw_query;

	ECSGPUBuffer<Light, Light::Light_STD140> lights_buffer; 
	ECSGPUBuffer<WorldTransform, glm::mat4> transform_buffer;

	Ref<Shader> m_main_shader;

};