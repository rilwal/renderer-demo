

#include <future>

#include "gltf.hpp"

#include "glm.hpp"
#include "gtx/quaternion.hpp"
#include "gtx/matrix_decompose.hpp"

#include "fastgltf/parser.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/tools.hpp"

#include "util.hpp"
#include "renderer.hpp"
#include "assets/image/dds.hpp"




uint64_t GLTF::get_texture(MeshBundle& mb, size_t texture_idx) {
    if (!m_texture_map.contains(texture_idx)) {
        auto& texture = m_asset.textures[texture_idx];

        // First let's load the image
        // TODO: Make sure it exists, and do somethign if it doesn't
        //      Might crash!!!
        auto& image = m_asset.images[*texture.imageIndex];
        
        // First let's make an image!
        std::unique_ptr<Image> i = {};
        
        std::filesystem::path png_src;

        bool from_buffer = false;

        if (auto image_buffer = std::get_if<fastgltf::sources::Vector>(&image.data); image_buffer) {
            png_src = "Internal Buffer";
            i = std::move(load_image(image_buffer->bytes, std::format("Internal Buffer in GLTF File {}", m_path.string())));
        }

        if (auto image_uri = std::get_if<fastgltf::sources::URI>(&image.data); image_uri) {
            std::filesystem::path path = image_uri->uri.fspath();
            std::filesystem::path cwd_relative_path = m_asset_dir.string() + "/" + path.string();
            std::filesystem::path dds_path = cwd_relative_path;
            dds_path.replace_extension(".dds");

            png_src = cwd_relative_path;
            i = load_image(std::filesystem::exists(dds_path) ? dds_path : cwd_relative_path);
        }

        Sampler my_sampler = {};

        if (texture.samplerIndex) {
            auto& gltf_sampler = m_asset.samplers[*texture.samplerIndex];
            if (gltf_sampler.magFilter) my_sampler.mag_filter = static_cast<TextureFilter>(*gltf_sampler.magFilter);
            if (gltf_sampler.minFilter) my_sampler.min_filter = static_cast<TextureFilter>(*gltf_sampler.minFilter);;

            my_sampler.wrap_s = static_cast<TextureWrap>(gltf_sampler.wrapS);
            my_sampler.wrap_t = static_cast<TextureWrap>(gltf_sampler.wrapT);
        }

        uint64_t handle = make_bindless_texture(std::move(i), my_sampler);
        m_texture_map[texture_idx] = handle;
    }

    return m_texture_map[texture_idx];
}


uint32_t GLTF::get_material(MeshBundle& mb, size_t material_idx) {
    // TODO: Think about using RGBA over RGB more often haha
    //          Alternatively support many different texture formats!!

    if (!m_material_map.contains(material_idx)) {
        auto& material = m_asset.materials[material_idx];

        Material m = {};

        m.diffuse_color = glm::vec3(material.pbrData.baseColorFactor[0], material.pbrData.baseColorFactor[1], material.pbrData.baseColorFactor[2]);
        m.metallic_roughness = glm::vec2(material.pbrData.metallicFactor, material.pbrData.roughnessFactor);

        if (material.pbrData.baseColorTexture) {
            auto texture_idx = material.pbrData.baseColorTexture->textureIndex;

            m.diffuse_texture = get_texture(mb, texture_idx);
        }

        if (material.normalTexture) {
            auto texture_idx = material.normalTexture->textureIndex;
            m.normal_map = get_texture(mb, texture_idx);
        }

        if (material.pbrData.metallicRoughnessTexture) {
            auto texture_idx = material.pbrData.metallicRoughnessTexture->textureIndex;
            m.metalic_roughness_texture = get_texture(mb, texture_idx);
        }

        if (material.alphaMode == fastgltf::AlphaMode::Blend) {
            m.blend = true;
        }

        m_material_map[material_idx] = mb.register_material(m);
    }

    return m_material_map[material_idx];
}


