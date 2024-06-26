#pragma once

#include <glm.hpp>

struct Light {
	// Let's think about using SSO so we don't need all this padding!
	struct Light_STD140 {
		glm::vec3 position;
		uint8_t _p_0[4];
		glm::vec3 color;
		float intensity;
	};
	
	Light_STD140 STD140(glm::vec3 position) const {
		return Light_STD140{
			.position = position,
			.color = color,
			.intensity = intensity
		};
	}

	glm::vec3 color;
	float intensity;
};
