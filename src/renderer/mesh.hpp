#pragma once

#include "asset_manager.hpp"

#include <glm.hpp>

#include "util.hpp"
#include "vertex_buffer.hpp"


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
	std::vector<glm::vec3> tans;


	bool has_normals = false;
	bool has_uvs = false;
	bool has_tangents = false;
	bool has_bitangents = false;


	std::vector<uint32_t> indices;


	void load_from_obj_string(const char* src);

	void load_from_file(const char* filename) override;
	void load_from_intermediate_mesh(const IntermediateMesh& im);
	

	void upload_gl_data();


	void reload() override;
	void unload() override;

	void bind_vbo();

	int32_t vao_id() { return m_vbo->vao_id(); }
	int32_t vbo_id() { return m_vbo->vbo_id(); }
	
	VertexBuffer& vbo() { return *m_vbo; } // FIXME: Danger Danger!

private:
	std::unique_ptr<VertexBuffer> m_vbo;

};


Ref<Mesh> construct_quad_mesh(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
Ref<Mesh> construct_cube_mesh(float size);