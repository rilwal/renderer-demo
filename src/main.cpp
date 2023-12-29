

#include <random>
#include <chrono>
#include <filesystem>
#include <variant>
#include <numbers>

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

#include "assets/asset_manager.hpp"
#include "ecs_componets.hpp"

#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "renderer/mesh.hpp"
#include "renderer/shader.hpp"
#include "renderer/material.hpp"
#include "renderer/gltf.hpp"

#include "renderer/vertex_buffer.hpp"
#include "renderer/index_buffer.hpp"



void set_entity_transform(flecs::entity& e, Position translation = Position(), Rotation rotation = Rotation(), Scale scale = Scale()) {

    flecs::entity parent = e.parent();
    glm::mat4 parent_transform = glm::mat4(1);

    if (parent) {
        if (parent.has<TransformComponent, World>()) {
            parent_transform = *parent.get<TransformComponent, World>();
        }
    }

    glm::mat4 local_transform = translation.mat4() * rotation.mat4() * scale.mat4();
    glm::mat4 world_transform = parent_transform * local_transform;

    e.set<TransformComponent, Local>({ local_transform });
    e.set<TransformComponent, World>({ world_transform });

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
    Model m2 = model;
    auto cube_entity = ecs.entity();
    m2.mesh.second = mb.register_material({ random_vec3(0, 1), glm::vec2(random_float(.1f, .9f), random_float(.1f, .9f)) });

    set_entity_transform(cube_entity, random_vec3(-10, 10), Rotation(glm::quat(random_vec3(-3.14f, 3.14f))), Scale(random_float(0.5f, 1.5f)));
    cube_entity.set<Model>(m2);

    return cube_entity;
}



void update_tree_transforms(flecs::entity e, glm::mat4 parent_transform = glm::mat4(1)) {
    // Get entity position

    glm::mat4 local_transform = e.has<TransformComponent, Local>() ? glm::mat4(*e.get<TransformComponent, Local>()) : glm::mat4(1);
    glm::mat4 world_transform = parent_transform * local_transform;

    e.set<TransformComponent, World>({ world_transform });

    // Iterate children recursively
    e.children([&](flecs::entity child) {
        update_tree_transforms(child, world_transform);
        });
}

static flecs::entity selected_entity;


void entity_inspector_iterate(flecs::entity e) {
    if (ImGui::TreeNodeEx((void*)e.id(), (e == selected_entity ? ImGuiTreeNodeFlags_Selected : 0), e.name().c_str())) {
        
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

        e.children([&](flecs::entity child) {
            entity_inspector_iterate(child);

            });

        ImGui::TreePop();
    }

}



bool EditTransform(const Camera& camera, glm::mat4& matrix)
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    
    if (ImGui::IsKeyPressed(ImGuiKey_Z))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_X))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_C))
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
    ImGui::DragFloat3("Tr", matrixTranslation);
    ImGui::DragFloat3("Rt", matrixRotation);
    ImGui::DragFloat3("Sc", matrixScale);
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
    return ImGuizmo::Manipulate(&view[0][0], &camera.projection[0][0], mCurrentGizmoOperation, mCurrentGizmoMode, &matrix[0][0], nullptr, nullptr);
}



void draw_entity_inspector(MeshBundle& bundle, flecs::entity root, Camera& camera) {

    if (ImGui::Begin("Entity Tree")) {
        entity_inspector_iterate(root);
    }

    ImGui::End();


    if (ImGui::Begin("Entity Inspector")) {
        if (selected_entity.is_alive()) {

            ImGui::TextWrapped("%s", ecs_type_str(ecs.get_world(), selected_entity.type()));

            static bool local = false;

            if (EditTransform(camera, selected_entity.get_mut<TransformComponent, World>()->transform)) {
                selected_entity.modified<TransformComponent, World>();
            }

            if (selected_entity.has<Model>()) {
                auto model = *selected_entity.get<Model>();

                auto& [mesh, material] = model.mesh;
                auto entry = bundle.get_entry(model.mesh.first);

                ImGui::Text("Number of indices: %u", entry.num_vertices);
                ImGui::Text("Base vertex: %d", entry.base_vertex);
                ImGui::Text("Base idx: %u", entry.first_idx);
            }

            if (selected_entity.has<Light>()) {
                auto& light = *selected_entity.get_mut<Light>();

                if (ImGui::ColorPicker3("Color", &light.color[0])) selected_entity.modified<Light>();
                if(ImGui::DragFloat("Intesnsity", &light.intensity)) selected_entity.modified<Light>();

            }
        }
    }
    ImGui::End();

}



