

#include <random>
#include <chrono>
#include <filesystem>
#include <variant>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"
#include "gtx/euler_angles.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "flecs.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include "stb_image.h"

#include "asset_manager.hpp"
#include "ecs_componets.hpp"

#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "renderer/mesh.hpp"
#include "renderer/shader.hpp"
#include "renderer/material.hpp"
#include "renderer/gltf.hpp"

#include "renderer/vertex_buffer.hpp"
#include "renderer/index_buffer.hpp"





void set_entity_transform (flecs::entity& e, Position translation=Position(), Rotation rotation= Rotation(), Scale scale= Scale()) {
    e.set<Position>(translation);
    e.set<Rotation>(rotation);
    e.set<Scale>(scale);

    flecs::entity parent = e.parent();
    glm::mat4 parent_transform = glm::mat4(1);

    if (parent) {
        if (parent.has<TransformComponent>()) {
            parent_transform = *parent.get<TransformComponent>();
        }
    }

    e.set<TransformComponent>(translation.mat4() * rotation.mat4() * scale.mat4());
}



float random_float(float min, float max) {
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_real_distribution dist(min, max);
    
    return float(dist(re));
}

glm::vec3 random_vec3(float min, float max) {

    return { random_float(min, max),random_float(min, max),random_float(min, max)};
}



flecs::entity spawn_cube(MeshBundle& mb, Model model) {
    auto cube_entity = ecs.entity();

    set_entity_transform(cube_entity, random_vec3(-10, 10), Rotation(glm::quat(random_vec3(-3.14f, 3.14f))), Scale(random_float(0.5f, 1.5f)));
    cube_entity.set<Model>(model);

    return cube_entity;
}



void update_tree_transforms(flecs::entity e, glm::mat4 parent_transform = glm::mat4(1)) {
    // Get entity position
    const Position* p = e.get<Position>();
    const Rotation* r = e.get<Rotation>();
    const Scale* s = e.get<Scale>();

    const Position position = p ? *p : Position(glm::vec3(0));
    const Rotation rotation = r ? *r : Rotation(glm::quat(1, 0, 0, 0));
    const Scale    scale = s ? *s : Scale(glm::vec3(1));

    glm::mat4 local_transform = position.mat4() * rotation.mat4() * scale.mat4();
    glm::mat4 global_transform = parent_transform * local_transform;

    e.set<TransformComponent>(global_transform);

    // Iterate children recursively
    e.children([&](flecs::entity child) {
        update_tree_transforms(child, global_transform);
        });
}

static flecs::entity selected_entity;


void entity_inspector_iterate(flecs::entity e) {
    if (ImGui::TreeNodeEx((void*)e.id(), ImGuiTreeNodeFlags_DefaultOpen | (e == selected_entity ? ImGuiTreeNodeFlags_Selected : 0), e.name().c_str())) {
        
        if (ImGui::IsItemClicked()) {
            selected_entity = e;
        }




        auto name_order = [](flecs::entity_t a, const void* name_a, flecs::entity_t b, const void* name_b) {
            if (name_a == 0) return 0;
            if (name_b == 0) return 0;
            return strcmp((const char*)name_a, (const char*)name_b);
        };


        auto filter = ecs.filter_builder()
            .term({ flecs::ChildOf, e.id() })
            //.term({ flecs::Name })
            //.order_by(flecs::Name, name_order )
            .build();

        filter.each([&](flecs::entity child) {
            entity_inspector_iterate(child);

            });


        

        e.children([&](flecs::entity child) {
            });
        ImGui::TreePop();
    }

}



void EditTransform(const Camera& camera, glm::mat4& matrix)
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    if (ImGui::IsKeyPressed(ImGuiKey_Z))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_X))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_C)) // r Key
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(&matrix[0][0], matrixTranslation, matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation);
    ImGui::InputFloat3("Rt", matrixRotation);
    ImGui::InputFloat3("Sc", matrixScale);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, &matrix[0][0]);

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
            mCurrentGizmoMode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
            mCurrentGizmoMode = ImGuizmo::WORLD;
    }
    

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    glm::mat4 view = camera.view();
    ImGuizmo::Manipulate(&view[0][0], &camera.projection[0][0], mCurrentGizmoOperation, mCurrentGizmoMode, &matrix[0][0], nullptr, nullptr);
}



