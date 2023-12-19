#pragma once

#include "asset_manager.hpp"

#include <glm.hpp>
#include "flecs.h"

#include "util.hpp"
#include "ecs_componets.hpp"

#include "vertex_buffer.hpp"
#include "index_buffer.hpp"
#include "shader.hpp"


// To be used only for e.g. primitve generation!
struct IntermediateMesh {
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
	};


#pragma pack(push,1)
	struct Triangle {
		uint32_t vert_idx[3];
	};
#pragma pack(pop)

	std::vector<Vertex> verts;
	std::vector<Triangle> tris;
};


struct Mesh : IAsset {
	Mesh();
	~Mesh();

	Mesh(Mesh&) = delete;
	Mesh(Mesh&&);

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> bitans;
	std::vector<glm::vec4> tans;


	bool has_normals = false;
	bool has_uvs = false;
	bool has_tangents = false;
	bool has_bitangents = false;


	std::vector<uint32_t> indices;


	void load_from_obj_string(const char* src);

	void load_from_file(const char* filename) override;
	void load_from_intermediate_mesh(const IntermediateMesh& im);
	

	void reload() override;
	void unload() override;


	

private:

};


struct RenderCommand {
	uint32_t count;
	uint32_t instance_count;
	uint32_t first_index;
	int32_t base_vertex;
	uint32_t base_instance;
};



Ref<Mesh> construct_quad_mesh(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
Ref<Mesh> construct_cube_mesh(float size);
Ref<Mesh> construct_cube_sphere(float size, int subdivisions);