int main() {
    ecs = flecs::world();


    ecs.observer<const LocalTransform>().event(flecs::OnSet | flecs::OnAdd).each(
        [](flecs::entity e, const TransformComponent& t) {
            // If any of these components are changed, let's update the translation matrices for all the children!!
            auto parent = e.parent();
            glm::mat4 parent_transform = glm::mat4(1);

            if (parent && parent.has<WorldTransform>()) {
                parent_transform = *parent.get<WorldTransform>();
            }

            e.set<TransformComponent, World>(TransformComponent(parent_transform * t.transform));
        }
    );

    ecs.observer <const WorldTransform>().event(flecs::OnSet | flecs::OnAdd).each(
        [](flecs::entity e, const TransformComponent& t) {
            e.children([&](flecs::entity child) {
                update_tree_transforms(child, t);
            });
        }
    );




    ecs.observer().term<const Position>().or_().term<const Rotation>().or_().term<const Scale>().event(flecs::OnSet | flecs::OnAdd).each(
        [](flecs::entity e) {
            const glm::mat4 p = (e.has<Position>() ? *e.get<Position>() : Position()).mat4();
            const glm::mat4 r = (e.has<Rotation>() ? *e.get<Rotation>() : Rotation()).mat4();
            const glm::mat4 s = (e.has<Scale>() ? *e.get<Scale>() : Scale()).mat4();

            glm::mat4 local_transform = p * r * s;
            e.set<TransformComponent, Local>({ local_transform });
        }
    );


    flecs::entity root_node = ecs.entity("Root")
        .add<Position>()
        .add<Rotation>()
        .add<Scale>();



    Renderer renderer;
    renderer.initialize();
    {    
        auto gpu_driven_shader = asset_manager.GetByPath<Shader>("assets/shaders/gpu_driven.glsl");

        auto entity_count_shader = asset_manager.GetByPath<Shader>("assets/shaders/entity_count.glsl");
        auto build_render_comand = asset_manager.GetByPath<Shader>("assets/shaders/build_render_command.glsl");
        auto generate_per_instance_data = asset_manager.GetByPath<Shader>("assets/shaders/generate_per_instance_data.glsl");


        bool running = true;


        auto cube_1 = construct_cube_mesh(1.0);
        auto sphere = construct_cube_sphere(1.0, 4);
        auto joker = asset_manager.GetByPath<Mesh>("assets/models/joker.obj");


        MeshBundle bundle;


        auto boombox = load_gltf("assets/models/boombox.gltf", root_node, bundle);
        boombox.lookup("BoomBox").set<Scale>(200);


#if 0
        auto sponza = load_gltf("assets/models/sponza/NewSponza_Main_glTF_002.gltf", root_node, bundle);

        ecs.entity("My Sponza")
            .child_of(root_node)
            .is_a(sponza);
#endif
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
        c.projection = glm::perspective(45.0, 16.0 / 9.0, 0.01, 10000.0);


        std::chrono::high_resolution_clock clock = {};

        double time = float(std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count()) * 0.001f;
        double last_time = time;

        std::vector<float> times;
        times.resize(16);

        uint32_t times_buffer_idx = 0;

        float total_time = 0;

        constexpr bool test_scene = true;

        if (test_scene) {
            auto boombox_holder = ecs.entity("Boomboxes!")
                .child_of(root_node);

            set_entity_transform(boombox_holder);

            constexpr int num_boomboxes_1d = 50;

            for (int i = 0; i < num_boomboxes_1d; i++) {
                for (int j = 0; j < num_boomboxes_1d; j++) {
                    auto n = ecs.entity(std::format("Boombox {} {}", i, j).c_str()).is_a(boombox)
                        .set<Position>(glm::vec3{ (i * 8) - num_boomboxes_1d, -10, (j * 8) - num_boomboxes_1d })
                        .child_of(boombox_holder);

                }
            }


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



            MaterialHandle m = bundle.register_material({ .diffuse_color = glm::vec3(1), .metallic_roughness = {1, .1} });
            Model big_cube_model(cube_mesh, m);
            auto big_box = ecs.entity("Big Box")
                .child_of(root_node)
                .set<Scale>(glm::vec3{ 100, 100, 1 })
                .add<Rotation>()
                .set<Position>(glm::vec3{ -50, 50, 20 })
                .set<Model>(big_cube_model);

        }


       


        auto light = ecs.entity("MainLight")
            .child_of(root_node)
            .add<Scale>()
            .add<Rotation>()
            .set<Position>(glm::vec3(0, 3, 0))
            .set<Light>({ glm::vec3(1) , 1200});

        auto lights = ecs.entity("Lights")
            .add<Rotation>()
            .add<Scale>()
            .set<Position>({})
            .child_of(root_node);

        for (int i = 0; i < 40; i++) {
            auto e = ecs.entity(std::format("Light {}", i).c_str())
                .child_of(lights)
                .set<Light>({ random_vec3(0, 1) , random_float(10, 100) });

            set_entity_transform(e, random_vec3(-10, 10));
        }



        while (!glfwWindowShouldClose(renderer.get_platform_window())) {
            double delta_time = time - last_time;
            last_time = time;
            time = double(std::chrono::duration_cast<std::chrono::nanoseconds>(clock.now().time_since_epoch()).count()) * 0.001 * 0.001 * 0.001;
            total_time -= times[times_buffer_idx % 16];
            total_time += delta_time;
            times[times_buffer_idx % 16] = delta_time;
            times_buffer_idx++;


            float move_speed = glfwGetKey(renderer.get_platform_window(), GLFW_KEY_LEFT_SHIFT) ? 50.0f : 10.0f;

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_W)) {
                c.position += c.get_heading() * (float)delta_time * move_speed;
            }

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_S)) {
                c.position -= c.get_heading() * (float)delta_time * move_speed;
            }

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_A)) {
                c.position -= glm::cross(c.get_heading(), glm::vec3(0, 1, 0)) * (float)delta_time * move_speed;
            }

            if (glfwGetKey(renderer.get_platform_window(), GLFW_KEY_D)) {
                c.position += glm::cross(c.get_heading(), glm::vec3(0, 1, 0)) * (float)delta_time * move_speed;
            }


            glm::dvec2 mouse_pos = {};
            glfwGetCursorPos(renderer.get_platform_window(), &mouse_pos.x, &mouse_pos.y);

            static double sensitivity = 1.0;
            constexpr double min_sens = 0.01;
            constexpr double max_sens = 2.0;
            constexpr double sens_step = 0.01;

            static glm::dvec2 last_mouse_pos = mouse_pos;
            glm::dvec2 mouse_delta = mouse_pos - last_mouse_pos;
            
            mouse_delta /= (250.0 / sensitivity);

            last_mouse_pos = mouse_pos;

            if (glfwGetMouseButton(renderer.get_platform_window(), GLFW_MOUSE_BUTTON_1)) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    constexpr double look_limit = (std::numbers::pi * 0.99) / 2;
                    c.rotation.y = std::max(std::min(c.rotation.y - mouse_delta.y, look_limit), -look_limit);
                    c.rotation.x -= mouse_delta.x;
                }
                    //c.rotation -= mouse_delta / 250.0;
            }


            glm::mat4 vp = c.projection * c.view();

            renderer.begin_frame();

            draw_entity_inspector(bundle, root_node, c);


            ImGui::Begin("Camera Controls");

            ImGui::Text("%f ms (%.02f fps)", (total_time / 16.f) * 1000.f, 1.f / (total_time / 16));

            ImGui::Text("%llu", bundle.get_rendered_tri_count());

            ImGui::BeginGroup();

            ImGui::DragFloat3("Camera Position", (float*)&c.position);
            ImGui::DragFloat2("Camera Rotation", (float*)&c.rotation, 3.14f / 360.f);

            ImGui::DragScalar("Sensitivity", ImGuiDataType_Double, &sensitivity, sens_step, &min_sens, &max_sens);

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


            bundle.render(c);


            renderer.end_frame();
        }
    }
    renderer.shutdown();
}
