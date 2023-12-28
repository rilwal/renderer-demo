#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

const uint MAX_MODELS = 100;

struct Mesh {
    uint num_vertices;
	uint first_idx;
	int base_vertex;
    uint padding;
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

layout(std430) restrict buffer RenderData {
    uint render_offset;
    uint num_instances[MAX_MODELS];
    uint model_data_start[MAX_MODELS];
};


void main() {    
	uint global_id = gl_GlobalInvocationID.x;
    uint local_id = gl_LocalInvocationID.x;

    if (global_id < meshes.length()) {
        Mesh m = meshes[global_id];

        uint instance_count = num_instances[global_id];

        // InstanceID start_idx -> start_idx + instance_count
        uint start_idx = atomicAdd(render_offset, instance_count);

        model_data_start[global_id] = start_idx;

        RenderCommand rc = RenderCommand(
            m.num_vertices,
            instance_count,
            m.first_idx,
            m.base_vertex,
            start_idx
        );

        commands[global_id] = rc;
        num_instances[global_id] = 0;
    }
}
