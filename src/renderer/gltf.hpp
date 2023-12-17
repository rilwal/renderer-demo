#pragma once

#include <filesystem>
#include <map>

#include "fastgltf/types.hpp"

#include "ecs_componets.hpp"

#include <string>
#include <initializer_list>



struct MeshBundle;

class GLTF {
public:
	using NodeList = FASTGLTF_FG_PMR_NS::MaybeSmallVector<std::size_t>;

	Model get_model(MeshBundle& mb, size_t mesh_idx);
	uint32_t get_material(MeshBundle& mb, size_t material_idx);
	uint64_t get_texture(MeshBundle& mb, size_t texture_idx);

	void iterate_node_list(MeshBundle& mb, NodeList node_list, flecs::entity parent);
	flecs::entity load(std::filesystem::path path, flecs::entity root, MeshBundle& mb);

private:
	std::filesystem::path m_path;
	std::filesystem::path m_asset_dir; // The directory the asset lives in
	fastgltf::Asset m_asset;

	std::map<size_t, Model> m_model_map;
	std::map<size_t, MaterialHandle> m_material_map;
	std::map<size_t, uint64_t> m_texture_map;
};

flecs::entity load_gltf(std::filesystem::path path, flecs::entity root, MeshBundle& mb);