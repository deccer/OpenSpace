#include "Assets.hpp"
#include "Io.hpp"
#include "Images.hpp"
#include "Helpers.hpp"
#include "Profiler.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <mikktspace.h>

#include <algorithm>
#include <format>
#include <ranges>

#include <spdlog/spdlog.h>
#include <unordered_map>
#include <utility>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace Assets {

struct TAssetRawImageData {
    std::string Name;
    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;
    TAssetImageDataType ImageDataType = {};
};

std::unordered_map<std::string, TAsset> g_assets = {};
std::unordered_map<std::string, TAssetImageData> g_assetImageDates = {};
std::unordered_map<std::string, TAssetSamplerData> g_assetSamplerDates = {};
std::unordered_map<std::string, TAssetMaterialData> g_assetMaterialDates = {};
std::unordered_map<std::string, TAssetMeshData> g_assetMeshDates = {};

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

auto GetSamplerIndex(fastgltf::Asset& fgAsset, const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.samplerIndex;
}

auto GetSamplerIndex(fastgltf::Asset& fgAsset, const std::optional<fastgltf::NormalTextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.samplerIndex;
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

    std::vector<TAssetImageData> assetImages;
    assetImages.resize(fgAsset.images.size());

    const auto imageIndices = std::ranges::iota_view{0ull, fgAsset.images.size()};
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

        auto& assetImage = assetImages[imageIndex];
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

    for(auto i = 0; i < assetImages.size(); ++i) {
        auto& assetImage = assetImages[i];
        asset.Images[i] = assetImage.Name;
        g_assetImageDates[assetImage.Name] = std::move(assetImage);
    }
}

constexpr auto ConvertMagFilter(std::optional<fastgltf::Filter> magFilter) -> TAssetSamplerMagFilter {
    if (!magFilter) {
        return TAssetSamplerMagFilter::Linear;
    }

    switch (*magFilter) {
        case fastgltf::Filter::Linear: return TAssetSamplerMagFilter::Linear;
        case fastgltf::Filter::Nearest: return TAssetSamplerMagFilter::Nearest;
        default: std::unreachable();
    }
}

constexpr auto ConvertMinFilter(std::optional<fastgltf::Filter> minFilter) -> TAssetSamplerMinFilter {
    if (!minFilter) {
        return TAssetSamplerMinFilter::Linear;
    }

    switch (*minFilter) {
        case fastgltf::Filter::Linear: return TAssetSamplerMinFilter::Linear;
        case fastgltf::Filter::Nearest: return TAssetSamplerMinFilter::Nearest;
        case fastgltf::Filter::LinearMipMapNearest: return TAssetSamplerMinFilter::LinearMipMapNearest;
        case fastgltf::Filter::NearestMipMapNearest: return TAssetSamplerMinFilter::NearestMipMapNearest;
        case fastgltf::Filter::LinearMipMapLinear: return TAssetSamplerMinFilter::LinearMipMapLinear;
        case fastgltf::Filter::NearestMipMapLinear: return TAssetSamplerMinFilter::NearestMipMapLinear;
        default: std::unreachable();
    }    
}

constexpr auto ConvertWrapMode(fastgltf::Wrap wrapMode) -> TAssetSamplerWrapMode {
    switch (wrapMode) {
        case fastgltf::Wrap::ClampToEdge: return TAssetSamplerWrapMode::ClampToEdge;
        case fastgltf::Wrap::MirroredRepeat: return TAssetSamplerWrapMode::MirroredRepeat;
        case fastgltf::Wrap::Repeat: return TAssetSamplerWrapMode::Repeat;
        default: std::unreachable();
    }
}

constexpr auto ToString(std::optional<fastgltf::Filter> filter) -> std::string {
    if (!filter) {
        return "L";
    }

    switch (*filter) {
        case fastgltf::Filter::Linear: return "L";
        case fastgltf::Filter::Nearest: return "N";
        case fastgltf::Filter::LinearMipMapNearest: return "LMN";
        case fastgltf::Filter::NearestMipMapNearest: return "NMN";
        case fastgltf::Filter::LinearMipMapLinear: return "LML";
        case fastgltf::Filter::NearestMipMapLinear: return "NML";
        default: std::unreachable();
    }    
}

