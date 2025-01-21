#include "AssetStorage.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <parallel_hashmap/phmap.h>

#include "../Core/Types.hpp"
#include "AssetTexture.hpp"
#include "AssetSampler.hpp"
#include "AssetMaterial.hpp"
#include "AssetMesh.hpp"
#include "AssetMeshInstance.hpp"
#include "AssetModel.hpp"

class TAssetStorage::Implementation {
public:
    Implementation() = default;
    ~Implementation() = default;

    auto GetAssetModel(const std::string& assetName) -> std::shared_ptr<TAssetModel>;
    auto GetAssetMesh(const std::string& assetMeshName) -> std::shared_ptr<void>;
    auto ImportModel(
        const std::string& modelName,
        const std::filesystem::path& filePath) -> std::expected<std::string, std::string>;
private:
    auto LoadImages(const fastgltf::Asset& fgAsset) -> void;
    auto LoadSamplers(const fastgltf::Asset& fgAsset) -> void;
    auto LoadMaterials(const fastgltf::Asset& fgAsset) -> void;

    phmap::flat_hash_map<std::string, TAssetModel> _models;
    phmap::flat_hash_map<std::string, TAssetTexture> _textures;
    phmap::flat_hash_map<std::string, TAssetMaterial> _materials;
};

TAssetStorage::TAssetStorage()
    : _implementation(CreateScoped<Implementation>()) {
}

TAssetStorage::~TAssetStorage() {
}

auto TAssetStorage::GetAssetModel(const std::string& assetName) -> std::shared_ptr<TAssetModel> {
    return _implementation->GetAssetModel(assetName);
}

auto TAssetStorage::GetAssetMesh(const std::string& assetMeshName) -> std::shared_ptr<void> {
    return _implementation->GetAssetMesh(assetMeshName);
}

auto TAssetStorage::ImportModel(
    const std::string& modelName,
    const std::filesystem::path& filePath) const -> std::expected<std::string, std::string> {

    return _implementation->ImportModel(modelName, filePath);
}

auto TAssetStorage::Implementation::LoadImages(const fastgltf::Asset& fgAsset) -> void {
    for (const auto& fgImage : fgAsset.images) {
    }
}

auto TAssetStorage::Implementation::LoadSamplers(const fastgltf::Asset& fgAsset) -> void {
    for (const auto& fgSampler : fgAsset.samplers) {

    }
}

auto TAssetStorage::Implementation::LoadMaterials(const fastgltf::Asset& fgAsset) -> void {
    for (const auto& fgMaterial : fgAsset.materials) {

    }
}

auto TAssetStorage::Implementation::GetAssetModel(const std::string& assetName) -> std::shared_ptr<TAssetModel> {
    return nullptr;
}

auto TAssetStorage::Implementation::GetAssetMesh(const std::string& assetMeshName) -> std::shared_ptr<void> {
    return nullptr;
}

auto TAssetStorage::Implementation::ImportModel(
    const std::string& modelName,
    const std::filesystem::path& filePath) -> std::expected<std::string, std::string> {

    if (!std::filesystem::exists(filePath)) {
        return std::unexpected(std::format("Unable to load model '{}'. File '{}' does not exist", modelName, filePath.string()));
    }

    constexpr auto parserOptions =
        fastgltf::Extensions::EXT_mesh_gpu_instancing |
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::EXT_meshopt_compression |
        fastgltf::Extensions::KHR_lights_punctual |
        fastgltf::Extensions::EXT_texture_webp |
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_texture_basisu |
        fastgltf::Extensions::MSFT_texture_dds |
        fastgltf::Extensions::KHR_materials_specular |
        fastgltf::Extensions::KHR_materials_ior |
        fastgltf::Extensions::KHR_materials_iridescence |
        fastgltf::Extensions::KHR_materials_volume |
        fastgltf::Extensions::KHR_materials_transmission |
        fastgltf::Extensions::KHR_materials_clearcoat |
        fastgltf::Extensions::KHR_materials_emissive_strength |
        fastgltf::Extensions::KHR_materials_sheen |
        fastgltf::Extensions::KHR_materials_unlit;
    fastgltf::Parser parser(parserOptions);

    auto dataResult = fastgltf::GltfDataBuffer::FromPath(filePath);
    if (dataResult.error() != fastgltf::Error::None) {
        return std::unexpected(std::format("fastgltf: Failed to load glTF data: {}", fastgltf::getErrorMessage(dataResult.error())));
    }

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages;
    const auto parentPath = filePath.parent_path();
    auto loadResult = parser.loadGltf(dataResult.get(), parentPath, gltfOptions);
    if (loadResult.error() != fastgltf::Error::None)
    {
        return std::unexpected(std::format("fastgltf: Failed to parse glTF: {}", fastgltf::getErrorMessage(loadResult.error())));
    }

    const auto& fgAsset = loadResult.get();
    LoadImages(fgAsset);
    LoadSamplers(fgAsset);
    LoadMaterials(fgAsset);

    return modelName;
}
