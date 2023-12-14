
#include "mesh.hpp"

#include "util.hpp"

#include <unordered_map>

Mesh::Mesh() : IAsset("Mesh") {
	m_vbo = std::make_unique<VertexBuffer>();
}

Mesh::~Mesh() {
}

Mesh::Mesh(Mesh&& other) : IAsset("Mesh") {
	m_vbo = std::move(other.m_vbo);
}

namespace std {
	template <>
	struct hash<IntermediateMesh::Vertex>
	{
		std::size_t operator()(const IntermediateMesh::Vertex& v) const
		{
			size_t res = hash<uint64_t>()(((uint64_t*)&v)[0]);
			res ^= hash<uint64_t>()(((uint64_t*)&v)[1]);
			res ^= hash<uint64_t>()(((uint64_t*)&v)[2]);

			return res;
		}
	};
}


bool operator==(const IntermediateMesh::Vertex& a, const IntermediateMesh::Vertex& b) {
	return a.position == b.position && a.normal == b.normal;
}


IntermediateMesh merge(const IntermediateMesh& a, const IntermediateMesh& b) {
	IntermediateMesh m;
	uint16_t idx = 0;

	// First merge the vertices and create a mapping from vertex to index
	std::unordered_map<IntermediateMesh::Vertex, uint16_t> idx_map;

	// Start with the existing vertices
	// We will deduplicate while we're here just in case
	// TODO: Reevaluate this, maybe we should deduplicate at creation
	//       and run with the assumption that models do not have duplicated vertices
	//       ALSO, maybe consider a threshold for equality because floats
	for (const auto& vert : a.verts) {
		if (!idx_map.contains(vert)) {
			m.verts.push_back(vert);
			idx_map[vert] = idx++;
		}
	}

	// Then do the same for model b
	// NOTE: Deduplication is definitely required here!
	for (const auto& vert : b.verts) {
		if (!idx_map.contains(vert)) {
			m.verts.push_back(vert);
			idx_map[vert] = idx++;
		}
	}

	// Next deal with the triangles!
	for (const auto& tri : a.tris) {
		IntermediateMesh::Triangle t = {};
		for (int i = 0; i < 3; i++)
			t.vert_idx[i] = idx_map[a.verts[tri.vert_idx[i]]];
		m.tris.push_back(t);
	}

	for (const auto& tri : b.tris) {
		IntermediateMesh::Triangle t = {};
		for (int i = 0; i < 3; i++)
			t.vert_idx[i] = idx_map[b.verts[tri.vert_idx[i]]];
		m.tris.push_back(t);
	}

	return m;
}

IntermediateMesh _construct_quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
	IntermediateMesh m;

	glm::vec3 diff_1 = b - a;
	glm::vec3 diff_2 = d - a;

	glm::vec3 norm = glm::normalize(glm::cross(diff_1, diff_2));

	m.verts = { {a, norm}, {b, norm}, {c, norm}, {d, norm} };
	m.tris = { { 0, 1, 2 }, {0, 2, 3} };

	return m;
}



IntermediateMesh _construct_cube(float size) {
	IntermediateMesh m;

	std::vector<uint16_t> indices;

	std::vector<glm::vec3> verts = {
		{-size, -size, -size}, {-size,  size, -size},
		{-size,  size,  size}, {-size, -size,  size},
		{ size, -size, -size}, { size,  size, -size},
		{ size,  size,  size}, { size, -size,  size}
	};

	m = merge(m, _construct_quad(verts[0], verts[3], verts[2], verts[1]));
	m = merge(m, _construct_quad(verts[2], verts[6], verts[5], verts[1]));
	m = merge(m, _construct_quad(verts[7], verts[4], verts[5], verts[6]));
	m = merge(m, _construct_quad(verts[4], verts[0], verts[1], verts[5]));
	m = merge(m, _construct_quad(verts[2], verts[3], verts[7], verts[6]));
	m = merge(m, _construct_quad(verts[0], verts[4], verts[7], verts[3]));

	return m;
}