constexpr auto ToString(fastgltf::Wrap wrapMode) -> std::string {
    switch (wrapMode) {
        case fastgltf::Wrap::ClampToEdge: return "C2E";
        case fastgltf::Wrap::MirroredRepeat: return "MR";
        case fastgltf::Wrap::Repeat: return "R";
        default: std::unreachable();
    }
}

auto LoadSamplers(const std::string& assetName, TAsset& asset, fastgltf::Asset& fgAsset) -> void {

    const auto samplerIndices = std::ranges::iota_view{0ull, fgAsset.samplers.size()};
    asset.Samplers.resize(fgAsset.samplers.size());
    std::for_each(
        samplerIndices.begin(),
        samplerIndices.end(),
        [&](size_t samplerIndex) -> void {

        auto& fgSampler = fgAsset.samplers[samplerIndex];
        auto magFilter = ToString(fgSampler.magFilter);
        auto minFilter = ToString(fgSampler.minFilter);
        auto wrapS = ToString(fgSampler.wrapS);
        auto wrapT = ToString(fgSampler.wrapT);
        auto samplerName = std::format("S_{}_{}_{}_{}", magFilter, minFilter, wrapS, wrapT);

        auto assetSamplerData = TAssetSamplerData {
            .Name = samplerName,
            .MagFilter = ConvertMagFilter(fgSampler.magFilter),
            .MinFilter = ConvertMinFilter(fgSampler.minFilter),
            .WrapS = ConvertWrapMode(fgSampler.wrapS),
            .WrapT = ConvertWrapMode(fgSampler.wrapT),
        };
        
        asset.Samplers[samplerIndex] = samplerName;
        if (!g_assetSamplerDates.contains(samplerName)) {
            g_assetSamplerDates[samplerName] = std::move(assetSamplerData);
        }
    });
}

auto LoadMaterials(const std::string& assetName, TAsset& asset, fastgltf::Asset& fgAsset) -> void {

    const auto materialIndices = std::ranges::iota_view{0ull, fgAsset.materials.size()};
    asset.Materials.resize(fgAsset.materials.size());

    std::vector<TAssetMaterialData> assetMaterials;
    assetMaterials.resize(fgAsset.materials.size());

    std::for_each(
        poolstl::execution::seq,
        materialIndices.begin(),
        materialIndices.end(),
        [&](size_t materialIndex) -> void {

        const auto& fgMaterial = fgAsset.materials[materialIndex];
        auto& material = assetMaterials[materialIndex];

        material.Name = GetSafeResourceName(assetName.data(), fgMaterial.name.data(), "material", materialIndex);

        auto baseColorTextureIndex = GetImageIndex(fgAsset, fgMaterial.pbrData.baseColorTexture);
        auto baseColorSamplerIndex = GetSamplerIndex(fgAsset, fgMaterial.pbrData.baseColorTexture);

        material.BaseColorTextureChannel = TAssetMaterialChannelData{
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = baseColorSamplerIndex.has_value() ? asset.Samplers[baseColorSamplerIndex.value()] : "S_L_L_C2E_C2E",
            .TextureName = baseColorTextureIndex.has_value() ? asset.Images[baseColorTextureIndex.value()] : "T_Default_B",
        };

        auto normalTextureIndex = GetImageIndex(fgAsset, fgMaterial.normalTexture);
        auto normalSamplerIndex = GetSamplerIndex(fgAsset, fgMaterial.normalTexture);

        material.NormalTextureChannel = TAssetMaterialChannelData{
            .Channel = TAssetMaterialChannel::Normals,
            .SamplerName = normalSamplerIndex.has_value() ? asset.Samplers[normalSamplerIndex.value()] : "S_L_L_C2E_C2E",
            .TextureName = normalTextureIndex.has_value() ? asset.Images[normalTextureIndex.value()] : "T_Default_N",
        };

        if (fgMaterial.packedOcclusionRoughnessMetallicTextures != nullptr) {
            auto armTextureIndex = GetImageIndex(fgAsset, fgMaterial.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture);
            auto armSamplerIndex = GetSamplerIndex(fgAsset, fgMaterial.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture);
            material.ArmTextureChannel = TAssetMaterialChannelData{
                .Channel = TAssetMaterialChannel::Scalar,
                .SamplerName = armSamplerIndex.has_value() ? asset.Samplers[armSamplerIndex.value()] : "S_L_L_C2E_C2E",
                .TextureName = armTextureIndex.has_value() ? asset.Images[armTextureIndex.value()] : "T_Default_MR",
            };
        }

        auto emissiveTextureIndex = GetImageIndex(fgAsset, fgMaterial.emissiveTexture);
        auto emissiveSamplerIndex = GetSamplerIndex(fgAsset, fgMaterial.emissiveTexture);

        material.EmissiveTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = emissiveSamplerIndex.has_value() ? asset.Samplers[emissiveSamplerIndex.value()] : "S_L_L_C2E_C2E",
            .TextureName = emissiveTextureIndex.has_value() ? asset.Images[emissiveTextureIndex.value()] : "T_Default_S"
        };

        material.BaseColor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
        material.Metalness = fgMaterial.pbrData.metallicFactor;
        material.Roughness = fgMaterial.pbrData.roughnessFactor;
    });

    for (auto i = 0; i < assetMaterials.size(); ++i) {
        auto& assetMaterial = assetMaterials[i];
        asset.Materials[i] = assetMaterial.Name;
        g_assetMaterialDates[assetMaterial.Name] = std::move(assetMaterial);
    }
}

