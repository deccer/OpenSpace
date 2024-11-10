#include "Assets.hpp"
#include "Io.hpp"
#include "Images.hpp"
#include "Helpers.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <algorithm>
#include <format>
#include <ranges>

#include <spdlog/spdlog.h>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include "Profiler.hpp"
#include "glm/gtc/type_ptr.hpp"

struct TAssetRawImageData {
    std::string Name;
    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;
    TAssetImageDataType ImageDataType = {};
};

std::unordered_map<std::string, TAsset> g_assets = {};

auto GetImageIndex(fastgltf::Asset& fgAsset, const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
}

auto GetImageIndex(fastgltf::Asset& fgAsset, const std::optional<fastgltf::NormalTextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
}

auto MimeTypeToImageDataType(fastgltf::MimeType mimeType) -> TAssetImageDataType {
    if (mimeType == fastgltf::MimeType::KTX2) {
        return TAssetImageDataType::CompressedKtx;
    }

    if (mimeType == fastgltf::MimeType::DDS) {
        return TAssetImageDataType::CompressedDds;
    }

    return TAssetImageDataType::Uncompressed;
}

auto CreateAssetRawImageData(
    const void* data,
    std::size_t dataSize,
    fastgltf::MimeType mimeType,
    std::string_view name) -> TAssetRawImageData {

    PROFILER_ZONESCOPEDN("CreateAssetImage");

    auto dataCopy = std::make_unique<std::byte[]>(dataSize);
    std::copy_n(static_cast<const std::byte*>(data), dataSize, dataCopy.get());

    return TAssetRawImageData {
        .Name = std::string(name),
        .EncodedData = std::move(dataCopy),
        .EncodedDataSize = dataSize,
        .ImageDataType = MimeTypeToImageDataType(mimeType)
    };
}

auto LoadImages(const std::string& modelName, TAsset& asset, fastgltf::Asset& fgAsset, const std::filesystem::path& filePath) -> void {

    asset.Images.resize(fgAsset.images.size());

    const auto imageIndices = std::ranges::iota_view{0ul, fgAsset.images.size()};
    std::for_each(
        poolstl::execution::par,
        imageIndices.begin(),
        imageIndices.end(),
        [&](size_t imageIndex) -> void {

        const auto& fgImage = fgAsset.images[imageIndex];

        const auto imageData = [&]{

            auto imageName = GetSafeResourceName(modelName.data(), fgImage.name.data(), "image", imageIndex);
            if (const auto* filePathUri = std::get_if<fastgltf::sources::URI>(&fgImage.data)) {
                auto filePathFixed = std::filesystem::path(filePathUri->uri.path());
                auto filePathParent = filePath.parent_path();
                if (filePathFixed.is_relative()) {
                    filePathFixed = filePath.parent_path() / filePathFixed;
                }
                auto [data, dataSize] = ReadBinaryFromFile(filePathFixed);
                return CreateAssetRawImageData(data.get(), dataSize, filePathUri->mimeType, imageName);
            }
            if (const auto* array = std::get_if<fastgltf::sources::Array>(&fgImage.data)) {
                return CreateAssetRawImageData(array->bytes.data(), array->bytes.size(), array->mimeType, imageName);
            }
            if (const auto* vector = std::get_if<fastgltf::sources::Vector>(&fgImage.data)) {
                return CreateAssetRawImageData(vector->bytes.data(), vector->bytes.size(), vector->mimeType, imageName);
            }
            if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&fgImage.data)) {
                auto& bufferView = fgAsset.bufferViews[view->bufferViewIndex];
                auto& buffer = fgAsset.buffers[bufferView.bufferIndex];
                if (const auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                    return CreateAssetRawImageData(
                        array->bytes.data() + bufferView.byteOffset,
                        bufferView.byteLength,
                        view->mimeType,
                        imageName);
                }
            }

            return TAssetRawImageData{};
        }();

        auto& assetImage = asset.Images[imageIndex];
        assetImage.ImageDataType = imageData.ImageDataType;
        if (assetImage.ImageDataType == TAssetImageDataType::CompressedKtx) {

        } else if (assetImage.ImageDataType == TAssetImageDataType::CompressedDds) {

        } else {
            int32_t width = 0;
            int32_t height = 0;
            int32_t components = 0;
            auto* pixels = LoadImageFromMemory(imageData.EncodedData.get(), imageData.EncodedDataSize, &width, &height, &components);

            assetImage.Name = imageData.Name;
            assetImage.Width = width;
            assetImage.Height = height;
            assetImage.Components = components;
            assetImage.Data.reset(pixels);
        }
    });
}

