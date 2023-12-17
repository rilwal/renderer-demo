#pragma once

#include "glm.hpp"

struct Camera {
	glm::vec3 position;
	glm::vec2 rotation;
		
	glm::mat4 projection;


	glm::vec3 get_heading() const {
		glm::vec4 heading = { 1, 0, 0, 0 };
		glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1), rotation.x, glm::vec3(0.f, 1.f, 0.f)) * glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0.f, 0.f, 1.f));
		return rotation_matrix * heading;
	}

	glm::mat4 view() const {
		return glm::lookAt(position, position + glm::vec3(get_heading()), {0, 1, 0});
	}
};