auto LoadMesh(
    std::string_view assetName,
    const fastgltf::Asset& fgAsset,
    TAsset& asset,
    size_t meshIndex,
    size_t primitiveIndex,
    size_t assetGlobalPrimitiveIndex,
    const glm::mat4x4& initialTransform) -> void {

    auto& primitive = fgAsset.meshes[meshIndex].primitives[primitiveIndex];
    if (!primitive.indicesAccessor.has_value() || primitive.findAttribute("POSITION") == primitive.attributes.end()) {
        return;
    }

    auto primitiveName = GetSafeResourceName(assetName.data(), fgAsset.meshes[meshIndex].name.data(), std::format("mesh-{}-primitive", meshIndex).data(), primitiveIndex);
    
    TAssetMeshData assetMeshData;
    assetMeshData.Name = primitiveName;
    assetMeshData.MaterialIndex = primitive.materialIndex;
    assetMeshData.InitialTransform = initialTransform;

    auto& indices = fgAsset.accessors[primitive.indicesAccessor.value()];
    assetMeshData.Indices.resize(indices.count);
    fastgltf::copyFromAccessor<uint32_t>(fgAsset, indices, assetMeshData.Indices.data());

    auto& positions = fgAsset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
    assetMeshData.Positions.resize(positions.count);
    fastgltf::copyFromAccessor<glm::vec3>(fgAsset, positions, assetMeshData.Positions.data());

    if (auto* normalsAttribute = primitive.findAttribute("NORMAL"); normalsAttribute != primitive.attributes.end()) {
        auto& normals = fgAsset.accessors[normalsAttribute->accessorIndex];
        assetMeshData.Normals.resize(normals.count);
        fastgltf::copyFromAccessor<glm::vec3>(fgAsset, normals, assetMeshData.Normals.data());
    }

    if (auto* uv0Attribute = primitive.findAttribute("TEXCOORD_0"); uv0Attribute != primitive.attributes.end()) {
        auto& uv0s = fgAsset.accessors[uv0Attribute->accessorIndex];
        assetMeshData.Uvs.resize(uv0s.count);
        fastgltf::copyFromAccessor<glm::vec2>(fgAsset, uv0s, assetMeshData.Uvs.data());
    }

    if (auto* tangentAttribute = primitive.findAttribute("TANGENT"); tangentAttribute != primitive.attributes.end()) {
        auto& tangents = fgAsset.accessors[tangentAttribute->accessorIndex];
        assetMeshData.Tangents.resize(tangents.count);
        fastgltf::copyFromAccessor<glm::vec4>(fgAsset, tangents, assetMeshData.Tangents.data());
    } else  {
        assetMeshData.Tangents.resize(assetMeshData.Positions.size());
        std::fill_n(assetMeshData.Tangents.begin(), assetMeshData.Positions.size(), glm::vec4{1.0f});
    }

    asset.Meshes[assetGlobalPrimitiveIndex++] = primitiveName;
    g_assetMeshDates[primitiveName] = std::move(assetMeshData);
}