auto LoadMaterials(const std::string& assetName, TAsset& asset, fastgltf::Asset& fgAsset) -> void {

    const auto materialIndices = std::ranges::iota_view{0uz, fgAsset.materials.size()};
    asset.Materials.resize(fgAsset.materials.size());
    std::for_each(
        poolstl::execution::seq,
        materialIndices.begin(),
        materialIndices.end(),
        [&](size_t materialIndex) -> void {

        const auto& fgMaterial = fgAsset.materials[materialIndex];

        auto& material = asset.Materials[materialIndex];
        material.Name = GetSafeResourceName(assetName.data(), fgMaterial.name.data(), "material", materialIndex);
        material.BaseColorTextureIndex = GetImageIndex(fgAsset, fgMaterial.pbrData.baseColorTexture);
        material.NormalTextureIndex = GetImageIndex(fgAsset, fgMaterial.normalTexture);
        material.ArmTextureIndex = fgMaterial.packedOcclusionRoughnessMetallicTextures != nullptr
            ? GetImageIndex(fgAsset, fgMaterial.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture)
            : std::nullopt;
        material.EmissiveTextureIndex = GetImageIndex(fgAsset, fgMaterial.emissiveTexture);

        material.BaseColor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
        material.Metalness = fgMaterial.pbrData.metallicFactor;
        material.Roughness = fgMaterial.pbrData.roughnessFactor;
    });
}

auto LoadMeshes(const fastgltf::Asset& fgAsset,
                TAsset& assetScene,
                size_t meshIndex,
                size_t primitiveIndex) -> void {

    auto& primitive = fgAsset.meshes[meshIndex].primitives[primitiveIndex];
    if (!primitive.indicesAccessor.has_value() || primitive.findAttribute("POSITION") == primitive.attributes.end()) {
        return;
    }

    auto& sceneMesh = assetScene.Meshes.emplace_back();
    sceneMesh.MaterialIndex = primitive.materialIndex;

    auto& indices = fgAsset.accessors[primitive.indicesAccessor.value()];
    sceneMesh.Indices.resize(indices.count);
    fastgltf::copyFromAccessor<uint32_t>(fgAsset, indices, sceneMesh.Indices.data());

    auto& positions = fgAsset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
    sceneMesh.Positions.resize(positions.count);
    fastgltf::copyFromAccessor<glm::vec3>(fgAsset, positions, sceneMesh.Positions.data());

    if (auto* normalsAttribute = primitive.findAttribute("NORMAL"); normalsAttribute != primitive.attributes.end()) {
        auto& normals = fgAsset.accessors[normalsAttribute->accessorIndex];
        sceneMesh.Normals.resize(normals.count);
        fastgltf::copyFromAccessor<glm::vec3>(fgAsset, normals, sceneMesh.Normals.data());
    }

    if (auto* uv0Attribute = primitive.findAttribute("TEXCOORD_0"); uv0Attribute != primitive.attributes.end()) {
        auto& uv0s = fgAsset.accessors[uv0Attribute->accessorIndex];
        sceneMesh.Uvs.resize(uv0s.count);
        fastgltf::copyFromAccessor<glm::vec2>(fgAsset, uv0s, sceneMesh.Uvs.data());
    }

    if (auto* tangentAttribute = primitive.findAttribute("TANGENT"); tangentAttribute != primitive.attributes.end()) {
        auto& tangents = fgAsset.accessors[tangentAttribute->accessorIndex];
        sceneMesh.Tangents.resize(tangents.count);
        fastgltf::copyFromAccessor<glm::vec4>(fgAsset, tangents, sceneMesh.Tangents.data());
    }
}

