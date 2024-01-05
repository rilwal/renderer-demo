#pragma once

#include "glm.hpp"

struct Camera {
	glm::vec3 position;
	glm::vec2 rotation;
		

	float near_clip;
	float far_clip;

	float fov;

	glm::mat4 projection() const {
		return glm::perspective(glm::radians(fov), 16.f / 9.f, near_clip, far_clip);
	}

	glm::vec3 get_heading() const {
		glm::vec4 heading = { 1, 0, 0, 0 };
		glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1), rotation.x, glm::vec3(0.f, 1.f, 0.f)) * glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0.f, 0.f, 1.f));
		return rotation_matrix * heading;
	}

	glm::mat4 view() const {
		return glm::lookAt(position, position + glm::vec3(get_heading()), {0, 1, 0});
	}
};

