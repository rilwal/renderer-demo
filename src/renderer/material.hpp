#pragma once

#include <cstdint>

#include "glm.hpp"

#include "texture.hpp"

struct Material {
#pragma pack(push, 1)
	struct MaterialSTD140 {
		glm::vec3 diffuse_color;			// 12 bytes	
		float _padding;						// 4 bytes padding
		glm::vec3 emissive_color;			// 12 bytes
		float _padding2;					// 4 bytes padding
		glm::vec2 metallic_roughness;		// 8 bytes
		uint64_t diffuse_texture;			// 8 bytes
		uint64_t normal_texture;			// 8 bytes
		uint64_t metalic_roughness_texture;	// 8 bytes
		uint64_t emissive_texture;			// 8 bytes
		float _padding3[2];					// 8 bytes
	};
#pragma pack(pop)


	MaterialSTD140 std140() const {
		return { 
			.diffuse_color=diffuse_color, 
			.emissive_color=emissive_color,
			.metallic_roughness=metallic_roughness, 
			.diffuse_texture=diffuse_texture, 
			.normal_texture=normal_map, 
			.metalic_roughness_texture=metalic_roughness_texture 
		};
	}

	glm::vec3 diffuse_color;
	glm::vec3 emissive_color;
	glm::vec2 metallic_roughness;

	uint64_t diffuse_texture; //It's a GPU Resident handle!
	uint64_t normal_map;
	uint64_t metalic_roughness_texture;

	bool blend = false;

};
