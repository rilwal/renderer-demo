

#include <random>

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "flecs.h"

#include "asset_manager.hpp"

#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "renderer/mesh.hpp"
#include "renderer/shader.hpp"

#include "renderer/vertex_buffer.hpp"
#include "renderer/index_buffer.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct Position {
    glm::vec3 position;

    void operator=(const glm::vec3& other) {
        position = other;
    }

    glm::mat4 mat4() {
        return glm::translate(glm::mat4(1), position);
    }

    operator glm::vec3&() {
        return position;
    }
};

struct Scale {
    glm::vec3 scale;

    glm::mat4 mat4() {
        return glm::scale(glm::mat4(1), scale);
    }

    operator glm::vec3& () {
        return scale;
    }
};

struct Rotation {
    glm::vec3 rotation;

    operator glm::vec3& () {
        return rotation;
    }

    glm::quat quat() {
        return glm::quat(rotation);
    }

    glm::mat4 mat4() {
        return glm::mat4_cast(quat());
    }

    operator glm::quat() {
        return quat();
    }
};


struct MeshComponent {
    Ref<Mesh> mesh;
};


struct Material {
    glm::vec3 diffuse_color;
};


glm::vec3 random_vec3(float min, float max) {
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_real_distribution dist(min, max);

    return { (float)dist(re), (float)dist(re), (float)dist(re) };
}


struct RenderCommand {
    uint32_t count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t base_vertex;
    uint32_t base_instance;
};



int main() {
    Renderer renderer;

    renderer.initialize();

    auto ecs = flecs::world();
    bool running = true;

    auto cube_1 = construct_cube_mesh(1.0);
    cube_1->upload_gl_data();

    for (int i = 0; i < 250; i++) {
        auto cube_entity = ecs.entity();

        cube_entity.set<Position>({ random_vec3(-8, 8)});
        cube_entity.set<Rotation>({ random_vec3(0, 3.13)});
        cube_entity.set<Scale>({ random_vec3(0.1, 2)});
        cube_entity.set<MeshComponent>({ cube_1 });
    }

    auto cartoon_shader = asset_manager.GetByPath<Shader>("assets/shaders/basic.glsl");


    Camera c = {};
    c.position = { 10, 10, 10 };
    c.rotation = { -3.9, -0.6 };
    c.projection = glm::perspective(45.0, 16.0 / 9.0, 0.01, 100.0);

    
    IndexBuffer index_buffer(BufferUsage::STATIC, ShaderDataType::U32);
    index_buffer.set_data(cube_1->indices);

    uint32_t command_buffer;
    glCreateBuffers(1, &command_buffer);


    VertexBuffer matrices(cube_1->vao_id());
    matrices.set_layout({ {"MVP", ShaderDataType::Mat4}, {"Model", ShaderDataType::Mat4} }, 1, 2);


    glBindVertexArray(cube_1->vao_id());

    while (!glfwWindowShouldClose(renderer.get_platform_window())) {

        glm::mat4 vp = c.projection * c.view();

        renderer.begin_frame();

        cartoon_shader->show_imgui();

        cube_1->bind_vbo();

        static auto position = glm::vec3(0);
        static auto scale = glm::vec3(2);
        static auto rotation = glm::vec3(0);

        auto quat_rotation = glm::quat(rotation);

        ImGui::Begin("Model Controls");

        ImGui::BeginGroup();
        ImGui::LabelText("Time", "%f", time);

        ImGui::DragFloat3("Camera Position", (float*)&c.position);
        ImGui::DragFloat2("Camera Rotation", (float*)&c.rotation, 3.14f / 360.f);

        ImGui::EndGroup();

        ImGui::End();

        std::vector<RenderCommand> command_list;
        std::vector<glm::mat4> matrix_data; // interleave mvp, model etc..

        uint32_t i = 0;
        ecs.each([&](flecs::entity e, Position& p, Rotation& r, Scale& s, MeshComponent& m) {
            const glm::mat4 model = p.mat4() * r.mat4()  * s.mat4();

            const auto mvp = vp * model;
            
            command_list.push_back({
                (uint32_t)cube_1->indices.size(),
                1,
                0,
                0,
                i++
            });

            matrix_data.push_back(mvp);
            matrix_data.push_back(model);
        });

        cartoon_shader->use();

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, command_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(RenderCommand) * command_list.size(), command_list.data(), GL_STREAM_DRAW);

        matrices.set_data<glm::mat4>(matrix_data);
        VertexBuffer::bind_multiple(cube_1->vbo(), matrices);
            
        index_buffer.bind();

        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, command_list.size(), 0);
        GL_ERROR_CHECK();

        renderer.end_frame();
    }

    renderer.shutdown();
}