auto LoadNode(
    const fastgltf::Asset& asset,
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
        assetScene.Instances.emplace_back(TAssetInstanceData{
            .WorldMatrix = transform,
            .MeshIndex = node.meshIndex.value()
        });
        /*
        auto [meshIndex, meshCount] = meshOffsets[node.meshIndex.value()];
        for (auto i = 0; i < meshCount; i++) {
            assetScene.Instances.emplace_back(TAssetInstanceData{
                .WorldMatrix = transform,
                .MeshIndex = meshIndex + i
            });
        }
        */
    }

    for (auto childNodeIndex : node.children) {
        LoadNode(asset, assetScene, childNodeIndex, transform, meshOffsets);
    }
}

auto LoadAssetFromFile(
    const std::string& assetName,
    const std::filesystem::path& filePath) -> std::expected<TAsset, std::string> {

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

    LoadImages(assetName, asset, fgAsset, filePath);
    LoadSamplers(assetName, asset, fgAsset);
    LoadMaterials(assetName, asset, fgAsset);

    std::vector<std::pair<size_t, size_t>> meshOffsets;
    meshOffsets.resize(fgAsset.meshes.size());

    // determine how many meshes we will end up with
    auto primitiveCount = 0;
    for(auto meshIndex = 0; meshIndex < fgAsset.meshes.size(); meshIndex++) {
        primitiveCount += fgAsset.meshes[meshIndex].primitives.size();
    }

    asset.Meshes.resize(primitiveCount);

    auto assetGlobalPrimitiveIndex = 0u;
    for(auto meshIndex = 0; meshIndex < fgAsset.meshes.size(); meshIndex++) {
        meshOffsets[meshIndex].first = asset.Meshes.size();
        meshOffsets[meshIndex].second = fgAsset.meshes[meshIndex].primitives.size();
        for (auto primitiveIndex = 0; primitiveIndex < fgAsset.meshes[meshIndex].primitives.size(); primitiveIndex++) {
            LoadMesh(assetName, fgAsset, asset, meshIndex, primitiveIndex, assetGlobalPrimitiveIndex++, glm::mat4(1.0f));
        }
    }

    const auto& defaultScene = fgAsset.scenes[fgAsset.defaultScene.value()];
    std::for_each(poolstl::execution::par,
                  defaultScene.nodeIndices.begin(),
                  defaultScene.nodeIndices.end(),
                  [&](size_t nodeIndex) -> void {
                      LoadNode(fgAsset, asset, nodeIndex, glm::mat4(1.0f), meshOffsets);
                  });

    return asset;
}