void GLTF::get_model(MeshBundle& mb, size_t mesh_idx, flecs::entity node_entity) {
    //if (!m_model_map.contains(mesh_idx)) {
        auto& mesh = m_asset.meshes[mesh_idx];
        bool flat = mesh.primitives.size() == 1;

        int i = 0;
        for (auto& primitive : mesh.primitives) {
            MeshHandle mesh_handle = 0;
            MaterialHandle material_handle = default_material;

            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                std::cerr << "We only know how to render triangles!\n";
                continue;
            }

            if (primitive.materialIndex) {
                material_handle = get_material(mb,  *primitive.materialIndex);
            }

            Ref<Mesh> m = make_ref<Mesh>();

            auto& indices_accessor = m_asset.accessors[*primitive.indicesAccessor];
            m->indices.resize(indices_accessor.count);
            fastgltf::copyFromAccessor<uint32_t>(m_asset, indices_accessor, m->indices.data());

            for (auto& [name, accessor_idx] : primitive.attributes) {
                auto& accessor = m_asset.accessors[accessor_idx];
                if (name == "POSITION") {
                    m->vertices.resize(accessor.count);
                    fastgltf::copyFromAccessor<glm::vec3>(m_asset, accessor, m->vertices.data());
                }
                else if (name == "NORMAL") {
                    m->normals.resize(accessor.count);
                    fastgltf::copyFromAccessor<glm::vec3>(m_asset, accessor, m->normals.data());
                }
                else if (name == "TEXCOORD_0") {
                    m->uvs.resize(accessor.count);
                    fastgltf::copyFromAccessor<glm::vec2>(m_asset, accessor, m->uvs.data());
                }
                else if (name == "TANGENT") {
                    m->tans.resize(accessor.count);
                    fastgltf::copyFromAccessor<glm::vec4>(m_asset, accessor, m->tans.data());
                }
                else {
                    // std::cerr << "Unused primitive attribute: " << name << "\n";
                }
            }

            mesh_handle = mb.add_entry(m);

            if (flat) {
                node_entity.set<Model>({ mesh_handle, material_handle });
            }
            else {
                ecs.entity(std::format("Primitive {}", i++).c_str())
                    .child_of(node_entity)
                    .add<LocalTransform>()
                    .set<Model>({ mesh_handle, material_handle });
            }

        }

        //m_model_map[mesh_idx] = model;
    //}

    //return;
}


Light GLTF::get_light(size_t light_idx) {
    auto& light = m_asset.lights[light_idx];

    return {
        .color = glm::vec3(light.color[0], light.color[1], light.color[2]),
        .intensity = light.intensity
    };
}


void GLTF::iterate_node_list(MeshBundle& mb, NodeList node_list, flecs::entity parent) {
    for (auto& node_index : node_list) {
        auto& node = m_asset.nodes[node_index];

        auto node_entity = ecs.prefab(node.name.c_str())
            .child_of(parent);

        fastgltf::Node::TRS transform = std::get<fastgltf::Node::TRS>(node.transform);

        Position node_translation = glm::vec3{ transform.translation[0], transform.translation[1], transform.translation[2] };
        Rotation node_rotation = glm::quat(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
        Scale node_scale = glm::vec3{ transform.scale[0], transform.scale[1], transform.scale[2] };

        glm::mat4 local_transform = node_translation.mat4() * node_rotation.mat4() * node_scale.mat4();


        glm::mat4 parent_transform = glm::mat4(1);

        if (parent.has<TransformComponent, World>()) {
            parent_transform = *parent.get<TransformComponent, World>();
        }


        glm::mat4 world_transform = parent_transform * local_transform;

        node_entity.set<TransformComponent, Local>({ local_transform });
        node_entity.set<TransformComponent, World>({ world_transform });

        if (node.meshIndex) {
            size_t mesh_index = *node.meshIndex;
            get_model(mb, mesh_index, node_entity);
            //node_entity.set<Model>(m);
        }

        if (node.lightIndex) {
            Light l = get_light(*node.lightIndex);
            node_entity.set<Light>(l);
        }

        if (node.children.size()) {
            iterate_node_list(mb, node.children, node_entity);
        }
    }
}

flecs::entity GLTF::load(std::filesystem::path path, flecs::entity parent, MeshBundle& mb) {
    m_asset_dir = path.parent_path();
    m_path = path;

    fastgltf::Parser parser (fastgltf::Extensions::KHR_lights_punctual);
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    {
        auto asset = parser.loadGLTF(&data, path.parent_path(), fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::GenerateMeshIndices);
        if (auto error = asset.error(); error != fastgltf::Error::None) {
            // Some error occurred while reading the buffer, parsing the JSON, or validating the data.
            std::cerr << "Failed to load GLTF " << path << "\n";
        }
        m_asset = std::move(asset.get());
    }

    auto gltf_file_node = ecs.prefab(path.stem().string().c_str())
        .add<TransformComponent, Local>()
        .add<TransformComponent, World>();

    for (auto& scene : m_asset.scenes) {
        // At some point I want to make some kind of prefab system,
        // but for now I will load all scenes on top of one another haha
        flecs::entity scene_node;

        if (m_asset.scenes.size() == 1) {
            scene_node = gltf_file_node;
        }
        else {
            const char* name = scene.name.c_str();

            scene_node = ecs.prefab(name)
                .add<TransformComponent, Local>()
                .add<TransformComponent, World>()
                .child_of(gltf_file_node);
        }

        iterate_node_list(mb, scene.nodeIndices, scene_node);
    }

    return gltf_file_node;
}


flecs::entity load_gltf(std::filesystem::path path, flecs::entity root, MeshBundle& mb) {
    GLTF gltf;
    return gltf.load(path, root, mb);
}