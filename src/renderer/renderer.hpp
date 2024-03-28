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
#include "framebuffer.h"

#include "meshoptimizer.h"


#include "instrumentation/instrumentor.hpp"


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
	uint32_t addr; // This is probably an offset into a buffer, but usage depends on the object
};

struct Dirty {}; // Tag struct for dirty bois



// A buffer that keeps data in sync between the GPU and the ECS
template <typename Entity_Type, typename GPU_Type>
class ECSGPUBuffer : public Buffer {
public:
	// TODO: Investigate STATIC vs STREAM for this kind of data
	ECSGPUBuffer(std::function<GPU_Type(const flecs::entity&, const Entity_Type&)> convert) : Buffer(BufferUsage::STATIC), m_convert(convert) {
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
			uint32_t addr = static_cast<uint32_t>(push_back(m_convert(e, value)));
			e.set<GPUResident, Entity_Type>({ addr });
		});
		ecs.defer_end();


		ecs.defer_begin();
		m_dirty_query.each([&](flecs::entity e, const TransformComponent& transform, const Entity_Type& value, const GPUResident& resident) {
			glm::vec3 world_pos = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
			set_subdata(m_convert(e, value), resident.addr);

			e.remove<Dirty, Entity_Type>();
		});
		ecs.defer_end();
	}


	uint32_t get_address(flecs::entity e) {
		return e.has<GPUResident, Entity_Type>() ? e.get<GPUResident, Entity_Type>()->addr : 0;
	}

	uint32_t get_index(flecs::entity e) {
		return get_address(e) / sizeof(GPU_Type);
	}


private:
	std::function<GPU_Type(const flecs::entity&, const Entity_Type&)> m_convert; // convert to GPU Type
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


inline uint32_t Pack_INT_2_10_10_10_REV(float x, float y, float z, float w)
{
	const uint32_t xs = x < 0;
	const uint32_t ys = y < 0;
	const uint32_t zs = z < 0;
	const uint32_t ws = w < 0;
	uint32_t vi =
		ws << 31 | ((uint32_t)(w + (ws << 1)) & 1) << 30 |
		zs << 29 | ((uint32_t)(z * 511 + (zs << 9)) & 511) << 20 |
		ys << 19 | ((uint32_t)(y * 511 + (ys << 9)) & 511) << 10 |
		xs << 9 | ((uint32_t)(x * 511 + (xs << 9)) & 511);
	return vi;
}


inline uint32_t Pack_INT_2_10_10_10_REV(glm::vec4 val) {
	return Pack_INT_2_10_10_10_REV(val.x, val.y, val.z, val.w);
}


inline uint32_t Pack_INT_2_10_10_10_REV(glm::vec3 val) {
	return Pack_INT_2_10_10_10_REV(val.x, val.y, val.z, 0.0f);
}


// The different depth check modes
enum class DepthCheckMode {
	Off,
	Always,
	Never,
	Equal,
	Less,
	LessEqual,
	Greater,
	GreaterEqual
};

// A struct to represent the state of the render pipeline
// Will expand as I need it
struct RenderPipelineState {
	DepthCheckMode m_depth_check_mode = DepthCheckMode::Less;

	void apply() {
		// Use static variables to keep track of current pipeline state.
		// Consider static RenderPipelineState current_state?
		static DepthCheckMode s_current_depth_check_mode = DepthCheckMode::Less;

		if (s_current_depth_check_mode != m_depth_check_mode) {
			using enum DepthCheckMode;

			if (s_current_depth_check_mode == Off) {
				glEnable(GL_DEPTH_TEST);
			}

			switch (m_depth_check_mode) {
			case Off:
				glDisable(GL_DEPTH_TEST);
				break;
			case Always:
				glDepthFunc(GL_ALWAYS);
				break;
			case Never:
				glDepthFunc(GL_NEVER);
				break;
			case Equal:
				glDepthFunc(GL_EQUAL);
				break;
			case Less:
				glDepthFunc(GL_LESS);
				break;
			case LessEqual:
				glDepthFunc(GL_LEQUAL);
				break;
			case Greater:
				glDepthFunc(GL_GREATER);
				break;
			case GreaterEqual:
				glDepthFunc(GL_GEQUAL);
				break;

			}

			s_current_depth_check_mode = m_depth_check_mode;
		}
	}
};


