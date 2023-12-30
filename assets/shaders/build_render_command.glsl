#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;


#include "gpu_driven_renderer_includes.glsl"

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
