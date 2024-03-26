#type vertex

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : enable

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in uint transform_idx;
layout(location = 5) in uint material_idx;

layout(std430, binding=7) restrict readonly buffer Transforms {
	mat4 transforms[];
};

out vec3 vertex_position_worldspace;
out vec3 vertex_normal;
out vec2 vertex_uv;
out mat3 TBN;

uniform mat4 vp;
uniform float time;

out flat uint material_idx_out;

void main() {
	mat4 model = transforms[transform_idx];
	mat4 mvp = vp * model;
	vec4 vertex_pos = vec4(vertex_position, 1);

	vertex_position_worldspace = (model * vertex_pos).xyz;
	gl_Position = mvp * vertex_pos;

	vertex_normal = mat3(inverse(transpose(model))) * normal;

	material_idx_out = material_idx;
	vertex_uv = uv;

	// Calculate TBN matrix for normal maps!
	// TODO: Look into tangent space lighting or whatever?
	vec3 bi_tan = cross(normal, tangent);
	
	vec3 T = normalize(vec3(model * vec4(tangent, 0)));
	vec3 B = normalize(vec3(model * vec4(bi_tan, 0)));
	vec3 N = normalize(vec3(model * vec4(normal, 0)));

	TBN = mat3(T, B, N);
}


#type fragment
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : enable

out vec4 fragColour; 

in flat uint material_idx_out;
in vec3 vertex_position_worldspace;
in vec3 vertex_normal;

in vec2 vertex_uv;
in mat3 TBN;

struct Material {
	vec3 diffuse_color;
	vec3 emissive_color;
	vec2 metallic_roughness;
	uint64_t diffuse_texture;
	uint64_t normal_texture;
	uint64_t metallic_roughness_texture;
	uint64_t emissive_texture;
};

struct Light {
	vec3 position;
	vec3 color;
	float intensity;
};


uniform vec3 camera_pos;

#range(0, .9, 0.001)
uniform float ambient = 0.2;

const float PI = 3.141;

uniform bool use_normal_map = true;
uniform bool use_roughness_metallic_map = true;

uniform bool render_normal_map = false;
uniform bool render_metallic_roughness_map = false;
uniform bool render_normals = false;

layout(std430, binding=6) restrict readonly buffer Lights {
	Light lights[];
};

layout(std430, binding=4) restrict readonly buffer Materials {
	Material materials[];
};

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}



void main() {
	Material mat = materials[material_idx_out];

	if (render_normal_map) {
		fragColour = texture(sampler2D(mat.normal_texture), vertex_uv);
		return;
	}

	vec3 albedo = materials[material_idx_out].diffuse_color;

	if (mat.diffuse_texture != 0) {
		albedo *= texture(sampler2D(mat.diffuse_texture), vertex_uv).rgb;
	} 

	vec3 N = normalize(vertex_normal);

	if (use_normal_map) {
		if (mat.normal_texture != 0) {
			N = texture(sampler2D(mat.normal_texture), vertex_uv).rgb;
			N = N * 2.0 - 1.0;
			N = normalize(TBN * N);
		}
	}

	if (render_normals) {
		fragColour = vec4((N + vec3(1)) / 2, 1.0);
		return;
	}

	float metallic = mat.metallic_roughness.x;
	float roughness = mat.metallic_roughness.y;

	if (mat.metallic_roughness_texture != 0) {
		if (render_metallic_roughness_map) {
			fragColour = texture(sampler2D(mat.metallic_roughness_texture),vertex_uv); 
			return;
		}

		if (use_roughness_metallic_map) {
			metallic *= texture(sampler2D(mat.metallic_roughness_texture), vertex_uv).b;
			roughness *= texture(sampler2D(mat.metallic_roughness_texture), vertex_uv).g;
		} else {
			metallic= 0;
			roughness = 0;
		}
	}
	

	vec3 F0 = vec3(0.04); // approximation of F0 for dielectrics
	F0 = mix(F0, albedo, metallic);

	vec3 V = normalize(camera_pos - vertex_position_worldspace);

	vec3 lo = vec3(0);

	for (int i = 0; i < lights.length(); i++) {
		Light l = lights[i];
		float distance = length(l.position - vertex_position_worldspace);
		float attenuation = max(0, 1.0 / (distance * distance) - 0.000001);

		//if (attenuation == 0) continue;

		vec3 radiance = l.color * l.intensity  * attenuation;



		vec3 L = normalize(l.position - vertex_position_worldspace);
		vec3 H = normalize(L + V);




		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		
		kD *= 1.0 - metallic;	

		vec3 numerator = NDF * G * F;
		float demoninator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / demoninator;


		float n_dot_l = max(dot(N, L), 0.0);
		lo += (kD * albedo / PI + specular) * radiance * n_dot_l;
	}
	
	vec3 ambient = vec3(0.05) * albedo;
	vec3 color = ambient + lo + mat.emissive_color * 2;


	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	fragColour = vec4(color, 1);
}