// A class to hold render passes, and run them when possible
class RenderGraph {
public:

private:

};

// Superclass "interface" for render passes
class RenderPass {
public:

	virtual void execute() = 0;

	RenderPipelineState m_pipeline_state;
private:

};

using NamedBuffer = std::pair<std::string, Ref<Buffer>>;

// Rasterize some geometry 
class RasterPass : public RenderPass {
public:
	RasterPass(Ref<Camera> camera, Ref<Shader> shader, Ref<VertexArray> vao, std::vector<Ref<VertexBuffer>> vbos, std::vector<NamedBuffer> ssbos)
		: m_camera(camera), m_shader(shader), m_vao(vao), m_vbos(vbos), m_ssbos(ssbos), RenderPass() {

	}

	void execute() override {
		m_pipeline_state.apply();

		for (int i = 0; i < m_vbos.size(); i++) {
			m_vbos[i]->bind(i);
		}

		//m_shader->use();
		
		glUseProgram(m_shader->get_id());

		//bind_ssbos();
		apply_camera();

		m_vao->bind();

		// TODO: Pass in render commands
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, 3, 0);

	}

private:

	void bind_ssbos() {
		// TODO: Check for stale buffers somehow, and don't bind them otherwise!
		uint32_t binding_location = 0;

		for (auto& [name, buffer] : m_ssbos) {

			buffer->bind(GL_SHADER_STORAGE_BUFFER, binding_location++);

			uint32_t binding_index = glGetProgramResourceIndex(m_shader->get_id(), GL_SHADER_STORAGE_BLOCK, name.c_str());

			if (binding_index != GL_INVALID_INDEX) {
				glShaderStorageBlockBinding(m_shader->get_id(), binding_index, binding_location);
			}
			
		}
	}

	void apply_camera() {
		glm::mat4 vp = m_camera->projection() * m_camera->view();
		//m_shader->uniforms["vp"].set(vp);
		//m_shader->uniforms["camera_pos"].set(m_camera->position);
		glUniformMatrix4fv(m_shader->uniforms["vp"].location, 1, false, glm::value_ptr(vp));
		glUniform3fv(m_shader->uniforms["camera_pos"].location, 1, glm::value_ptr(m_camera->position));
	}


	// If this is true, draw data should be a buffer, otherwise it comes in as a pointer
	bool m_indirect;

	Ref<Camera> m_camera;
	Ref<Shader> m_shader;

	// TODO: Consider making MeshBundle work how it sounds like it should (dumber), and use that here!
	Ref<VertexArray> m_vao;
	Ref<IndexBuffer> m_idx; // TODO: Make buffers good enough we don't need this!
	std::vector<Ref<VertexBuffer>> m_vbos;

	std::vector<NamedBuffer> m_ssbos;
};

/*
// Runs a compute shader
class ComputePass : public RenderPass {
public:

private:

};


/*
// Run a fullscreen pass by rasterizing
// In some scenarios a compute shader may be better!
class FullscreenPass : public RenderPass {
public:

private:

};
*/



// This represents a number of meshes in a single vertex buffer.
// TODO: Think about a more appropriate name? (maybe renderer lol)
// Limitations: All meshes must have the same vertex specification!!!
// Maybe template this by vertex spec somehow?
class MeshBundle {
public:
	struct Entry {
		uint32_t num_vertices;
		uint32_t first_idx;
		int32_t base_vertex;
		glm::vec3 aabb_min;
		glm::vec3 aabb_max;

		float bounding_sphere;
		uint32_t idx;
	};

#pragma pack(push, 1)
	struct PerInstanceData {
		uint32_t transform_idx;
		uint32_t material_idx;
	};

	// Represents a renderable entity on the GPU
	struct GPUEntity {
		uint32_t mesh_idx;		// 4 bytes
		uint32_t material_idx;	// 4 bytes
		uint32_t transform_idx;	// 4 bytes
		uint32_t padding;		// 4 bytes
	};

