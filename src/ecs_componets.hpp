#pragma once

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"
#include "flecs.h"

#include "util.hpp"
#include "renderer/types.hpp"

extern flecs::world ecs;


// We wanna just directly set our transformation matrcices!
struct TransformComponent {
    glm::mat4 transform;

    TransformComponent() : transform() {};

    TransformComponent(glm::mat4 other) {
        transform = other;
    }

    auto& operator[](size_t idx) {
        return transform[idx];
    }

    const auto& operator[](size_t idx) const {
        return transform[idx];
    }

    operator glm::mat4() const {
        return transform;
    }
};

struct Position {
    glm::vec3 position;

    Position() : position(0.0) {};

    Position (glm::vec3 other) {
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

    Scale() : scale(1.0) {};

    Scale(float n) {
        scale = glm::vec3(n);
    }

    Scale(glm::vec3 other) {
        scale = other;
    }

    inline const glm::mat4 mat4() const {
        return glm::scale(glm::mat4(1), scale);
    }

    inline operator const glm::vec3& () const {
        return scale;
    }
};

struct Rotation {
    glm::quat rotation;

    Rotation() : rotation(1, 0, 0, 0) {};


    Rotation(glm::quat other) {
        rotation = other;
    }

    inline operator glm::quat() const {
        return rotation;
    }

    inline glm::mat4 mat4() const {
        return glm::mat4_cast(rotation);
    }
};


// A model will be a set of mesh, material pairs
struct Model {
    std::vector<std::pair<MeshHandle, MaterialHandle>> meshes;

    Model() : meshes() {};

    Model(MeshHandle mesh_handle, MaterialHandle material_handle = default_material) {
        meshes.push_back({ mesh_handle, material_handle });
    }
};


// This is very experimental!!!
struct Velocity {
    glm::vec3 velocity;

    Velocity() : velocity(0.0) {};

    Velocity(glm::vec3 other) {
        velocity = other;
    }

    inline operator const glm::vec3& () const {
        return velocity;
    }
};