#type compute

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

const uint MAX_MODELS = 100;

struct Entity {
    uint mesh_idx;
    uint material_idx;
    uint transform_idx;
    uint padding;
};

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


struct PerInstanceData {
    uint transform_idx;
    uint material_idx;
};


layout(std430) restrict readonly buffer Entities {
	Entity entities[];
};

layout(std430) restrict readonly buffer Meshes {
	Mesh meshes[];
};

layout(std430) restrict writeonly buffer PerInstance {
    PerInstanceData per_instance_data[];
};

layout(std430) restrict writeonly buffer RenderCommands {
    RenderCommand commands[];
};

buffer RenderData {
    uint render_offset;
    uint num_instances[MAX_MODELS];
    uint model_data_start[MAX_MODELS];
};


void main() {    
	uint global_id = gl_GlobalInvocationID.x;
    
    if(global_id < entities.length()) {
        Entity e = entities[global_id];
        Mesh m = meshes[e.mesh_idx];

        PerInstanceData pid = PerInstanceData(
            e.transform_idx,
            e.material_idx
        );

        RenderCommand rc = RenderCommand(
            m.num_vertices,
            1,
            m.first_idx,
            m.base_vertex,
            global_id
        );

        commands[global_id] = rc;
        per_instance_data[global_id] = pid;
    }
}