	// Represents a mesh on the GPU
	struct GPUMesh {
		uint32_t num_vertices;
		uint32_t first_idx;
		int32_t base_vertex;
		float bounding_sphere;
	};

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 tan;
	};

	// TODO: Make this magical (auto quantization from some spec)
	struct QuantizedVertex {
		uint16_t position[3]; // halfs
		int32_t normal;
		uint16_t uv[2];
		int32_t tan; // Let's try 8-bit UNORMs?
	};
#pragma pack(pop)

	MeshBundle()
		: m_vertex_array(make_ref<VertexArray>()), m_vertex_buffer(std::make_shared<VertexBuffer>(m_vertex_array->get_id())), m_per_idx_buffer(std::make_shared<VertexBuffer>(m_vertex_array->get_id(), 1)),
		m_command_buffer(std::make_shared<Buffer>(BufferUsage::STREAM)), m_draw_query(), m_main_shader(asset_manager.GetByPath<Shader>("assets/shaders/basic.glsl")), material_buffer(std::make_shared<Buffer>(BufferUsage::STATIC)),
		lights_buffer(std::make_shared<ECSGPUBuffer<Light, Light::Light_STD140>>(light_convert)), m_transform_buffer(std::make_shared<Buffer>(BufferUsage::STREAM)), m_mesh_buffer(std::make_shared<Buffer>(BufferUsage::STATIC)),
		m_render_intermediate_buffer(std::make_shared<Buffer>(BufferUsage::STREAM)), m_framebuffer(1920, 1080), m_entity_buffer(std::make_shared<Buffer>(BufferUsage::STREAM))
	{

		m_vertex_buffer->set_layout({
			{"position", ShaderDataType::F16, 3 },
			{"normal", ShaderDataType::UNORM1010102, 1},
			{"uv", ShaderDataType::UNORM16, 2},
			{"tangent", ShaderDataType::UNORM1010102, 1}
		});

		m_per_idx_buffer->set_layout({ { "ModelIDX", ShaderDataType::U32 }, {"MaterialIDX", ShaderDataType::U32} }, 4);
		m_per_idx_buffer->set_per_instance(true);

		m_non_resident_transform_query = ecs.query_builder<const WorldTransform>().term<Model>().term<GPUResident, WorldTransform>().not_().build();
		m_dirty_transform_query = ecs.query_builder<const WorldTransform, const flecs::pair<GPUResident, WorldTransform>>().term<Dirty, WorldTransform>().build();

		m_draw_query = ecs.query_builder<const flecs::pair<GPUResident, WorldTransform>, const Model>()
			.build();

		m_non_resident_entity_query = ecs.query_builder<const flecs::pair<GPUResident, WorldTransform>, const Model>()
			.term<GPUResident>().not_()
			.build();


		ecs.observer<const WorldTransform>().term<const flecs::pair<GPUResident, WorldTransform>>().event(flecs::OnSet).each(
			[](flecs::entity e, const WorldTransform& wt) {
				e.add<Dirty, WorldTransform>();
		});

		Material def = { glm::vec3(0.8f) };
		register_material(def);

	}

	~MeshBundle() {}


	MaterialHandle register_material(const Material& m) {
		m_materials.push_back(m);
		material_buffer->push_back(m.std140());
		return m_material_count++;
	}


	MeshHandle add_entry(Ref<Mesh> m) {
		uint32_t index = static_cast<uint32_t>(m_entries.size());

		uint32_t index_count = static_cast<uint32_t>(m->indices.size());
		uint32_t vertex_count = static_cast<uint32_t>(m->vertices.size());

		glm::vec3 min = {}, max = {}; // AABB
		float bounding_sphere = 0;


		std::vector<Vertex> vertices;
		for (uint32_t i = 0; i < vertex_count; i++) {
			auto& vertex = m->vertices[i];
			if (vertex.x < min.x) min.x = vertex.x;
			if (vertex.y < min.y) min.y = vertex.y;
			if (vertex.z < min.z) min.z = vertex.z;

			if (vertex.x > max.x) max.x = vertex.x;
			if (vertex.y > max.y) max.y = vertex.y;
			if (vertex.z > max.z) max.z = vertex.z;

			// TODO: use length ^ 2 and just square root at hte end?
			//			Also, asset conditioning!!!
			if (glm::length(vertex) > bounding_sphere) bounding_sphere = glm::length(vertex);

			Vertex v;

			v.position = vertex;
			v.normal = m->normals[i];
			
			if (m->uvs.size()) {
				v.uv = m->uvs[i];
			}

			if (m->tans.size()) {
				v.tan = m->tans[i];
			}

			vertices.push_back(v);
		}

		// remap code, for now let's skip since our models already seem good!
		if (0) {
			std::vector<unsigned int> remap(index_count); // allocate temporary memory for the remap table
			size_t remap_vertex_count = meshopt_generateVertexRemap(remap.data(), m->indices.data(), m->indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));

			std::vector<Vertex> reindexed_vertices;
			std::vector<uint32_t> reindexed_indices;
			reindexed_vertices.resize(remap_vertex_count);
			reindexed_indices.resize(index_count);

			meshopt_remapIndexBuffer(reindexed_indices.data(), m->indices.data(), index_count, remap.data());
			meshopt_remapVertexBuffer(reindexed_vertices.data(), vertices.data(), vertex_count, sizeof(Vertex), remap.data());
		}

		std::vector<uint32_t> indices = m->indices;

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
		meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &vertices[0].position.x, vertex_count, sizeof(Vertex), 1.05f);
		
		meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));




		std::vector<QuantizedVertex> quantized_vertices;
		quantized_vertices.resize(vertex_count);

		for (int i = 0; i < vertex_count; i++) {
			quantized_vertices[i].position[0] = meshopt_quantizeHalf(vertices[i].position.x);
			quantized_vertices[i].position[1] = meshopt_quantizeHalf(vertices[i].position.y);
			quantized_vertices[i].position[2] = meshopt_quantizeHalf(vertices[i].position.z);

			quantized_vertices[i].normal = Pack_INT_2_10_10_10_REV(vertices[i].normal);

			quantized_vertices[i].uv[0] = meshopt_quantizeUnorm(vertices[i].uv.x, 16); //vertices[i].uv;
			quantized_vertices[i].uv[1] = meshopt_quantizeUnorm(vertices[i].uv.y, 16); //vertices[i].uv;

			quantized_vertices[i].tan = Pack_INT_2_10_10_10_REV(vertices[i].tan);
		}


		// Now add the indices to the index buffer
		m_index_buffer.extend(indices);

		m_entries.push_back(Entry{ index_count, cumulative_idx_count, cumulative_vertex_count, min, max, bounding_sphere, index });

		GPUMesh gpu_mesh = { .num_vertices=index_count, .first_idx=cumulative_idx_count, .base_vertex=cumulative_vertex_count, .bounding_sphere=bounding_sphere };
		m_mesh_buffer->push_back(gpu_mesh);

		cumulative_idx_count += index_count;
		cumulative_vertex_count += vertex_count;

		m_vertex_buffer->extend(quantized_vertices);

		return index;
	}

	template <uint32_t num_shaders, uint32_t num_buffers>
	inline void setup_shaders(std::array<Ref<Shader>, num_shaders> shaders, std::array<std::string, num_buffers> buffer_names, std::array<Ref<Buffer>, num_buffers> buffers) {
		PROFILE_FUNC();
		// Binds all the buffers and uniforms for shaders
		// TODO: figure out what to do if these buffer ids change!

		uint32_t binding_location = 0;

		for (uint32_t i = 0; i < num_buffers; i++) {
			buffers[i]->bind(GL_SHADER_STORAGE_BUFFER, i);

			for (uint32_t j = 0; j < num_shaders; j++) {
				uint32_t binding_index = glGetProgramResourceIndex(shaders[j]->get_id(), GL_SHADER_STORAGE_BLOCK, buffer_names[i].c_str());
				
				if (binding_index != GL_INVALID_INDEX) {
					glShaderStorageBlockBinding(shaders[j]->get_id(), binding_index, i);
				}
			}
		}

	}


	inline void render(Camera& camera) {
		// TODO:
		//	* Use a UBO for frame constants like camera parameters

		PROFILE_FUNC();

		//m_framebuffer.bind();

		//m_framebuffer.clear_color({ .5f, .6f, .7f, 1.f });
		//m_framebuffer.clear_depth();
		

		auto shader_update = [&]() {
			setup_shaders<5, 8>({ m_entity_count_shader, m_build_render_command_shader, m_generate_per_instance_data_shader, m_z_prepass_shader, m_main_shader },
				{ "RenderData", "Entities", "Meshes", "RenderCommands", "PerInstance", "Transforms", "Lights", "Materials" },
				{ m_render_intermediate_buffer, m_entity_buffer, m_mesh_buffer, m_command_buffer, m_per_idx_buffer, m_transform_buffer, lights_buffer, material_buffer });
		};


		static int renderer = 0;
		const glm::mat4 vp = camera.projection() * camera.view();

		lights_buffer->update();

		size_t non_resident_transforms = m_non_resident_transform_query.count();

		if (non_resident_transforms > 0) {
			puts(std::format("Making {} non resident transforms resident", non_resident_transforms).c_str());

			ecs.defer_begin();
			m_non_resident_transform_query.each([&](flecs::entity e, const TransformComponent& transform) {
				size_t addr = m_transform_buffer->push_back(transform.transform);
				uint32_t idx = static_cast<uint32_t>(addr / sizeof(glm::mat4));
				e.set<GPUResident, WorldTransform>({ idx });
				});
			ecs.defer_end();

			// chance that buffer re-allocated
			// todo: actually update only when required!
			shader_update();
		}

		size_t dirty_transforms = m_dirty_transform_query.count();

		if (dirty_transforms > 0) {
			puts(std::format("Updaing {} dirty transforms", dirty_transforms).c_str());

			ecs.defer_begin();
			m_dirty_transform_query.each([&](flecs::entity e, const TransformComponent& transform, const GPUResident& gr) {
				m_transform_buffer->set_subdata<glm::mat4>(transform.transform, gr.addr * sizeof(glm::mat4));
				e.remove<Dirty, WorldTransform>();
			});
			ecs.defer_end();

			// chance that buffer re-allocated
			// todo: actually update only when required!
			shader_update();
		}

		size_t non_resident_entities = m_non_resident_entity_query.count();

		if (non_resident_entities > 0) {
			puts(std::format("Making {} non resident entities resident", non_resident_entities).c_str());

			ecs.defer_begin();
			m_non_resident_entity_query.each([&](flecs::entity e, const GPUResident& transform, const Model& model) {
				const auto& [mesh_handle, material_handle] = model.mesh;
				auto& mesh = m_entries[mesh_handle];
				auto& material = m_materials[material_handle];
				uint32_t idx = 0;

				if (!material.blend) {
					size_t address = m_entity_buffer->push_back(GPUEntity{
						.mesh_idx = mesh.idx,
						.material_idx = material_handle,
						.transform_idx = static_cast<uint32_t>(transform.addr),
						.padding = 12345
						});

					idx = static_cast<uint32_t>(address / sizeof(GPUEntity));
				}
				e.set<GPUResident>({ idx });
				});
			ecs.defer_end();

			// chance that buffer re-allocated
			// todo: actually update only when required!
			shader_update();
		}

		uint32_t draw_count = m_draw_query.count();

		m_command_buffer->resize(sizeof(RenderCommand) * m_entries.size());
		m_per_idx_buffer->resize(sizeof(PerInstanceData) * draw_count);

		m_render_intermediate_buffer->resize((m_entries.size() * 2 + 1) * sizeof(uint32_t));
		constexpr uint32_t zero = 0;
		glClearNamedBufferData(m_render_intermediate_buffer->get_id(), GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);

		m_entity_count_shader->uniforms["num_entities"].set<uint32_t>(draw_count);
		m_entity_count_shader->use();

		glDispatchCompute((draw_count + 32) / 32, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		m_build_render_command_shader->uniforms["num_models"].set<uint32_t>(m_entries.size());
		m_build_render_command_shader->use();

		glDispatchCompute((m_entries.size() + 32) / 32, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		
		m_generate_per_instance_data_shader->uniforms["num_entities"].set<uint32_t>(draw_count);
		m_generate_per_instance_data_shader->use();

		glDispatchCompute((draw_count + 32) / 32, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_command_buffer->bind(GL_DRAW_INDIRECT_BUFFER);
		m_index_buffer.bind();

		RasterPass z_prepass(std::make_shared<Camera>(camera), m_z_prepass_shader, m_vertex_array, { m_vertex_buffer, m_per_idx_buffer }, { {"Transforms", m_transform_buffer} });

		z_prepass.m_pipeline_state.m_depth_check_mode = DepthCheckMode::Less;

		RasterPass main_pass(std::make_shared<Camera>(camera), m_main_shader, m_vertex_array, { m_vertex_buffer, m_per_idx_buffer },
			{
				{"Transforms", m_transform_buffer},
				{"Lights", lights_buffer},
				{"Materials", material_buffer}
			}
		);

		main_pass.m_pipeline_state.m_depth_check_mode = m_z_prepass_enabled ? DepthCheckMode::Equal : DepthCheckMode::Less;
		
		if (m_z_prepass_enabled) {
			z_prepass.execute();
		}

		main_pass.execute();
		
		//m_framebuffer.unbind();

		static bool show_shader_config = true;

		if (ImGui::Begin("Renderer Stats")) {

			int total_memory = 0;
			int available_memory = 0;

			glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_memory);
			glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available_memory);

			ImGui::SliderInt("Renderer", &renderer, 0, 1);

			//ImGui::LabelText("Number of  commands: ", "%llu", command_list.size());
			ImGui::LabelText("Available video mem:", "%d / %d MB", available_memory / 1024, total_memory / 1024);

			ImGui::Checkbox("Z Prepass", &m_z_prepass_enabled);

			if (ImGui::Button("Show Shader Config")) {
				show_shader_config = true;
			}
		}

		if (show_shader_config) m_main_shader->show_imgui();

		ImGui::End();

		GL_ERROR_CHECK()
	}




	Entry get_entry(uint32_t index) {
		return m_entries[index];
	}

private:
	uint32_t m_mesh_count = 0;
	uint32_t m_material_count = 0;


	uint32_t cumulative_idx_count = 0; // the cumulative idx count until now
	int32_t cumulative_vertex_count = 0; // the cumulative vertex count

	std::vector<Entry> m_entries = {};
	std::vector<Material> m_materials = {};
	
	Ref<VertexArray> m_vertex_array;

	// TODO: these should probably not be refs,
	//			but instead this class should be passed by ref!
	Ref<VertexBuffer> m_vertex_buffer;
	Ref<VertexBuffer> m_per_idx_buffer;

	Ref<Buffer> m_command_buffer;
	Ref<Buffer> material_buffer;
	Ref<Buffer> m_render_intermediate_buffer;
	
	Ref<Buffer> m_transform_buffer;

	Ref<Buffer> m_mesh_buffer;
	Ref<Buffer> m_entity_buffer;

	Framebuffer m_framebuffer;

	IndexBuffer m_index_buffer;

	flecs::query<const flecs::pair<GPUResident, WorldTransform>, const Model> m_draw_query;
	flecs::query<const WorldTransform> m_non_resident_transform_query;
	flecs::query<const WorldTransform, const flecs::pair<GPUResident, WorldTransform>> m_dirty_transform_query;
	flecs::query<const flecs::pair<GPUResident, WorldTransform>, const Model> m_non_resident_entity_query;
	//flecs::query<const WorldTransform, const flecs::pair<GPUResident, WorldTransform>> m_dirty_transform_query;

	bool m_z_prepass_enabled = true;

	Ref<ECSGPUBuffer<Light, Light::Light_STD140>> lights_buffer; 

	Ref<Shader> m_main_shader;

	Ref<Shader> m_z_prepass_shader = asset_manager.GetByPath<Shader>("assets/shaders/z_prepass.glsl");

	Ref<Shader> m_entity_count_shader = asset_manager.GetByPath<Shader>("assets/shaders/entity_count.glsl");
	Ref<Shader> m_build_render_command_shader = asset_manager.GetByPath<Shader>("assets/shaders/build_render_command.glsl");
	Ref<Shader> m_generate_per_instance_data_shader = asset_manager.GetByPath<Shader>("assets/shaders/generate_per_instance_data.glsl");
};

