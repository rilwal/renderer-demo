

struct Mesh {
		uint num_vertices;
		uint first_idx;
		int base_vertex;
		float bounding_sphere;
		// vec3 aabb_min;
		// float padding_2;
		// vec3 aabb_max;
		// float padding_3;
};

struct Entity {
    uint mesh_idx;
    uint material_idx;
    uint transform_idx;
    uint padding;
};

struct RenderCommand {
    uint count;
    uint instance_count;
    uint first_index;
    int base_vertex;
    uint base_instance;
};

struct PerInstanceData {
    uint transform_idx;
    uint material_idx;
};

struct ModelInstanceData {
    uint count;
    uint first_instance;
};


layout(std430) restrict readonly buffer Entities {
	Entity entities[];
};

layout(std430) restrict readonly buffer Meshes {
	Mesh meshes[];
};

layout(std430) restrict writeonly buffer RenderCommands {
    RenderCommand commands[];
};

layout(std430) restrict buffer RenderData {
    uint render_offset;
    ModelInstanceData instance_data[];
};

layout(std430) restrict writeonly buffer PerInstance {
    PerInstanceData per_instance_data[];
};