void draw_entity_inspector(MeshBundle& bundle, flecs::entity root, Camera& camera) {

    if (ImGui::Begin("Entity Tree")) {
        entity_inspector_iterate(root);
    }

    ImGui::End();


    if (ImGui::Begin("Entity Inspector")) {
        if (selected_entity.is_alive()) {
            EditTransform(camera, selected_entity.get_mut<TransformComponent>()->transform);

            if (ImGui::DragFloat3("Position", (float*)selected_entity.get_mut<Position>(), 0.1f)) {
                selected_entity.modified<Position>();
            }

            if (selected_entity.has<Rotation>()) {
                glm::quat rotation = *selected_entity.get<Rotation>();
                if (ImGui::DragFloat4("Rotation", (float*)&rotation, 0.05f, -3.14f, 3.14f)) {
                    selected_entity.set<Rotation>(rotation);
                    selected_entity.modified<Rotation>();
                }
            }

            if (ImGui::DragFloat3("Scale", (float*)selected_entity.get_mut<Scale>(), 0.1f)) {
                selected_entity.modified<Scale>();
            }

            if (selected_entity.has<Model>()) {
                auto model = *selected_entity.get<Model>();

                size_t i = 0;
                for (auto& [mesh, material] : model.meshes) {
                    i++;
                    auto entry = bundle.get_entry(model.meshes[0].first);

                    ImGui::Text("Mesh %d", i);
                    ImGui::Text("Number of indices: %u", entry.num_vertices);
                    ImGui::Text("Base vertex: %d", entry.base_vertex);
                    ImGui::Text("Base idx: %u", entry.first_idx);
                }
            }

            if (selected_entity.has<Light>()) {
                auto& light = *selected_entity.get_mut<Light>();

                ImGui::ColorPicker3("Color", &light.color[0]);
                ImGui::DragFloat("Intesnsity", &light.intensity);
            }
        }
    }
    ImGui::End();

}







