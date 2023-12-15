#type vertex

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in mat4 model;


uniform mat4 vp;

void main() {
	mat4 mvp = vp * model;
	vec4 vertex_pos = vec4(vertex_position, 1);

	gl_Position = mvp * vertex_pos;
}


#type fragment
out vec4 fragColour; 

void main() {

	fragColour = vec4(1,1,1,1);
	
}

