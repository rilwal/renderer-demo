#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "gpu_driven_renderer_includes.glsl"

layout(location=0) uniform uint num_entities;
layout(location=1) uniform mat4 model_view;

void main() {   
	uint global_id = gl_GlobalInvocationID.x;
    uint local_id = gl_LocalInvocationID.x;

    if(global_id < num_entities) {
        Entity e = entities[global_id];

        mat4 t = transforms[e.transform_idx];
        Mesh m = meshes[e.mesh_idx];

        vec4 pos = t[3];

        //pos *= model_view;

        //if (pos.z < 0) return;

        atomicAdd(instance_data[e.mesh_idx].count, 1);
    }
}
