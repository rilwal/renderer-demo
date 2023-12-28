#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

const uint MAX_MODELS = 100;

struct Entity {
    uint mesh_idx;
    uint material_idx;
    uint transform_idx;
    uint padding;
};

struct PerInstanceData {
    uint transform_idx;
    uint material_idx;
};

layout(std430) restrict readonly buffer Entities {
	Entity entities[];
};

layout(std430) restrict writeonly buffer PerInstance {
    PerInstanceData per_instance_data[];
};

layout(std430) coherent buffer RenderData {
    uint render_offset;
    uint num_instances[MAX_MODELS];
    uint model_data_start[MAX_MODELS];
};


void main() {    
	uint global_id = gl_GlobalInvocationID.x;

    if(global_id < entities.length()) {
        Entity e = entities[global_id];
        
        PerInstanceData pid = PerInstanceData(
            e.transform_idx,
            e.material_idx
        );

        uint model_offset = atomicAdd(num_instances[e.mesh_idx], 1);
        per_instance_data[model_data_start[e.mesh_idx] + model_offset] = pid;
    }
}
