#type vertex

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in uint transform_idx;
layout(location = 5) in uint material_idx;

layout(std430, binding=7) restrict readonly buffer Transforms {
	mat4 transforms[];
};

uniform mat4 vp;

void main() {
	mat4 model = transforms[transform_idx];
	mat4 mvp = vp * model;
	vec4 vertex_pos = vec4(vertex_position, 1);

	gl_Position = mvp * vertex_pos;
}
