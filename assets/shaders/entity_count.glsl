#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

const uint MAX_MODELS = 100;

struct Entity {
    uint mesh_idx;
    uint material_idx;
    uint transform_idx;
    uint padding;
};

layout(std430) restrict readonly buffer Entities {
	Entity entities[];
};

layout(std430) restrict writeonly buffer RenderData {
    uint render_offset;
    uint num_instances[MAX_MODELS];
    uint model_data_start[MAX_MODELS];
};


void main() {    
	uint global_id = gl_GlobalInvocationID.x;
    uint local_id = gl_LocalInvocationID.x;

    if(global_id < entities.length()) {
        Entity e = entities[global_id];
        atomicAdd(num_instances[e.mesh_idx], 1);
    }
}
