

#include <random>
#include <chrono>

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "flecs.h"

#include "asset_manager.hpp"
#include "ecs_componets.hpp"

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



float random_float(float min, float max) {
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_real_distribution dist(min, max);
    
    return float(dist(re));
}

glm::vec3 random_vec3(float min, float max) {

    return { random_float(min, max),random_float(min, max),random_float(min, max)};
}



flecs::entity spawn_cube(MeshBundle& mb, MeshBundle::Handle handle) {
    auto cube_entity = ecs.entity();

    cube_entity.set<Position>({ random_vec3(-8, 8) });
    cube_entity.set<Rotation>({ random_vec3(0, 3.14) });
    cube_entity.set<Scale>({ glm::vec3(random_float(.6, 1.4))});
    mb.add_mesh_to_entity(cube_entity, handle);
    //cube_entity.set<MeshComponent>({ handle });

    return cube_entity;
}



int main() {
    Renderer renderer;
    renderer.initialize();
    {
        ecs = flecs::world();
        bool running = true;

        auto cube_1 = construct_cube_mesh(1.0);
        auto sphere = construct_cube_sphere(1.0, 4);
        auto joker = asset_manager.GetByPath<Mesh>("assets/models/joker.obj");
        auto sponza = asset_manager.GetByPath<Mesh>("assets/models/sponza.gltf");

        GL_ERROR_CHECK();

        MeshBundle bundle(ecs);

        uint32_t cube_mesh = bundle.add_entry(cube_1);
        uint32_t sphere_mesh = bundle.add_entry(sphere);
        uint32_t joker_mesh = bundle.add_entry(joker);


        for (int i = 0; i < 100; i++) {
            spawn_cube(bundle, cube_mesh);
            spawn_cube(bundle, sphere_mesh);
            spawn_cube(bundle, joker_mesh);

        }

        auto my_joker = spawn_cube(bundle, joker_mesh);
        my_joker.set<Scale>({ glm::vec3(10) });



        Camera c = {};
        c.position = { 10, 10, 10 };
        c.rotation = { -3.9, -0.6 };
        c.projection = glm::perspective(45.0, 16.0 / 9.0, 0.01, 100.0);


        std::chrono::high_resolution_clock clock;

        float time = float(std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count()) * 0.001f;
        float last_time = time;

        std::vector<float> times;
        times.resize(16);

        uint32_t times_buffer_idx = 0;

        float total_time = 0;


        while (!glfwWindowShouldClose(renderer.get_platform_window())) {
            float delta_time = time - last_time;
            last_time = time;
            time = float(std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count()) * 0.001f;
            total_time -= times[times_buffer_idx % 16];
            total_time += delta_time;
            times[times_buffer_idx % 16] = delta_time;
            times_buffer_idx++;

            glm::mat4 vp = c.projection * c.view();

            renderer.begin_frame();

            ImGui::Begin("Camera Controls");

            ImGui::Text("%f ms (%.02f fps)", (total_time / 16) * 1000, 1.f / (total_time / 16));

            ImGui::Text("%llu", bundle.get_rendered_tri_count());

            ImGui::BeginGroup();

            ImGui::DragFloat3("Camera Position", (float*)&c.position);
            ImGui::DragFloat2("Camera Rotation", (float*)&c.rotation, 3.14f / 360.f);

            ImGui::EndGroup();

            if (ImGui::Button("Spawn cube")) {
                spawn_cube(bundle, cube_mesh);
            }

            if (ImGui::Button("Spawn sphere")) {
                spawn_cube(bundle, sphere_mesh);
            }


            if (ImGui::Button("Spawn Joker")) {
                for (int i = 0; i < 1000; i++)
                    spawn_cube(bundle, joker_mesh);
            }

            if (ImGui::Button("Move Joker")) {
                my_joker.set<Position>({ random_vec3(-20, 20) });
                my_joker.set<Rotation>({ random_vec3(0, 3.141) });

            }


            ImGui::End();


            bundle.render(ecs, vp);



            renderer.end_frame();
        }
    }
    renderer.shutdown();
}