int main() {


    ecs = flecs::world();
    flecs::entity root_node = ecs.entity("Root")
        .add<Position>()
        .add<Rotation>()
        .add<Scale>();



    Renderer renderer;
    renderer.initialize();
    {    
        ecs.observer<const Position, const Rotation, const Scale>().event(flecs::OnSet | flecs::OnAdd).each(
        [](flecs::entity e, const Position&, const Rotation&, const Scale&) {
            // If any of these components are changed, let's update the translation matrices for all the children!!
            auto parent = e.parent();
            glm::mat4 parent_transform = glm::mat4(1);

            if (parent && parent.has<TransformComponent>()) {
                parent_transform = *parent.get<TransformComponent>();
            }

            update_tree_transforms(e, parent_transform);
        });


        bool running = true;

        int max_fragment_blocks, max_uniform_block_size;

        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &max_fragment_blocks);
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_uniform_block_size);

        printf("Max Fragment blocks: %d\nMax Block Size: %d\n\n", max_fragment_blocks, max_uniform_block_size);

        auto cube_1 = construct_cube_mesh(1.0);
        auto sphere = construct_cube_sphere(1.0, 4);
        auto joker = asset_manager.GetByPath<Mesh>("assets/models/joker.obj");

        GL_ERROR_CHECK();

        MeshBundle bundle;


        //load_gltf("assets/models/sponza/NewSponza_Main_glTF_002.gltf", root_node, bundle);

        MeshHandle cube_mesh = bundle.add_entry(cube_1);
        MeshHandle sphere_mesh = bundle.add_entry(sphere);
        MeshHandle joker_mesh = bundle.add_entry(joker);

        //flecs::entity cube = spawn_cube(bundle, cube_mesh);
        flecs::entity cube = ecs.entity()
            .child_of(root_node)
            .set<Position>(glm::vec3{ 0, 0, 0 })
            .set<Scale>(glm::vec3(1, 1, 1))
            .set<Rotation>(glm::quat(1, 0, 0, 0));




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

        constexpr bool test_scene = true;

        if (test_scene) {
            // Spawn a classic sphere grid!
            auto balls = ecs.entity("Balls")
                .add<Rotation>()
                .add<Scale>()
                .set<Position>(glm::vec3{ 20, 0, 0 })
                .child_of(root_node);

            constexpr int grid_size = 6;
            for (int x = 0; x < grid_size; x++) {
                for (int y = 0; y < grid_size; y++) {
                    Material m = {};
                    m.diffuse_color = { 1, .05, .05 };
                    m.metallic_roughness = { (float)y / grid_size, (float)x / grid_size };
                    MaterialHandle material = bundle.register_material(m);
                    Model model(sphere_mesh, material);

                    ecs.entity(std::format("Ball {}, {}", x, y).c_str())
                        .child_of(balls)
                        .add<Rotation>()
                        .set<Scale>(0.8f)
                        .set<Position>(glm::vec3{ x * 2, y * 2, 0 })
                        .set<Model>(model);
                }
            }



            auto cubes = ecs.entity("Cubes")
                .child_of(root_node);

            set_entity_transform(cubes);

            for (int i = 0; i < 100; i++) {
                MaterialHandle material = bundle.register_material({ random_vec3(0, 1), glm::vec2(random_float(.1f, .9f), random_float(.1f, .9f)) });
                Model m(cube_mesh, material);

                auto cube_entity = ecs.entity(std::format("Cube {}", i).c_str())
                    .child_of(cubes)
                    .set<Model>(m);

                set_entity_transform(cube_entity, random_vec3(-10, 10), Rotation(glm::quat(random_vec3(-3.14f, 3.14f))), Scale(random_float(0.5f, 1.5f)));
            }

            auto lights = ecs.entity("Lights")
                .add<Rotation>()
                .add<Scale>()
                .set<Position>({})
                .child_of(root_node);


            auto e = ecs.entity(std::format("Master Light").c_str())
                .child_of(lights)
                .add<Scale>()
                .add<Rotation>()
                .set<Position>(glm::vec3{ 0.f, 0.f, 0.f })
                .set<Light>({ glm::vec3(1, 1, 1) , 100 });


            for (int i = 0; i < 40; i++) {
                auto e = ecs.entity(std::format("Light {}", i).c_str())
                    .child_of(lights)
                    .add<Scale>()
                    .add<Rotation>()
                    .set<Position>(random_vec3(-10, 10))
                    .set<Light>({ random_vec3(0, 1) , random_float(10, 100) });
            }

        }




        while (!glfwWindowShouldClose(renderer.get_platform_window())) {
            float delta_time = time - last_time;
            last_time = time;
            time = float(std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count()) * 0.001f;
            total_time -= times[times_buffer_idx % 16];
            total_time += delta_time;
            times[times_buffer_idx % 16] = delta_time;
            times_buffer_idx++;


            for (auto& task : g_tasks) {
                task.callback(task.parameter);
            }


            static float move_speed = glfwGetKey(renderer.get_platform_window(), GLFW_KEY_LEFT_SHIFT) ? 10.0f : 5.0f;

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_W)) {
                c.position += c.get_heading() * delta_time * move_speed;
            }

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_S)) {
                c.position -= c.get_heading() * delta_time * move_speed;
            }

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_A)) {
                c.position -= glm::cross(c.get_heading(), glm::vec3(0, 1, 0)) * delta_time * move_speed;
            }

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_D)) {
                c.position += glm::cross(c.get_heading(), glm::vec3(0, 1, 0)) * delta_time * move_speed;
            }


            glm::dvec2 mouse_pos = {};
            glfwGetCursorPos(renderer.get_platform_window(), &mouse_pos.x, &mouse_pos.y);

            static glm::dvec2 last_mouse_pos = mouse_pos;
            glm::dvec2 mouse_delta = mouse_pos - last_mouse_pos;

            last_mouse_pos = mouse_pos;

            if (glfwGetMouseButton(renderer.get_platform_window(), GLFW_MOUSE_BUTTON_1)) {
                if (!ImGui::GetIO().WantCaptureMouse)
                    c.rotation -= mouse_delta / 250.0;
            }


            glm::mat4 vp = c.projection * c.view();

            renderer.begin_frame();

            draw_entity_inspector(bundle, root_node, c);


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


            ImGui::End();


            bundle.render(ecs, c);



            renderer.end_frame();
        }
    }
    renderer.shutdown();
}