void cubesphere_subdiv(IntermediateMesh& m) {
	// Subdivide triangles in a model as required for cube sphere
	//      
	//                       |Å_
	//   |Å_                 |  Å_ 
	//   |  Å_               |____Å_
	//   |    Å_         Å®   |Å_    |Å_ 
	//   |      Å_           |  Å_  |  Å_
	//   |________Å_         |____Å_|____Å_
	//

	std::vector<IntermediateMesh::Triangle> new_tris;

	for (auto& tri : m.tris) {
		uint16_t v1i = tri.vert_idx[0];
		uint16_t v2i = tri.vert_idx[1];
		uint16_t v3i = tri.vert_idx[2];

		auto v1 = m.verts[v1i];
		auto v2 = m.verts[v2i];
		auto v3 = m.verts[v3i];

		uint16_t m12 = m.verts.size();
		m.verts.push_back(IntermediateMesh::Vertex{ (v1.position + v2.position) / 2.f, (v1.normal + v2.normal) / 2.f });

		uint16_t m23 = m.verts.size();
		m.verts.push_back({ (v2.position + v3.position) / 2.f, (v2.normal + v3.normal) / 2.f });

		uint16_t m13 = m.verts.size();
		m.verts.push_back({ (v1.position + v3.position) / 2.f, (v1.normal + v3.normal) / 2.f });


		new_tris.push_back(IntermediateMesh::Triangle{ v1i, m12, m13 });
		new_tris.push_back(IntermediateMesh::Triangle{ m12, v2i, m23 });
		new_tris.push_back(IntermediateMesh::Triangle{ m12, m23, m13 });
		new_tris.push_back(IntermediateMesh::Triangle{ m13, m23, v3i });
	}

	m.tris = new_tris;
}

IntermediateMesh _construct_cube_sphere(float size, int subdivisions) {
	// First, generate a coob;
	IntermediateMesh m = _construct_cube(1.f);

	// Then subdivide n times
	for (int i = 0; i < subdivisions; i++) {
		cubesphere_subdiv(m);
	}

	printf("Pre-dedup Verts: %llu\n", m.verts.size());
	m = merge(m, IntermediateMesh()); // de-dupe
	printf("Post-dedup Verts: %llu\n", m.verts.size());


	// Then blow out the cube and fix the normals
	for (auto& vert : m.verts) {
		vert.normal = glm::normalize(vert.position);
		vert.position = vert.normal * size;
	}

	return  m;
}




Ref<Mesh> construct_quad_mesh(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
	Ref<Mesh> m = make_ref<Mesh>();
	m->load_from_intermediate_mesh(_construct_quad(a, b, c, d));
	return m;
}


Ref<Mesh> construct_cube_mesh(float size) {
	Ref<Mesh> m = make_ref<Mesh>();
	m->load_from_intermediate_mesh(_construct_cube(size));
	return m;
}


Ref<Mesh> construct_cube_sphere(float size, int subdivisions) {
	Ref<Mesh> m = make_ref<Mesh>();
	m->load_from_intermediate_mesh(_construct_cube_sphere(size, subdivisions));
	return m;
}