auto AddAssetFromFile(
    const std::string& assetName,
    const std::filesystem::path& filePath) -> void {

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

auto IsAssetLoaded(const std::string& assetName) -> bool {
    return g_assets.contains(assetName);
}

auto GetAssetImageData(const std::string& imageDataName) -> TAssetImageData& {
    return g_assetImageDates[imageDataName];
}

auto GetAssetSamplerData(const std::string& samplerDataName) -> TAssetSamplerData& {
    return g_assetSamplerDates[samplerDataName];
}

auto GetAssetMaterialData(const std::string& materialDataName) -> TAssetMaterialData& {
    return g_assetMaterialDates[materialDataName];
}

auto GetAssetMeshData(const std::string& meshDataName) -> TAssetMeshData& {
    return g_assetMeshDates[meshDataName];
}

auto AddImage(
    std::string imageName,
    const std::filesystem::path& filePath) -> void {

    int32_t width = 0;
    int32_t height = 0;
    int32_t components = 0;

    auto [fileData, fileDataSize] = ReadBinaryFromFile(filePath);

    auto dataCopy = std::make_unique<std::byte[]>(fileDataSize);
    std::copy_n(static_cast<const std::byte*>(fileData.get()), fileDataSize, dataCopy.get());

    auto* pixels = LoadImageFromMemory(dataCopy.get(), fileDataSize, &width, &height, &components);

    auto assetImage = TAssetImageData {
        .Width = width,
        .Height = height,
        .PixelType = 0,
        .Bits = 8,
        .Components = components,
        .Name = std::string(imageName),
    };
    assetImage.Data.reset(pixels);

    g_assetImageDates[imageName] = std::move(assetImage);
}

auto CalculateTangents(TAssetMeshData& assetMeshData) -> void {

    auto getNumFaces = [](const SMikkTSpaceContext* context) -> int32_t {
        auto* meshData = static_cast<TAssetMeshData*>(context->m_pUserData);
        return meshData->Indices.size() / 3;
    };

    auto getNumVerticesOfFace = [](const SMikkTSpaceContext* context, const int32_t iFace) -> int32_t {
        return 3;
    };

    auto getPosition = [](const SMikkTSpaceContext* context, float posOut[], const int32_t faceIndex, const int32_t vertIndex) -> void {
        auto* meshData = static_cast<TAssetMeshData*>(context->m_pUserData);

        auto index = meshData->Indices[faceIndex * 3 + vertIndex];
        const glm::vec3& pos = meshData->Positions[index];
        posOut[0] = pos.x;
        posOut[1] = pos.y;
        posOut[2] = pos.z;
    };

    auto getNormal = [](const SMikkTSpaceContext* context, float normOut[], const int32_t faceIndex, const int32_t vertIndex) -> void {
        auto* meshData = static_cast<TAssetMeshData*>(context->m_pUserData);

        auto index = meshData->Indices[faceIndex * 3 + vertIndex];
        const glm::vec3& normal = meshData->Normals[index];
        normOut[0] = normal.x;
        normOut[1] = normal.y;
        normOut[2] = normal.z;
    };

    auto getUv = [](const SMikkTSpaceContext* context, float uvOut[], const int32_t faceIndex, const int32_t vertIndex) -> void {
        auto* meshData = static_cast<TAssetMeshData*>(context->m_pUserData);

        auto index = meshData->Indices[faceIndex * 3 + vertIndex];
        const glm::vec2& uv = meshData->Uvs[index];
        uvOut[0] = uv.x;
        uvOut[1] = uv.y;
    };

    auto setTSpaceBasic = [](const SMikkTSpaceContext* context, const float tangent[], const float sign, const int32_t faceIndex, const int32_t vertIndex) {
        auto* meshData = static_cast<TAssetMeshData*>(context->m_pUserData);
        auto index = meshData->Indices[faceIndex * 3 + vertIndex];

        glm::vec3 tan(tangent[0], tangent[1], tangent[2]);
        meshData->Tangents[index] = glm::vec4(glm::normalize(tan), sign);
    };

    SMikkTSpaceInterface interface = {
        .m_getNumFaces = getNumFaces,
        .m_getNumVerticesOfFace = getNumVerticesOfFace,
        .m_getPosition = getPosition,
        .m_getNormal = getNormal,
        .m_getTexCoord = getUv,
        .m_setTSpaceBasic = setTSpaceBasic,
    };

    SMikkTSpaceContext context = {
        .m_pInterface = &interface,
        .m_pUserData = &assetMeshData
    };

    genTangSpaceDefault(&context);
}

auto CreateUvSphereMeshData(
    std::string name,
    float radius,
    int32_t rings,
    int32_t segments) -> TAssetMeshData {

    TAssetMeshData assetMeshData;

    assetMeshData.Name = name;
    assetMeshData.Positions.resize(rings * segments * 4);
    assetMeshData.Normals.resize(rings * segments * 4);
    assetMeshData.Uvs.resize(rings * segments * 4);
    assetMeshData.Tangents.resize(rings * segments * 4);

    auto index = 0;
    const auto pi = glm::pi<float>();
    for (auto ring = 0; ring <= rings; ++ring) {
        auto theta = ring * pi / rings;
        auto sinTheta = glm::sin(theta);
        auto cosTheta = glm::cos(theta);

        for (unsigned int segment = 0; segment <= segments; ++segment) {
            float phi = segment * 2.0f * pi / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            glm::vec3 position;
            position.x = radius * cosPhi * sinTheta;
            position.y = radius * cosTheta;
            position.z = radius * sinPhi * sinTheta;

            glm::vec3 normal = {
                cosPhi * sinTheta,
                cosTheta,
                sinPhi * sinTheta
            };

            glm::vec2 uv = {
                static_cast<float>(segment) / segments,
                static_cast<float>(ring) / rings,
            };

            assetMeshData.Positions[index] = position;
            assetMeshData.Normals[index] = normal;
            assetMeshData.Uvs[index] = uv;
            assetMeshData.Tangents[index] = glm::vec4{0.0f};
            index++;
        }
    }

    assetMeshData.Indices.resize(rings * segments * 6);

    index = 0;
    for (auto ring = 0; ring < rings; ++ring) {
        for (auto seg = 0; seg < segments; ++seg) {
            auto first = (ring * (segments + 1)) + seg;
            auto second = first + segments + 1;

            // Two triangles per quad
            assetMeshData.Indices[index + 0] = first;
            assetMeshData.Indices[index + 1] = first + 1;
            assetMeshData.Indices[index + 2] = second;

            assetMeshData.Indices[index + 3] = second;
            assetMeshData.Indices[index + 4] = first + 1;
            assetMeshData.Indices[index + 5] = second + 1;

            index += 6;
        }
    }

    assetMeshData.InitialTransform = glm::mat4(1.0f);
    assetMeshData.MaterialIndex = 0;

    CalculateTangents(assetMeshData);

    return assetMeshData;
}

auto CreateCuboid(
    const std::string& name,
    float width,
    float height,
    float depth,
    uint32_t segmentsX = 1,
    uint32_t segmentsY = 1,
    uint32_t resZ = 1) -> TAssetMeshData {

    auto halfW = width / 2.0f;
    auto halfH = height / 2.0f;
    auto halfD = depth / 2.0f;

    TAssetMeshData assetMeshData;
    assetMeshData.Name = name;
    assetMeshData.MaterialIndex = 0;
    assetMeshData.InitialTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, halfH, 0));

    std::size_t vertOffset = 0;

    // Front Face //
    glm::vec3 normal = {0.f, 0.f, 1.f};
    for(int iy = 0; iy < segmentsY; iy++) {
        for(int ix = 0; ix < segmentsX; ix++) {

            glm::vec2 uv;
            uv.x = ((float)ix/((float)segmentsX-1.f));
            uv.y = 1.f - ((float)iy/((float)segmentsY-1.f));

            glm::vec3 position;
            position.x = uv.x * width - halfW;
            position.y = -(uv.y-1.f) * height - halfH;
            position.z = halfD;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4(0.0f));
        }
    }

    for(int y = 0; y < segmentsY-1; y++) {
        for(int x = 0; x < segmentsX-1; x++) {
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x + vertOffset);            

            assetMeshData.Indices.push_back((y+1)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Right Side Face //
    normal = {1.f, 0.f, 0.f};
    for(int iy = 0; iy < segmentsY; iy++) {
        for(int ix = 0; ix < resZ; ix++) {

            glm::vec2 texcoord;
            texcoord.x = ((float)ix/((float)resZ-1.f));
            texcoord.y = 1.f - ((float)iy/((float)segmentsY-1.f));

            glm::vec3 vert;
            vert.x = halfW;
            vert.y = -(texcoord.y-1.f) * height - halfH;
            vert.z = texcoord.x * -depth + halfD;

            assetMeshData.Positions.push_back(vert);
            assetMeshData.Uvs.push_back(texcoord);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for(int y = 0; y < segmentsY-1; y++) {
        for(int x = 0; x < resZ-1; x++) {
            assetMeshData.Indices.push_back((y)*resZ + x+1 + vertOffset);            
            assetMeshData.Indices.push_back((y+1)*resZ + x + vertOffset);
            assetMeshData.Indices.push_back((y)*resZ + x + vertOffset);            

            assetMeshData.Indices.push_back((y+1)*resZ + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*resZ + x + vertOffset);
            assetMeshData.Indices.push_back((y)*resZ + x+1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Left Side Face //
    normal = {-1.f, 0.f, 0.f};
    for(int iy = 0; iy < segmentsY; iy++) {
        for(int ix = 0; ix < resZ; ix++) {

            glm::vec2 texcoord;
            texcoord.x = ((float)ix/((float)resZ-1.f));
            texcoord.y = 1.f-((float)iy/((float)segmentsY-1.f));

            glm::vec3 vert;
            vert.x = -halfW;
            vert.y = -(texcoord.y-1.f) * height - halfH;
            vert.z = texcoord.x * depth - halfD;

            assetMeshData.Positions.push_back(vert);
            assetMeshData.Uvs.push_back(texcoord);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for(int y = 0; y < segmentsY-1; y++) {
        for(int x = 0; x < resZ-1; x++) {
            assetMeshData.Indices.push_back((y)*resZ + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*resZ + x + vertOffset);
            assetMeshData.Indices.push_back((y)*resZ + x + vertOffset);            

            assetMeshData.Indices.push_back((y+1)*resZ + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*resZ + x + vertOffset);
            assetMeshData.Indices.push_back((y)*resZ + x+1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();


    // Back Face //
    normal = {0.f, 0.f, -1.f};
    for(int iy = 0; iy < segmentsY; iy++) {
        for(int ix = 0; ix < segmentsX; ix++) {

            glm::vec2 texcoord;
            texcoord.x = ((float)ix/((float)segmentsX-1.f));
            texcoord.y = 1.f-((float)iy/((float)segmentsY-1.f));

            glm::vec3 vert;
            vert.x = texcoord.x * -width + halfW;
            vert.y = -(texcoord.y-1.f) * height - halfH;
            vert.z = -halfD;

            assetMeshData.Positions.push_back(vert);
            assetMeshData.Uvs.push_back(texcoord);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for(int y = 0; y < segmentsY-1; y++) {
        for(int x = 0; x < segmentsX-1; x++) {
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x + vertOffset);            

            assetMeshData.Indices.push_back((y+1)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Top Face //
    normal = {0.f, -1.f, 0.f};
    for(int iy = 0; iy < resZ; iy++) {
        for(int ix = 0; ix < segmentsX; ix++) {

            glm::vec2 texcoord;
            texcoord.x = ((float)ix/((float)segmentsX-1.f));
            texcoord.y = 1.f-((float)iy/((float)resZ-1.f));

            glm::vec3 vert;
            vert.x = texcoord.x * width - halfW;
            vert.y = -halfH;
            vert.z = texcoord.y * depth - halfD;

            assetMeshData.Positions.push_back(vert);
            assetMeshData.Uvs.push_back(texcoord);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for(int y = 0; y < resZ-1; y++) {
        for(int x = 0; x < segmentsX-1; x++) {
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);            
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x + vertOffset);

            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y+1)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Bottom Face //
    normal = {0.f, 1.f, 0.f};
    for(int iy = 0; iy < resZ; iy++) {
        for(int ix = 0; ix < segmentsX; ix++) {

            glm::vec2 texcoord;
            texcoord.x = ((float)ix/((float)segmentsX-1.f));
            texcoord.y = 1.f-((float)iy/((float)resZ-1.f));

            glm::vec3 vert;
            vert.x = texcoord.x * width - halfW;
            vert.y = halfH;
            vert.z = texcoord.y * -depth + halfD;

            assetMeshData.Positions.push_back(vert);
            assetMeshData.Uvs.push_back(texcoord);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for(int y = 0; y < resZ-1; y++) {
        for(int x = 0; x < segmentsX-1; x++) {
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x + vertOffset);
            
            assetMeshData.Indices.push_back((y+1)*segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y+1)*segmentsX + x+1 + vertOffset);
            assetMeshData.Indices.push_back((y)*segmentsX + x+1 + vertOffset);
            
        }
    }
   
    return std::move(assetMeshData);
}

auto CreateCuboid(
    const std::string& name,
    float width,
    float height,
    float depth,
    uint32_t segmentsX,
    uint32_t segmentsY,
    uint32_t segmentsZ,
    const std::string& materialName) -> void {

    TAsset cuboid = {};
    cuboid.Materials.push_back(materialName);
    cuboid.Meshes.push_back(name);
    cuboid.Instances.push_back(TAssetInstanceData{
        .WorldMatrix = glm::mat4(1.0f),
        .MeshIndex = 0,
    });
   
    g_assets[name] = std::move(cuboid);
    g_assetMeshDates[name] = CreateCuboid(name, width, height, depth, segmentsX, segmentsY, segmentsZ);
}

auto AddAsset(const std::string& assetName, const TAsset& asset) -> void {
    if (!g_assets.contains(assetName)) {
        g_assets[assetName] = std::move(asset);
    }
}

auto AddDefaultAssets() -> void {

    AddImage("T_Default_B", "data/default/T_Default_B.png");
    AddImage("T_Default_N", "data/default/T_Default_N.png");
    AddImage("T_Default_S", "data/default/T_Default_S.png");
    AddImage("T_Default_MR", "data/default/T_Default_MR.png");

    AddImage("T_Purple", "data/default/T_Purple.png");
    AddImage("T_Orange", "data/default/T_Orange.png");

    auto defaultAssetSampler = TAssetSamplerData {
        .Name = "S_L_L_C2E_C2E",
        .MagFilter = TAssetSamplerMagFilter::Linear,
        .MinFilter = TAssetSamplerMinFilter::Linear,
        .WrapS = TAssetSamplerWrapMode::ClampToEdge,
        .WrapT = TAssetSamplerWrapMode::ClampToEdge,
    };
    g_assetSamplerDates["S_L_L_C2E_C2E"] = std::move(defaultAssetSampler);

    auto defaultMaterial = TAssetMaterialData {
        .Name = "M_Default",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Purple"
        },
    };
    g_assetMaterialDates["M_Default"] = std::move(defaultMaterial);

    auto orangeMaterial = TAssetMaterialData {
        .Name = "M_Orange",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Orange"
        },
    };
    g_assetMaterialDates["M_Orange"] = std::move(orangeMaterial);

    AddImage("T_Mars_B", "data/default/2k_mars.jpg");
    auto marsMaterial = TAssetMaterialData {
        .Name = "M_Mars",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Mars_B"
        }
    };
    g_assetMaterialDates["M_Mars"] = std::move(marsMaterial);

    auto geodesic = CreateUvSphereMeshData("SM_Geodesic", 1, 64, 64);
    g_assetMeshDates["SM_Geodesic"] = std::move(geodesic);

    for (auto i = 1; i < 11; i++) {
        CreateCuboid(std::format("SM_Cuboid_x{}_y1_z1", i), i, 1.0f, 1.0f, i + 1, i + 1, i + 1, "M_Orange");
        CreateCuboid(std::format("SM_Cuboid_x1_y{}_z1", i), 1.0f, i, 1.0f, i + 1, i + 1, i + 1, "M_Orange");
        CreateCuboid(std::format("SM_Cuboid_x1_y1_z{}", i), 1.0f, 1.0f, i, i + 1, i + 1, i + 1, "M_Orange");
    }

    CreateCuboid("SM_Cuboid_x50_y1_z50", 50.0f, 1.0f, 50.0f, 4, 4, 4, "M_Orange");
}

}