auto LoadNodes(const fastgltf::Asset& asset,
               TAsset& assetScene,
               size_t nodeIndex,
               glm::mat4 parentTransform,
               std::vector<std::pair<size_t, size_t>>& meshOffsets) -> void {

    auto& node = asset.nodes[nodeIndex];

    glm::mat4 transform{1.0};

    if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform))
    {
        auto rotation = glm::quat{trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]};
        auto scale = glm::make_vec3(trs->scale.data());
        auto translation = glm::make_vec3(trs->translation.data());

        auto rotationMatrix = glm::mat4_cast(rotation);

        // T * R * S
        transform = glm::scale(glm::translate(glm::mat4(1.0f), translation) * rotationMatrix, scale);
    }
    else if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform))
    {
        const auto& m = *mat;
        transform = glm::make_mat4x4(m.data());
    }

    if (node.meshIndex.has_value()) {
        auto [meshIndex, meshCount] = meshOffsets[node.meshIndex.value()];
        for (auto i = 0; i < meshCount; ++i) {
            assetScene.Instances.emplace_back(TAssetInstanceData{
                .WorldMatrix = transform,
                .MeshIndex = meshIndex + i
            });
        }
    }

    for (auto childNodeIndex : node.children) {
        LoadNodes(asset, assetScene, childNodeIndex, transform, meshOffsets);
    }
}

auto LoadAssetFromFile(const std::string& assetName, const std::filesystem::path& filePath) -> std::expected<TAsset, std::string> {
    if (!std::filesystem::exists(filePath)) {
        return std::unexpected(std::format("Unable to load asset. File '{}' does not exist", filePath.string()));
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

    auto& fgAsset = loadResult.get();

    TAsset asset = {};
    asset.Name = assetName;

    LoadImages(assetName, asset, fgAsset, filePath);
    LoadMaterials(assetName, asset, fgAsset);

    std::vector<std::pair<size_t, size_t>> meshOffsets;
    meshOffsets.resize(fgAsset.meshes.size());
    for(auto meshIndex = 0; meshIndex < fgAsset.meshes.size(); ++meshIndex) {
        meshOffsets[meshIndex].first = asset.Meshes.size();
        meshOffsets[meshIndex].second = fgAsset.meshes[meshIndex].primitives.size();
        for (auto primitiveIndex = 0; primitiveIndex < fgAsset.meshes[meshIndex].primitives.size(); primitiveIndex++) {
            LoadMeshes(fgAsset, asset, meshIndex, primitiveIndex);
        }
    }

    const auto nodeIndices = std::ranges::iota_view{0uz, fgAsset.scenes[fgAsset.defaultScene.value()].nodeIndices.size()};
    std::for_each(poolstl::execution::par,
                  nodeIndices.begin(),
                  nodeIndices.end(),
                  [&](size_t nodeIndex) -> void {
                      LoadNodes(fgAsset, asset, nodeIndex, glm::mat4(1.0f), meshOffsets);
                  });

    return asset;
}

auto AddAssetFromFile(const std::string& assetName, const std::filesystem::path& filePath) -> void {
    auto assetResult = LoadAssetFromFile(assetName, filePath);
    if (!assetResult) {
        spdlog::error(assetResult.error());
        return;
    }

    g_assets[assetName] = std::move(*assetResult);
}

auto GetAssets() -> std::unordered_map<std::string, TAsset>& {
    return g_assets;
}

auto GetAsset(const std::string& assetName) -> TAsset& {
    return g_assets[assetName];
}
