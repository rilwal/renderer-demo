
#include "mesh.hpp"

#include <filesystem>
#include <unordered_map>

#include "fastgltf/parser.hpp"
#include "fastgltf/types.hpp"


#include "util.hpp"


Mesh::Mesh()  noexcept : IAsset("Mesh")  {
}

Mesh::~Mesh() noexcept {
}

Mesh::Mesh(Mesh&& other) noexcept : IAsset("Mesh") {
	has_normals = other.has_normals;
	has_tangents = other.has_tangents;
	has_bitangents = other.has_bitangents;
	has_uvs = other.has_uvs;
	
	vertices = std::move(other.vertices);
	normals = std::move(other.vertices);
	tans = std::move(other.tans);
	bitans = std::move(other.bitans);
	uvs = std::move(other.uvs);

	indices = std::move(other.indices);
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
		uint32_t v1i = tri.vert_idx[0];
		uint32_t v2i = tri.vert_idx[1];
		uint32_t v3i = tri.vert_idx[2];

		const auto v1 = m.verts[v1i];
		const auto v2 = m.verts[v2i];
		const auto v3 = m.verts[v3i];

		uint32_t m12 = static_cast<uint32_t>(m.verts.size());
		m.verts.push_back(IntermediateMesh::Vertex{ (v1.position + v2.position) / 2.f, (v1.normal + v2.normal) / 2.f });

		uint32_t m23 = static_cast<uint32_t>(m.verts.size());
		m.verts.push_back({ (v2.position + v3.position) / 2.f, (v2.normal + v3.normal) / 2.f });

		uint32_t m13 = static_cast<uint32_t>(m.verts.size());
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
	std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> _indices;

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
			uint32_t v = 0, t = 0, n = 0;

			for (int i = 0; i < 3; i++) {
				skip_whitespace(it);
				v = static_cast<uint32_t>(consume_int(it));
				if (*it == '/') {
					it++;
					t = static_cast<uint32_t>(consume_int(it));
					if (*it == '/') {
						it++;
						n = static_cast<uint32_t>(consume_int(it));
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
}

void Mesh::load_from_file(const char* filename) {
	auto path = std::filesystem::path(filename);

	if (path.extension() == ".obj") {
		std::vector<uint8_t> src = load_file(filename);
		load_from_obj_string((const char*)src.data());
	}
	else if (path.extension() == ".gltf") {
		
	}
	else {
		std::cerr << "Can't load model from unknown format: " << path.extension() << "\n";
	}
}

void Mesh::load_from_intermediate_mesh(const IntermediateMesh& im) {
	indices.resize(im.tris.size() * 3);
	memcpy(indices.data(), im.tris.data(), sizeof(im.tris[0]) *im.tris.size());

	for (int i = 0; i < im.verts.size(); i++) {
		vertices.push_back(im.verts[i].position);
		normals.push_back(im.verts[i].normal);
	}

	has_normals = true;
}


void Mesh::reload() {

}


void Mesh::unload() {

}

