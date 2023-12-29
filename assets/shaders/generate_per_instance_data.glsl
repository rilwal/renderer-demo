#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

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

struct ModelInstanceData {
    uint count;
    uint first_instance;
};

layout(std430) restrict readonly buffer RenderData {
    uint render_offset;
    ModelInstanceData instance_data[];
};



layout(location=0) uniform uint num_entities;


void main() {    
	uint global_id = gl_GlobalInvocationID.x;

    if(global_id < num_entities) {
        Entity e = entities[global_id];
        
        PerInstanceData pid = PerInstanceData(
            e.transform_idx,
            e.material_idx
        );

        uint model_offset = atomicAdd(instance_data[e.mesh_idx].count, 1);
        per_instance_data[instance_data[e.mesh_idx].first_instance + model_offset] = pid;
    }
}
