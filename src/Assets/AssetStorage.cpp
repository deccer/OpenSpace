#include "AssetStorage.hpp"
#include "../Core/Types.hpp"
#include "AssetImage.hpp"
#include "AssetTexture.hpp"
#include "AssetSampler.hpp"
#include "AssetMaterial.hpp"
#include "AssetMesh.hpp"
#include "AssetModel.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <parallel_hashmap/phmap.h>
#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include <optional>
#include <ranges>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>

auto GetSafeResourceName(
    const char* const baseName,
    const char* const text,
    const char* const resourceType,
    const std::size_t resourceIndex) -> std::string {

    return text == nullptr || strlen(text) == 0
           ? std::format("{}-{}-{}", baseName, resourceType, resourceIndex)
           : std::format("{}-{}", baseName, text);
}

auto GetImageIndex(
    const fastgltf::Asset& fgAsset,
    const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {

    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
}

auto GetImageIndex(
    const fastgltf::Asset& fgAsset,
    const std::optional<fastgltf::NormalTextureInfo>& textureInfo) -> std::optional<size_t> {

    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
}

auto GetSamplerIndex(
    const fastgltf::Asset& fgAsset,
    const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {

    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.samplerIndex;
}

auto GetSamplerIndex(
    const fastgltf::Asset& fgAsset,
    const std::optional<fastgltf::NormalTextureInfo>& textureInfo) -> std::optional<size_t> {

    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.samplerIndex;
}

class TAssetStorage::Implementation {
public:
    Implementation() = default;
    ~Implementation() = default;

    auto GetAssetModel(const std::string& assetName) -> TAssetModel&;
    auto GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh&;

    auto ImportModel(
        const std::string& modelName,
        const std::filesystem::path& filePath) -> std::expected<std::string, std::string>;
private:
    auto LoadImages(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel,
        const std::filesystem::path& basePath) -> void;
    auto LoadSamplers(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel) -> void;
    auto LoadTextures(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel) -> void;
    auto LoadMaterials(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel) -> void;
    auto LoadMeshes(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel) -> void;
    auto LoadNodes(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel) -> void;

    phmap::flat_hash_map<std::string, TAssetImage> _images;
    phmap::flat_hash_map<std::string, TAssetSampler> _samplers;
    phmap::flat_hash_map<std::string, TAssetTexture> _textures;
    phmap::flat_hash_map<std::string, TAssetMaterial> _materials;
    phmap::flat_hash_map<std::string, TAssetMesh> _meshes;
    phmap::flat_hash_map<std::string, TAssetModel> _models;
};

TAssetStorage::TAssetStorage()
    : _implementation(CreateScoped<Implementation>()) {
}

TAssetStorage::~TAssetStorage() {
}

auto TAssetStorage::GetAssetModel(const std::string& assetName) -> TAssetModel& {
    return _implementation->GetAssetModel(assetName);
}

auto TAssetStorage::GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& {
    return _implementation->GetAssetMesh(assetMeshName);
}

auto TAssetStorage::ImportModel(
    const std::string& modelName,
    const std::filesystem::path& filePath) const -> std::expected<std::string, std::string> {

    return _implementation->ImportModel(modelName, filePath);
}

auto TAssetStorage::Implementation::LoadImages(
        const fastgltf::Asset& fgAsset,
        TAssetModel& assetModel,
        const std::filesystem::path& basePath) -> void {

    auto images = std::vector<TAssetImage>(fgAsset.images.size(), TAssetImage{});
    const auto imageIndices = std::ranges::iota_view{0uz, fgAsset.images.size()};

    std::for_each(
        poolstl::execution::par,
        imageIndices.begin(),
        imageIndices.end(),
        [&](const size_t imageIndex) -> void {

            const auto& fgImage = fgAsset.images[imageIndex];
            const auto imageName = GetSafeResourceName(assetModel.Name.data(), fgImage.name.data(), "image", imageIndex);

            auto& image = images[imageIndex];
            assetModel.Images[imageIndex] = imageName;

            image.Name = imageName;
    });

    for (const auto& image : images) {
        _images[image.Name] = image;
    }
}

auto TAssetStorage::Implementation::LoadSamplers(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    auto samplers = std::vector<TAssetSampler>(fgAsset.samplers.size(), TAssetSampler{});
    const auto samplerIndices = std::ranges::iota_view{0uz, fgAsset.samplers.size()};

    std::for_each(
        poolstl::execution::seq,
        samplerIndices.begin(),
        samplerIndices.end(),
        [&](const size_t samplerIndex) -> void {

            const auto& fgSampler = fgAsset.samplers[samplerIndex];
            const auto samplerName = GetSafeResourceName(assetModel.Name.data(), fgSampler.name.data(), "sampler", samplerIndex);

            auto& sampler = samplers[samplerIndex];
            assetModel.Samplers[samplerIndex] = samplerName;

            sampler.Name = samplerName;
    });

    for (const auto& sampler : samplers) {
        _samplers[sampler.Name] = sampler;
    }
}

auto TAssetStorage::Implementation::LoadTextures(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    auto textures = std::vector<TAssetTexture>(fgAsset.textures.size(), TAssetTexture{});
    const auto textureIndices = std::ranges::iota_view{0uz, fgAsset.textures.size()};

    std::for_each(
        poolstl::execution::seq,
        textureIndices.begin(),
        textureIndices.end(),
        [&](const size_t textureIndex) -> void {

            const auto& fgTexture = fgAsset.textures[textureIndex];
            const auto textureName = GetSafeResourceName(assetModel.Name.data(), fgTexture.name.data(), "texture", textureIndex);

            auto& texture = textures[textureIndex];
            assetModel.Textures[textureIndex] = textureName;

            texture.Name = textureName;
    });

    for (const auto& texture : textures) {
        _textures[texture.Name] = texture;
    }
}

auto TAssetStorage::Implementation::LoadMaterials(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    auto materials = std::vector<TAssetMaterial>(fgAsset.materials.size(), TAssetMaterial{});
    const auto materialIndices = std::ranges::iota_view{0uz, fgAsset.materials.size()};

    std::for_each(
        poolstl::execution::par,
        materialIndices.begin(),
        materialIndices.end(),
        [&](const size_t materialIndex) -> void {

            const auto& fgMaterial = fgAsset.materials[materialIndex];
            const auto& materialName = GetSafeResourceName(assetModel.Name.data(), fgMaterial.name.data(), "material", materialIndex);

            auto& material = materials[materialIndex];
            assetModel.Materials[materialIndex] = materialName;

            material.Name = materialName;
/*
            TAssetMaterialChannel BaseColorChannel;
                TAssetMaterialChannel NormalChannel;
                TAssetMaterialChannel SpecularChannel;
                TAssetMaterialChannel EmissiveChannel;
                TAssetMaterialChannel ArmChannel;
                TAssetMaterialChannel OcclusionChannel;
                TAssetMaterialChannel RoughnessChannel;
                TAssetMaterialChannel MetalnessChannel;
                bool IsArmSeparateChannels; // if true, use Occlusion/Roughness/Metalness-Channel, else ArmChannel
 */
            material.IsArmPacked = fgMaterial.packedOcclusionRoughnessMetallicTextures != nullptr && fgMaterial.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture.has_value();
            if (material.IsArmPacked) {

                const auto& fgArmTexture = fgAsset.textures[fgMaterial.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture.value().textureIndex];
                const auto& armSamplerIndex = fgArmTexture.samplerIndex;
                material.ArmChannel = TAssetMaterialChannel {
                    .Type = TAssetMaterialChannelType::Scalar,
                    .SamplerName = armSamplerIndex.has_value() ? assetModel.Samplers[*armSamplerIndex] : "default-sampler-0",
                    .TextureName = assetModel.Textures[fgMaterial.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture.value().textureIndex],
                };
            } else {

                if (fgMaterial.occlusionTexture.has_value()) {
                    const auto& fgOcclusionTexture = fgAsset.textures[fgMaterial.occlusionTexture.value().textureIndex];
                    const auto& occlusionSamplerIndex = fgOcclusionTexture.samplerIndex;
                    material.OcclusionChannel = TAssetMaterialChannel {
                        .Type = TAssetMaterialChannelType::Scalar,
                        .SamplerName = occlusionSamplerIndex.has_value() ? assetModel.Samplers[*occlusionSamplerIndex] : "default-sampler-0",
                        .TextureName = assetModel.Textures[fgMaterial.occlusionTexture.value().textureIndex],
                    };
                }
                if (fgMaterial.pbrData.metallicRoughnessTexture.has_value()) {
                    const auto& fgMetallicRoughnessTexture = fgAsset.textures[fgMaterial.pbrData.metallicRoughnessTexture.value().textureIndex];
                    const auto& metallicRoughnessSamplerIndex = fgMetallicRoughnessTexture.samplerIndex;
                    material.MetallicRoughnessChannel = TAssetMaterialChannel {
                        .Type = TAssetMaterialChannelType::Scalar,
                        .SamplerName = metallicRoughnessSamplerIndex.has_value() ? assetModel.Samplers[*metallicRoughnessSamplerIndex] : "default-sampler-0",
                        .TextureName = assetModel.Textures[fgMaterial.pbrData.metallicRoughnessTexture.value().textureIndex],
                    };
                }
            }
            material.IsUnlit = fgMaterial.unlit;
            material.IsDoubleSided = fgMaterial.doubleSided;
            material.BaseColor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
            material.NormalStrength = fgMaterial.normalTexture.has_value() ? fgMaterial.normalTexture->scale : 1.0f;
            material.Metallic = fgMaterial.pbrData.metallicFactor;
            material.Roughness = fgMaterial.pbrData.roughnessFactor;
            material.EmissiveFactor = glm::make_vec3(fgMaterial.emissiveFactor.data());
            material.IndexOfRefraction = fgMaterial.ior;
    });

    for (const auto& material : materials) {
        _materials[material.Name] = material;
    }
}

auto TAssetStorage::Implementation::LoadMeshes(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    struct TFlatPrimitive {
        size_t PrimitiveId;
        size_t MeshId;
        std::optional<size_t> MaterialId;
        std::string Name;
    };

    auto totalPrimitiveCount = 0uz;
    for (size_t meshId = 0; meshId < fgAsset.meshes.size(); ++meshId) {
        totalPrimitiveCount += fgAsset.meshes[meshId].primitives.size();
    }

    std::vector<TFlatPrimitive> flatPrimitives;
    flatPrimitives.reserve(totalPrimitiveCount);

    size_t primitiveId = 0;
    for (size_t meshId = 0; meshId < fgAsset.meshes.size(); ++meshId) {
        const auto& fgMesh = fgAsset.meshes[meshId];
        for (size_t primitiveIndex = 0; primitiveIndex < fgMesh.primitives.size(); ++primitiveIndex) {
            flatPrimitives.push_back(TFlatPrimitive{
                .PrimitiveId = primitiveIndex,
                .MeshId = meshId,
                .MaterialId = fgMesh.primitives[primitiveIndex].materialIndex,
                .Name = GetSafeResourceName(assetModel.Name.data(), nullptr, "mesh", primitiveId++)
            });
        }
    }

    assetModel.Meshes.resize(flatPrimitives.size());
    auto meshes = std::vector<TAssetMesh>(flatPrimitives.size(), TAssetMesh());

    for (size_t primitiveIndex = 0; primitiveIndex < flatPrimitives.size(); ++primitiveIndex) {
        const auto& flatPrimitive = flatPrimitives[primitiveIndex];
        const auto& fgPrimitive = fgAsset.meshes[flatPrimitive.MeshId].primitives[flatPrimitive.PrimitiveId];
        assetModel.Meshes[primitiveIndex] = flatPrimitive.Name;

        auto& mesh = meshes[primitiveIndex];
        mesh.Name = flatPrimitive.Name;
        mesh.MaterialName = flatPrimitive.MaterialId.has_value()
            ? assetModel.Materials[flatPrimitive.MaterialId.value()]
            : "default-material-0";

        auto& indices = fgAsset.accessors[fgPrimitive.indicesAccessor.value()];
        mesh.Indices.resize(indices.count);
        fastgltf::copyFromAccessor<uint32_t>(fgAsset, indices, mesh.Indices.data());

        auto& positions = fgAsset.accessors[fgPrimitive.findAttribute("POSITION")->accessorIndex];
        mesh.Positions.resize(positions.count);
        fastgltf::copyFromAccessor<glm::vec3>(fgAsset, positions, mesh.Positions.data());
        if (auto* normalsAttribute = fgPrimitive.findAttribute("NORMAL"); normalsAttribute != fgPrimitive.attributes.end()) {
            auto& normals = fgAsset.accessors[normalsAttribute->accessorIndex];
            mesh.Normals.resize(normals.count);
            fastgltf::copyFromAccessor<glm::vec3>(fgAsset, normals, mesh.Normals.data());
        }
        if (auto* uv0Attribute = fgPrimitive.findAttribute("TEXCOORD_0"); uv0Attribute != fgPrimitive.attributes.end()) {
            auto& uv0s = fgAsset.accessors[uv0Attribute->accessorIndex];
            mesh.Uvs.resize(uv0s.count);
            fastgltf::copyFromAccessor<glm::vec2>(fgAsset, uv0s, mesh.Uvs.data());
        }
        if (auto* tangentAttribute = fgPrimitive.findAttribute("TANGENT"); tangentAttribute != fgPrimitive.attributes.end()) {
            auto& tangents = fgAsset.accessors[tangentAttribute->accessorIndex];
            mesh.Tangents.resize(tangents.count);
            fastgltf::copyFromAccessor<glm::vec4>(fgAsset, tangents, mesh.Tangents.data());
        } else  {
            mesh.Tangents.resize(mesh.Positions.size());
            std::fill_n(mesh.Tangents.begin(), mesh.Positions.size(), glm::vec4{1.0f});
        }
    }

    for (const auto mesh : meshes) {
        _meshes[mesh.Name] = mesh;
    }
}

auto TAssetStorage::Implementation::LoadNodes(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    auto parseNode = [&](
        this const auto& self,
        std::string_view baseName,
        uint32_t nodeId,
        const fastgltf::Node& fgNode,
        const std::vector<std::string>& meshNames,
        const std::optional<std::vector<fastgltf::Node>>& allNodes,
        TAssetModelNode& assetNode) -> void {

        assetNode.Name = GetSafeResourceName(baseName.data(), fgNode.name.data(), "node", nodeId++);

        auto& trs = std::get<fastgltf::TRS>(fgNode.transform);
        assetNode.LocalPosition = glm::make_vec3(trs.translation.data());;
        assetNode.LocalRotation = glm::quat{trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]};
        assetNode.LocalScale = glm::make_vec3(trs.scale.data());

        if (fgNode.meshIndex && !meshNames.empty()) {
            size_t meshId = *fgNode.meshIndex;
            if (meshId < meshNames.size()) {
                assetNode.MeshName = meshNames[meshId];
            } else {
                assetNode.MeshName = std::nullopt;
            }
        } else {
            assetNode.MeshName = std::nullopt;
        }

        if (!fgNode.children.empty() && allNodes) {
            for (size_t childIndex : fgNode.children) {
                if (childIndex < allNodes->size()) {
                    const auto& childNode = (*allNodes)[childIndex];
                    TAssetModelNode childAssetNode;
                    self(baseName, nodeId, childNode, meshNames, allNodes, childAssetNode);
                    assetNode.Children.push_back(std::move(childAssetNode));
                }
            }
        }
    };

    uint32_t nodeId = 0;
    for (const auto& fgNode : fgAsset.nodes) {
        TAssetModelNode rootNode;
        parseNode(assetModel.Name, nodeId, fgNode, assetModel.Meshes, fgAsset.nodes, rootNode);
        assetModel.Hierarchy.push_back(std::move(rootNode));
    }
}

auto TAssetStorage::Implementation::GetAssetModel(const std::string& assetName) -> TAssetModel& {
    return _models[assetName];
}

auto TAssetStorage::Implementation::GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& {
    return _meshes[assetMeshName];
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
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::DecomposeNodeMatrices;
    const auto parentPath = filePath.parent_path();
    auto loadResult = parser.loadGltf(dataResult.get(), parentPath, gltfOptions);
    if (loadResult.error() != fastgltf::Error::None)
    {
        return std::unexpected(std::format("fastgltf: Failed to parse glTF: {}", fastgltf::getErrorMessage(loadResult.error())));
    }

    const auto& fgAsset = loadResult.get();

    TAssetModel assetModel = {};
    assetModel.Name = modelName;
    assetModel.Animations.resize(fgAsset.animations.size());
    assetModel.Skins.resize(fgAsset.skins.size());
    assetModel.Images.resize(fgAsset.images.size());
    assetModel.Samplers.resize(fgAsset.samplers.size());
    assetModel.Textures.resize(fgAsset.textures.size());
    assetModel.Materials.resize(fgAsset.materials.size());
    assetModel.Meshes.resize(fgAsset.meshes.size());

    LoadImages(fgAsset, assetModel, "");
    LoadSamplers(fgAsset, assetModel);
    LoadTextures(fgAsset, assetModel);
    LoadMaterials(fgAsset, assetModel);
    LoadMeshes(fgAsset, assetModel);
    LoadNodes(fgAsset, assetModel);

    _models[modelName] = assetModel;

    return assetModel.Name;
}