void Mesh::load_from_obj_string(const char* src) {
	// Temporary index vector, tuple goes Vertex, UV, Normal
	std::vector<std::tuple<int, int, int>> _indices;

	// Iterate over data, parsing as we go
	const char* it = src;
	while (*it) {
		if (*it == 'v') {
			it++;
			if (*it == ' ') {
				// Vertex
				// Format is:
				// v {x} {y} {z} {w}
				// w is optional ( we throw it out! )

				float x, y, z, w;

				skip_whitespace(it);
				x = consume_float(it);
				skip_whitespace(it);
				y = consume_float(it);
				skip_whitespace(it);
				z = consume_float(it);
				skip_whitespace_not_nl(it);
				if (*it != '\n')
					w = consume_float(it);
				else
					w = 1.0f;

				vertices.push_back({ x, y, z });
			}
			else if (*it == 'n') {
				// Normal
				// Format:
				// vn {x} {y} {z}
				it++;
				float x, y, z;

				skip_whitespace(it);
				x = consume_float(it);
				skip_whitespace(it);
				y = consume_float(it);
				skip_whitespace(it);
				z = consume_float(it);

				normals.push_back({ x, y, z });
			}
			else if (*it == 't') {
				// Texture UV
				// Format
				// vt {u} {v}
				// TODO: support the proper format vt {u} [{v} {w}]

				it++;
				float u, v;

				skip_whitespace(it);
				u = consume_float(it);
				skip_whitespace(it);
				v = consume_float(it);

				uvs.push_back({ u, v });
			}
		}
		else if (*it == 'f') {
			// Faces
			// Format:
			// f {v}[/{t}[/{n}]]
			// Where:
			//   v is the index of the vertex to render
			//   t is the index of the texture coordinate
			//   n is the index of the vertex normal
			it++;
			size_t v = 0, t = 0, n = 0;

			for (int i = 0; i < 3; i++) {
				skip_whitespace(it);
				v = consume_int(it);
				if (*it == '/') {
					it++;
					t = consume_int(it);
					if (*it == '/') {
						it++;
						n = consume_int(it);
					}
				}

				_indices.push_back({ v - 1, t - 1, n - 1 });
			}
		}
		else if (*it == '#') {} // do nothing for comments
		else {
			// Unsupported lines
			printf("Unsupported Line: ");
			while (*it != '\n') putchar(*it++);
			putchar('\n');
		}
		skip_to_newline(it);
		skip_whitespace(it);

	}


	// Now it's time to make this into a format that makes sense for modern OpenGL
	std::map<std::tuple<int, int, int>, uint32_t> conversion_map;
	std::vector<glm::vec3> _vertices;
	std::vector<glm::vec2> _uvs;
	std::vector<glm::vec3> _normals;
	uint32_t i = 0; // running tally

	for (auto& [v, vt, vn] : _indices) {
		uint32_t index = 0;
		if (conversion_map.contains({ v, vt, vn })) {
			index = conversion_map[{v, vt, vn}];
		}
		else {
			index = i++;
			conversion_map[{v, vt, vn}] = index;
			if (v != -1)	//should never be -1
				_vertices.push_back(vertices[v]);
			else assert(false);

			if (vt != -1)
				_uvs.push_back(uvs[vt]);

			if (vn != -1)
				_normals.push_back(normals[vn]);

		}

		indices.push_back(index);

		if (index == UINT32_MAX) fprintf(stderr, "Overrun indexes!\n");
	}

	vertices = _vertices;
	normals = _normals;
	uvs = _uvs;

	bool generate_tangents = false;
	bool generate_bitangents = false;

	has_normals = normals.size() != 0;
	has_uvs = false; //TODO:  uvs.size() != 0;

	

	// TODO: Generate tangents and bitangents,
	//		 and set the has_tangents and has_bitangents to true


	upload_gl_data();
}

void Mesh::load_from_file(const char* filename) {
	std::vector<uint8_t> src = load_file(filename);
	load_from_obj_string((const char*)src.data());
}

void Mesh::load_from_intermediate_mesh(const IntermediateMesh& im) {
	indices.resize(im.tris.size() * 3);
	memcpy(indices.data(), im.tris.data(), sizeof(im.tris[0]) *im.tris.size());

	for (int i = 0; i < im.verts.size(); i++) {
		vertices.push_back(im.verts[i].position);
		normals.push_back(im.verts[i].normal);
	}

	has_normals = true;

	upload_gl_data();
}

void Mesh::upload_gl_data() {
	VertexBufferLayout vbl = {};

	vbl.append({ "position", ShaderDataType::Vec3 });
	if (has_normals)		vbl.append({ "normal",		ShaderDataType::Vec3 });
	if (has_uvs)			vbl.append({ "uvs",			ShaderDataType::Vec2 });
	if (has_tangents)		vbl.append({ "tangent",		ShaderDataType::Vec3 });
	if (has_bitangents)		vbl.append({ "bitangent",	ShaderDataType::Vec3 });

	m_vbo->set_layout(vbl);

	int32_t stride = m_vbo->get_stride();

	// Create a buffer to create our GL compatible data in
	// Later on if we do our own binary model format, it will probably
	// primarily consist of this binary data with some kind of fast
	// and light compression

	// TODO: Think about a safer and more extensible format for this.
	std::vector<uint8_t> gl_data;
	gl_data.resize(stride * vertices.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		size_t offset = 0;

		memcpy(&gl_data[i * stride + offset], &vertices[i], sizeof(glm::vec3));
		offset += sizeof(glm::vec3);

		if (has_normals) {
			memcpy(&gl_data[i * stride + offset], &normals[i], sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
		}

		if (has_uvs) {
			memcpy(&gl_data[i * stride + offset], &uvs[i], sizeof(glm::vec2));
			offset += sizeof(glm::vec2);
		}

		if (has_tangents) {
			memcpy(&gl_data[i * stride + offset], &tans[i], sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
		}

		if (has_bitangents) {
			memcpy(&gl_data[i * stride + offset], &bitans[i], sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
		}
	}

	m_vbo->set_data(gl_data);
}


void Mesh::reload() {

}


void Mesh::unload() {

}

void Mesh::bind_vbo() {
	m_vbo->bind();
}
