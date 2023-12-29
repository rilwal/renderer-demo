#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

struct Mesh {
		uint num_vertices;
		uint first_idx;
		int base_vertex;
		float padding_1;
		vec3 aabb_min;
		float padding_2;
		vec3 aabb_max;
		float padding_3;
};

struct RenderCommand {
    uint count;
    uint instance_count;
    uint first_index;
    int base_vertex;
    uint base_instance;
};

layout(std430) restrict readonly buffer Meshes {
	Mesh meshes[];
};

layout(std430) restrict writeonly buffer RenderCommands {
    RenderCommand commands[];
};


struct ModelInstanceData {
    uint count;
    uint first_instance;
};


layout(std430) restrict buffer RenderData {
    uint render_offset;
    ModelInstanceData instance_data[];
};

layout(location=0) uniform uint num_models;

void main() {    
	uint global_id = gl_GlobalInvocationID.x;
    uint local_id = gl_LocalInvocationID.x;

    if (global_id < num_models) {
        Mesh m = meshes[global_id];

        uint instance_count = instance_data[global_id].count;

        // InstanceID start_idx -> start_idx + instance_count
        uint start_idx = atomicAdd(render_offset, instance_count);

        instance_data[global_id].first_instance = start_idx;

        RenderCommand rc = RenderCommand(
            m.num_vertices,
            instance_count,
            m.first_idx,
            m.base_vertex,
            start_idx
        );

        commands[global_id] = rc;
        instance_data[global_id].count = 0;
    }
}
