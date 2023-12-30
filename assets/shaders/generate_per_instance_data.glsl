#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "gpu_driven_renderer_includes.glsl"

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
