#pragma once

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"
#include "flecs.h"

#include "util.hpp"

extern flecs::world ecs;


struct Position {
    glm::vec3 position;

    inline void operator=(const glm::vec3& other) {
        position = other;
    }

    inline glm::mat4 mat4() const {
        return glm::translate(glm::mat4(1), position);
    }

    inline operator const glm::vec3& () const {
        return position;
    }
};

struct Scale {
    glm::vec3 scale;

    inline const glm::mat4 mat4() const {
        return glm::scale(glm::mat4(1), scale);
    }

    inline operator const glm::vec3& () const {
        return scale;
    }
};

struct Rotation {
    glm::vec3 rotation;

    inline operator const glm::vec3& () const {
        return rotation;
    }

    inline glm::quat quat() const {
        return glm::quat(rotation);
    }

    inline glm::mat4 mat4() const {
        return glm::mat4_cast(quat());
    }

    inline operator glm::quat() const {
        return quat();
    }
};

struct MeshComponent {
    uint32_t h;
    
    inline operator uint32_t() const {
        return h;
    }
};