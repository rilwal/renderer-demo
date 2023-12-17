#pragma once

#include <cstdint>

#include "glm.hpp"

#include "texture.hpp"

struct Material {
	struct MaterialSTD140 {
		glm::vec3 diffuse_color; // 12 bytes
		uint8_t _padding[4]; // 4 bytes padding
		glm::vec2 metallic_roughness;
		uint8_t _padding_2[8];
		uint64_t diffuse_texture; // Stored as a vec2 in shader!
		uint64_t normal_texture;
		uint64_t metalic_roughness_texture;
		uint64_t _padding_3;
	};

	MaterialSTD140 std140() const {
		return { 
			.diffuse_color=diffuse_color, 
			.metallic_roughness=metallic_roughness, 
			.diffuse_texture=diffuse_texture, 
			.normal_texture=normal_map, 
			.metalic_roughness_texture=metalic_roughness_texture 
		};
	}

	glm::vec3 diffuse_color;
	glm::vec2 metallic_roughness;

	uint64_t diffuse_texture; //It's a GPU Resident handle!
	uint64_t normal_map;
	uint64_t metalic_roughness_texture;

	bool blend = false;

};
