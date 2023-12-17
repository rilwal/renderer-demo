
#include "gltf.hpp"

#include "glm.hpp"
#include "gtx/quaternion.hpp"

#include "fastgltf/parser.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/tools.hpp"

#include "stb_image.h"

#include "renderer.hpp"


uint64_t GLTF::get_texture(MeshBundle& mb, size_t texture_idx) {
    if (!m_texture_map.contains(texture_idx)) {
        auto& texture = m_asset.textures[texture_idx];

        // First let's load the image
        // TODO: Make sure it exists, and do somethign if it doesn't
        //      Might crash!!!
        auto& image = m_asset.images[*texture.imageIndex];

        // Let's try the simplest implementation: bindless textures
        // It's probably better to use a virutal texture atlas or a teture array though!
        uint32_t texture_id = 0;
        uint32_t sampler_id = 0;

        glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
        glCreateSamplers(1, &sampler_id);

        int w, h, channels;
        uint8_t* data = nullptr;


        if (auto image_buffer = std::get_if<fastgltf::sources::Vector>(&image.data); image_buffer) {
            data = stbi_load_from_memory(image_buffer->bytes.data(), image_buffer->bytes.size(), &w, &h, &channels, 4);

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);

            glGenerateTextureMipmap(texture_id);
        }

        if (auto image_uri = std::get_if<fastgltf::sources::URI>(&image.data); image_uri) {
            std::filesystem::path path = image_uri->uri.fspath();
            std::filesystem::path cwd_relative_path = m_asset_dir.string() + "/" + path.string();

            data = stbi_load(cwd_relative_path.string().c_str(), &w, &h, &channels, 4);

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);

            glGenerateTextureMipmap(texture_id);
        }

        if (texture.samplerIndex) {
            auto& sampler = m_asset.samplers[*texture.samplerIndex];
            uint32_t min_filter = (uint32_t)sampler.minFilter.value_or(GL_LINEAR);
            uint32_t mag_filter = (uint32_t)sampler.magFilter.value_or(GL_LINEAR);

            glSamplerParameterf(sampler_id, GL_TEXTURE_MIN_FILTER, (uint32_t)*sampler.minFilter);
            glSamplerParameterf(sampler_id, GL_TEXTURE_MAG_FILTER, (uint32_t)*sampler.magFilter);
            glSamplerParameterf(sampler_id, GL_TEXTURE_WRAP_S, (uint32_t)sampler.wrapS);
            glSamplerParameterf(sampler_id, GL_TEXTURE_WRAP_T, (uint32_t)sampler.wrapT);

            float border_color[4] = { 0, 0, 0, 0 };
            glSamplerParameterfv(sampler_id, GL_TEXTURE_BORDER_COLOR, border_color);

        }
        else {
            std::cerr << "Textures without samplers are not currently supported!\n";
        }

        uint64_t handle = glGetTextureSamplerHandleARB(texture_id, sampler_id);
        glMakeTextureHandleResidentARB(handle); // TODO: Investigate the effect of making too many textures resident?
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


Model GLTF::get_model(MeshBundle& mb, size_t mesh_idx) {
    if (!m_model_map.contains(mesh_idx)) {
        auto& mesh = m_asset.meshes[mesh_idx];
        Model model;

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
                    fastgltf::copyFromAccessor<glm::vec3>(m_asset, accessor, m->tans.data());
                }
                else {
                    // std::cerr << "Unused primitive attribute: " << name << "\n";
                }
            }

            mesh_handle = mb.add_entry(m);
            model.meshes.push_back({ mesh_handle, material_handle });
        }

        m_model_map[mesh_idx] = model;
    }

    return m_model_map[mesh_idx];
}

void GLTF::iterate_node_list(MeshBundle& mb, NodeList node_list, flecs::entity parent) {
    for (auto& node_index : node_list) {
        auto& node = m_asset.nodes[node_index];

        auto node_entity = ecs.entity(node.name.c_str())
            .child_of(parent);

        fastgltf::Node::TRS transform = std::get<fastgltf::Node::TRS>(node.transform);

        Position node_translation = glm::vec3{ transform.translation[0], transform.translation[1], transform.translation[2] };
        Rotation node_rotation = glm::quat(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
        Scale node_scale = glm::vec3{ transform.scale[0], transform.scale[1], transform.scale[2] };

        node_entity.set<Position>(node_translation);
        node_entity.set<Rotation>(node_rotation);
        node_entity.set<Scale>(node_scale);

        glm::mat4 local_transform = node_translation.mat4() * node_rotation.mat4() * node_scale.mat4();

        glm::mat4 parent_transform = glm::mat4(1);

        if (parent.has<TransformComponent>()) {
            parent_transform = *parent.get<TransformComponent>();
        }

        glm::mat4 global_transform = parent_transform * local_transform;

        node_entity.set<TransformComponent>({ global_transform });

        if (node.meshIndex) {
            size_t mesh_index = *node.meshIndex;
            Model m = get_model(mb, mesh_index);

            mb.add_model_to_entity(node_entity, m);
        }

        if (node.children.size()) {
            iterate_node_list(mb, node.children, node_entity);
        }
    }
}

flecs::entity GLTF::load(std::filesystem::path path, flecs::entity parent, MeshBundle& mb) {
    m_asset_dir = path.parent_path();
    m_path = path;

    fastgltf::Parser parser;
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

    auto gltf_file_node = ecs.entity(m_path.stem().string().c_str())
        .add<Position>()
        .add<Rotation>()
        .add<Scale>()
        .child_of(parent);

    for (auto& scene : m_asset.scenes) {
        // At some point I want to make some kind of prefab system,
        // but for now I will load all scenes on top of one another haha
        flecs::entity scene_node;

        if (m_asset.scenes.size() == 1) {
            scene_node = gltf_file_node;
        }
        else {
            const char* name = scene.name.c_str();

            scene_node = ecs.entity(name)
                .add<Position>()
                .add<Rotation>()
                .add<Scale>()
                .child_of(gltf_file_node);
        }

        iterate_node_list(mb, scene.nodeIndices, scene_node);
    }

    return parent;
}


flecs::entity load_gltf(std::filesystem::path path, flecs::entity root, MeshBundle& mb) {
    GLTF gltf;
    return gltf.load(path, root, mb);
}