#type vertex

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in mat4 mvp;
layout(location = 6) in mat4 model;


out vec4 vertex_position_worldspace;
out vec3 vertex_normal;


uniform float time;

#step(0.001)
#range(0, 1)
uniform vec3 scale = vec3(1);

void main() {
	vec4 vp = vec4(vertex_position * scale, 1);

	vertex_position_worldspace = model * vp;
	gl_Position = mvp * vp;

	vertex_normal = (inverse(transpose(model)) * vec4(normal, 1)).xyz;
}


#type fragment
out vec4 fragColour; 

uniform vec3 light_pos = { 0, 0, 0 };

#range(0, .9, 0.001)
uniform float ambient = 0.2;

#color
uniform vec3 color = vec3(.8, .4, 1);
uniform bool cartoon = false;
uniform bool tonemap = true;

#step(1)
uniform float light_intensity = 190.0;

in vec4 vertex_position_worldspace;
in vec3 vertex_normal;

#range(0, 1, 0.001)
uniform float cartoon_offset_1 = 0.1;

#range(0, 1, 0.001)
uniform float cartoon_offset_2 = 0.4;

#range(0, 1, 0.001)
uniform float cartoon_offset_3 = 0.8;



vec3 aces(vec3 color) {
	return (color * (2.51 * color + 0.3)) / (color * (2.43 * color + 0.59) + 0.14);
}


void main() {
	vec3 lightDir = normalize(light_pos - vertex_position_worldspace.xyz);
	float diff = max(0, dot(normalize(vertex_normal), lightDir));


	//float distance_2 = dot(light_pos - vertex_position_worldspace.xyz, light_pos - vertex_position_worldspace.xyz);
	//diff *= light_intensity;
	//diff /= distance_2;
	
	diff += ambient;

	if (cartoon) {
		//diff = ceil(diff * toon_color_levels) * toon_scale_factor;
		if (diff > cartoon_offset_3) diff = 1.0;
		else if (diff > cartoon_offset_2) diff = cartoon_offset_2;
		else if (diff > cartoon_offset_1) diff = cartoon_offset_1;
		else diff = 0;
	}

	vec3 diff_color = color * diff;
	//vec3 ambient_color = color * ambient;

	//vec3 total_color = diff_color + ambient_color;

	if (tonemap) {
		diff_color = aces(diff_color);
	}

	fragColour = vec4(diff_color, 1);
	
	//fragColour = vec4(normalize(vertex_normal), 1);
}

