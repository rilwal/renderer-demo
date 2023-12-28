#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

const uint MAX_MODELS = 100;


layout(std430) restrict writeonly buffer RenderData {
    uint render_offset;
    uint num_instances[MAX_MODELS];
    uint model_data_start[MAX_MODELS];
};


void main() {    
	uint global_id = gl_GlobalInvocationID.x;

    if(global_id < MAX_MODELS) {
        num_instances[global_id] = 0;
        model_data_start[global_id] = 0;
    }
}
