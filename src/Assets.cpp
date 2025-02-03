#include "Core.hpp"
#include "Assets.hpp"
#include "Io.hpp"
#include "Images.hpp"
#include "Profiler.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include <spdlog/spdlog.h>
#include <mikktspace.h>

#include <algorithm>
#include <format>
#include <ranges>
#include <unordered_map>
#include <utility>

namespace Assets {

struct TAssetRawImageData {
    std::string Name;
    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;
    TAssetImageType ImageDataType = {};
};

std::unordered_map<std::string, TAssetModel> g_assetModels = {};
std::unordered_map<std::string, TAssetImage> g_assetImageDates = {};
std::unordered_map<std::string, TAssetSampler> g_assetSamplerDates = {};
std::unordered_map<std::string, TAssetMaterial> g_assetMaterialDates = {};
std::unordered_map<std::string, TAssetMesh> g_assetMeshDates = {};

auto GetSafeResourceName(
    const char* const baseName,
    const char* const text,
    const char* const resourceType,
    const std::size_t resourceIndex) -> std::string {

    return (text == nullptr) || strlen(text) == 0
           ? std::format("{}-{}-{}", baseName, resourceType, resourceIndex)
           : std::format("{}-{}", baseName, text);
}

auto GetImageIndex(const fastgltf::Asset& fgAsset, const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
}

auto GetImageIndex(const fastgltf::Asset& fgAsset, const std::optional<fastgltf::NormalTextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
}

auto GetSamplerIndex(const fastgltf::Asset& fgAsset, const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.samplerIndex;
}

auto GetSamplerIndex(const fastgltf::Asset& fgAsset, const std::optional<fastgltf::NormalTextureInfo>& textureInfo) -> std::optional<size_t> {
    if (!textureInfo.has_value()) {
        return std::nullopt;
    }

    const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
    return fgTexture.samplerIndex;
}

auto MimeTypeToImageDataType(fastgltf::MimeType mimeType) -> TAssetImageType {
    if (mimeType == fastgltf::MimeType::KTX2) {
        return TAssetImageType::CompressedKtx;
    }

    if (mimeType == fastgltf::MimeType::DDS) {
        return TAssetImageType::CompressedDds;
    }

    return TAssetImageType::Uncompressed;
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

auto LoadImages(
    std::string_view assetModelName,
    TAssetModel& assetModel,
    const fastgltf::Asset& fgAsset,
    const std::filesystem::path& filePath) -> void {

    assetModel.Images.resize(fgAsset.images.size());

    std::vector<TAssetImage> assetImages;
    assetImages.resize(fgAsset.images.size());

    const auto imageIndices = std::ranges::iota_view{IndexZero, fgAsset.images.size()};
    std::for_each(
        poolstl::execution::par,
        imageIndices.begin(),
        imageIndices.end(),
        [&](size_t imageIndex) -> void {

        const auto& fgImage = fgAsset.images[imageIndex];

        const auto imageData = [&]{

            auto imageName = GetSafeResourceName(assetModelName.data(), fgImage.name.data(), "image", imageIndex);
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
        if (assetImage.ImageDataType == TAssetImageType::CompressedKtx) {

        } else if (assetImage.ImageDataType == TAssetImageType::CompressedDds) {

        } else {
            int32_t width = 0;
            int32_t height = 0;
            int32_t components = 0;
            auto* pixels = Image::LoadImageFromMemory(imageData.EncodedData.get(), imageData.EncodedDataSize, &width, &height, &components);

            assetImage.Name = imageData.Name;
            assetImage.Width = width;
            assetImage.Height = height;
            assetImage.Components = components;
            assetImage.Data.reset(pixels);
        }
    });

    for(auto i = 0; i < assetImages.size(); ++i) {
        auto& assetImage = assetImages[i];
        assetModel.Images[i] = assetImage.Name;
        g_assetImageDates[assetImage.Name] = std::move(assetImage);
    }
}

auto LoadSamplers(
    TAssetModel& asset,
    const fastgltf::Asset& fgAsset) -> void {

    const auto samplerIndices = std::ranges::iota_view{IndexZero, fgAsset.samplers.size()};
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

        auto assetSamplerData = TAssetSampler {
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

auto LoadMaterials(
    std::string_view assetModelName,
    TAssetModel& asset,
    const fastgltf::Asset& fgAsset) -> void {

    const auto materialIndices = std::ranges::iota_view{IndexZero, fgAsset.materials.size()};
    asset.Materials.resize(fgAsset.materials.size());

    std::vector<TAssetMaterial> assetMaterials;
    assetMaterials.resize(fgAsset.materials.size());

    std::for_each(
        poolstl::execution::par,
        materialIndices.begin(),
        materialIndices.end(),
        [&](size_t materialIndex) -> void {

        const auto& fgMaterial = fgAsset.materials[materialIndex];
        auto& material = assetMaterials[materialIndex];

        material.Name = GetSafeResourceName(assetModelName.data(), fgMaterial.name.data(), "material", materialIndex);

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

auto LoadMeshes(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    /*
    auto& primitive = fgAsset.meshes[meshIndex].primitives[primitiveIndex];
    if (!primitive.indicesAccessor.has_value() || primitive.findAttribute("POSITION") == primitive.attributes.end()) {
        return;
    }

    auto primitiveName = GetSafeResourceName(assetModelName.data(), fgAsset.meshes[meshIndex].name.data(), std::format("mesh-{}-primitive", meshIndex).data(), primitiveIndex);
    
    TAssetMeshData assetMeshData;
    assetMeshData.Name = primitiveName;
    assetMeshData.MaterialName = primitive.materialIndex.has_value()
        ? assetModel.Materials[primitive.materialIndex.value()]
        : "default-material-0";

    auto& indices = fgAsset.accessors[primitive.indicesAccessor.value()];
    assetMeshData.Indices.resize(indices.count);
    fastgltf::copyFromAccessor<uint32_t>(fgAsset, indices, assetMeshData.Indices.data());

    const auto& positions = fgAsset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
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
    } else {
        assetMeshData.Uvs.resize(positions.count);
    }

    if (auto* tangentAttribute = primitive.findAttribute("TANGENT"); tangentAttribute != primitive.attributes.end()) {
        auto& tangents = fgAsset.accessors[tangentAttribute->accessorIndex];
        assetMeshData.Tangents.resize(tangents.count);
        fastgltf::copyFromAccessor<glm::vec4>(fgAsset, tangents, assetMeshData.Tangents.data());
    } else  {
        assetMeshData.Tangents.resize(assetMeshData.Positions.size());
        std::fill_n(assetMeshData.Tangents.begin(), assetMeshData.Positions.size(), glm::vec4{1.0f});
    }

    assetModel.Meshes[assetGlobalPrimitiveIndex++] = primitiveName;
    g_assetMeshDates[primitiveName] = std::move(assetMeshData);
    */

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
        } else {
            mesh.Normals.resize(mesh.Positions.size());
            std::fill_n(mesh.Normals.data(), mesh.Positions.size(), glm::vec3(0.5f, 0.5f, 1.0f));
        }

        if (auto* uv0Attribute = fgPrimitive.findAttribute("TEXCOORD_0"); uv0Attribute != fgPrimitive.attributes.end()) {
            auto& uv0s = fgAsset.accessors[uv0Attribute->accessorIndex];
            mesh.Uvs.resize(uv0s.count);
            fastgltf::copyFromAccessor<glm::vec2>(fgAsset, uv0s, mesh.Uvs.data());
        } else {
            mesh.Uvs.resize(mesh.Positions.size());
            std::fill_n(mesh.Uvs.begin(), mesh.Positions.size(), glm::vec2(0.0f, 0.0f));
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

    for (const auto& mesh : meshes) {
        g_assetMeshDates[mesh.Name] = mesh;
    }
}

auto LoadNodes(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    using TParseNodeDelegate = std::function<void(
        std::string_view,
        uint32_t,
        const fastgltf::Node&,
        const std::vector<std::string>&,
        const std::optional<std::vector<fastgltf::Node>>&,
        TAssetModelNode&)>;

    TParseNodeDelegate parseNode = [&](
        std::string_view baseName,
        uint32_t nodeId,
        const fastgltf::Node& fgNode,
        const std::vector<std::string>& meshNames,
        const std::optional<std::vector<fastgltf::Node>>& allNodes,
        TAssetModelNode& assetNode) -> void {

        assetNode.Name = GetSafeResourceName(baseName.data(), fgNode.name.data(), "node", nodeId++);

        const auto& [translation, rotation, scale] = std::get<fastgltf::TRS>(fgNode.transform);
        assetNode.LocalPosition = glm::make_vec3(translation.data());;
        assetNode.LocalRotation = glm::quat{rotation[3], rotation[0], rotation[1], rotation[2]};
        assetNode.LocalScale = glm::make_vec3(scale.data());

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
                    parseNode(baseName, nodeId, childNode, meshNames, allNodes, childAssetNode);
                    assetNode.Children.push_back(std::move(childAssetNode));
                }
            }
        }
    };

    uint32_t nodeId = 0;

    for (const auto& nodeIndex : fgAsset.scenes[0].nodeIndices) {

        const auto& fgNode = fgAsset.nodes[nodeIndex];

        TAssetModelNode rootNode;
        parseNode(assetModel.Name, nodeId, fgNode, assetModel.Meshes, fgAsset.nodes, rootNode);
        assetModel.Hierarchy.push_back(std::move(rootNode));
    }
}

auto LoadAssetModelFromFile(
    const std::string& assetModelName,
    const std::filesystem::path& filePath) -> std::expected<TAssetModel, std::string> {

    if (!std::filesystem::exists(filePath)) {
        return std::unexpected(std::format("Unable to load asset '{}'. File '{}' does not exist", assetModelName, filePath.string()));
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
        fastgltf::Extensions::KHR_materials_pbrSpecularGlossiness |
        fastgltf::Extensions::KHR_materials_specular |
        fastgltf::Extensions::KHR_materials_ior |
        fastgltf::Extensions::KHR_materials_iridescence |
        fastgltf::Extensions::KHR_materials_volume |
        fastgltf::Extensions::KHR_materials_transmission |
        fastgltf::Extensions::KHR_materials_clearcoat |
        fastgltf::Extensions::KHR_materials_emissive_strength |
        fastgltf::Extensions::KHR_materials_sheen |
        fastgltf::Extensions::KHR_draco_mesh_compression |
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

    auto& fgAsset = loadResult.get();

    TAssetModel assetModel = {};
    assetModel.Name = assetModelName;

    LoadImages(assetModelName, assetModel, fgAsset, filePath);
    LoadSamplers(assetModel, fgAsset);
    LoadMaterials(assetModelName, assetModel, fgAsset);

    /*
    std::vector<std::pair<size_t, size_t>> meshOffsets;
    meshOffsets.resize(fgAsset.meshes.size());

    // determine how many meshes we will end up with
    auto primitiveCount = 0;
    for(auto meshIndex = 0; meshIndex < fgAsset.meshes.size(); meshIndex++) {
        primitiveCount += fgAsset.meshes[meshIndex].primitives.size();
    }
    assetModel.Meshes.resize(primitiveCount);
    */

    LoadMeshes(fgAsset, assetModel);

    /*
    auto assetGlobalPrimitiveIndex = 0u;
    for(auto meshIndex = 0; meshIndex < fgAsset.meshes.size(); meshIndex++) {
        meshOffsets[meshIndex].first = assetModel.Meshes.size();
        meshOffsets[meshIndex].second = fgAsset.meshes[meshIndex].primitives.size();
        for (auto primitiveIndex = 0; primitiveIndex < fgAsset.meshes[meshIndex].primitives.size(); primitiveIndex++) {
            LoadMesh(assetModelName, fgAsset, assetModel, meshIndex, primitiveIndex, assetGlobalPrimitiveIndex++);
        }
    }
    */

    LoadNodes(fgAsset, assetModel);

    return assetModel;
}

auto AddAssetModelFromFile(
    const std::string& assetName,
    const std::filesystem::path& filePath) -> void {

    auto assetResult = LoadAssetModelFromFile(assetName, filePath);
    if (!assetResult) {
        spdlog::error(assetResult.error());
        return;
    }

    g_assetModels[assetName] = std::move(*assetResult);
}

auto GetAssetModels() -> std::unordered_map<std::string, TAssetModel>& {
    return g_assetModels;
}

auto GetAssetModel(const std::string& assetName) -> TAssetModel& {
    return g_assetModels[assetName];
}

auto IsAssetLoaded(const std::string& assetName) -> bool {
    return g_assetModels.contains(assetName);
}

auto GetAssetImageData(const std::string& imageDataName) -> TAssetImage& {
    return g_assetImageDates[imageDataName];
}

auto GetAssetSamplerData(const std::string& samplerDataName) -> TAssetSampler& {
    return g_assetSamplerDates[samplerDataName];
}

auto GetAssetMaterialData(const std::string& materialDataName) -> TAssetMaterial& {
    return g_assetMaterialDates[materialDataName];
}

auto GetAssetMeshData(const std::string& meshDataName) -> TAssetMesh& {
    return g_assetMeshDates[meshDataName];
}

auto AddImage(
    const std::string& imageName,
    const std::filesystem::path& filePath) -> void {

    int32_t width = 0;
    int32_t height = 0;
    int32_t components = 0;

    auto [fileData, fileDataSize] = ReadBinaryFromFile(filePath);

    auto dataCopy = std::make_unique<std::byte[]>(fileDataSize);
    std::copy_n(static_cast<const std::byte*>(fileData.get()), fileDataSize, dataCopy.get());

    auto* pixels = Image::LoadImageFromMemory(dataCopy.get(), fileDataSize, &width, &height, &components);

    auto assetImage = TAssetImage{
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

auto CalculateTangents(TAssetMesh& assetMeshData) -> void {

    auto getNumFaces = [](const SMikkTSpaceContext* context) -> int32_t {

        auto* meshData = static_cast<TAssetMesh*>(context->m_pUserData);
        return meshData->Indices.size() / 3;
    };

    auto getNumVerticesOfFace = [](
        const SMikkTSpaceContext* context,
        const int32_t iFace) -> int32_t {

        return 3;
    };

    auto getPosition = [](
        const SMikkTSpaceContext* context,
        float posOut[],
        const int32_t faceIndex,
        const int32_t vertIndex) -> void {

        auto* meshData = static_cast<TAssetMesh*>(context->m_pUserData);
        auto index = meshData->Indices[faceIndex * 3 + vertIndex];
        const glm::vec3& pos = meshData->Positions[index];
        posOut[0] = pos.x;
        posOut[1] = pos.y;
        posOut[2] = pos.z;
    };

    auto getNormal = [](
        const SMikkTSpaceContext* context,
        float normOut[],
        const int32_t faceIndex,
        const int32_t vertIndex) -> void {

        auto* meshData = static_cast<TAssetMesh*>(context->m_pUserData);
        auto index = meshData->Indices[faceIndex * 3 + vertIndex];
        const glm::vec3& normal = meshData->Normals[index];
        normOut[0] = normal.x;
        normOut[1] = normal.y;
        normOut[2] = normal.z;
    };

    auto getUv = [](
        const SMikkTSpaceContext* context,
        float uvOut[],
        const int32_t faceIndex,
        const int32_t vertIndex) -> void {

        auto* meshData = static_cast<TAssetMesh*>(context->m_pUserData);
        auto index = meshData->Indices[faceIndex * 3 + vertIndex];
        const glm::vec2& uv = meshData->Uvs[index];
        uvOut[0] = uv.x;
        uvOut[1] = uv.y;
    };

    auto setTSpaceBasic = [](
        const SMikkTSpaceContext* context,
        const float tangent[],
        const float sign,
        const int32_t faceIndex,
        const int32_t vertIndex) {

        auto* meshData = static_cast<TAssetMesh*>(context->m_pUserData);
        auto index = meshData->Indices[faceIndex * 3 + vertIndex];

        glm::vec3 t(tangent[0], tangent[1], tangent[2]);
        meshData->Tangents[index] = glm::vec4(glm::normalize(t), sign);
    };

    auto interface = SMikkTSpaceInterface{
        .m_getNumFaces = getNumFaces,
        .m_getNumVerticesOfFace = getNumVerticesOfFace,
        .m_getPosition = getPosition,
        .m_getNormal = getNormal,
        .m_getTexCoord = getUv,
        .m_setTSpaceBasic = setTSpaceBasic,
    };

    auto context = SMikkTSpaceContext{
        .m_pInterface = &interface,
        .m_pUserData = &assetMeshData
    };

    genTangSpaceDefault(&context);
}

auto CreateUvSphereMeshData(
    const std::string& name,
    float radius,
    int32_t rings,
    int32_t segments) -> TAssetMesh {

    TAssetMesh assetMeshData;

    assetMeshData.Name = name;
    assetMeshData.Positions.resize(rings * segments * 4);
    assetMeshData.Normals.resize(rings * segments * 4);
    assetMeshData.Uvs.resize(rings * segments * 4);
    assetMeshData.Tangents.resize(rings * segments * 4);

    auto index = 0;
    constexpr auto pi = glm::pi<float>();

    for (auto ring = 0; ring <= rings; ++ring) {

        auto theta = ring * pi / rings;
        auto sinTheta = glm::sin(theta);
        auto cosTheta = glm::cos(theta);

        for (auto segment = 0; segment <= segments; ++segment) {

            auto phi = segment * 2.0f * pi / segments;
            auto sinPhi = glm::sin(phi);
            auto cosPhi = glm::cos(phi);

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
            assetMeshData.Normals[index] = glm::normalize(normal);
            assetMeshData.Uvs[index] = uv;
            assetMeshData.Tangents[index] = glm::vec4{0.0f};
            index++;
        }
    }

    assetMeshData.Indices.resize(rings * segments * 6);

    index = 0;
    for (auto ring = 0; ring < rings; ++ring) {
        for (auto segment = 0; segment < segments; ++segment) {
            auto first = (ring * (segments + 1)) + segment;
            auto second = first + segments + 1;

            assetMeshData.Indices[index + 0] = first;
            assetMeshData.Indices[index + 1] = first + 1;
            assetMeshData.Indices[index + 2] = second;

            assetMeshData.Indices[index + 3] = second;
            assetMeshData.Indices[index + 4] = first + 1;
            assetMeshData.Indices[index + 5] = second + 1;

            index += 6;
        }
    }

    assetMeshData.MaterialName = std::nullopt;

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
    uint32_t segmentsZ = 1) -> TAssetMesh {

    auto halfWidth = width / 2.0f;
    auto halfHeight = height / 2.0f;
    auto halfDepth = depth / 2.0f;

    TAssetMesh assetMeshData;
    assetMeshData.Name = name;
    assetMeshData.MaterialName = std::nullopt;
    //assetMeshData.InitialTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, halfHeight, 0));

    std::size_t vertOffset = 0;

    // Front Face
    glm::vec3 normal = {0.0f, 0.0f, 1.0f};
    for (auto iy = 0; iy < segmentsY; iy++) {
        for (auto ix = 0; ix < segmentsX; ix++) {

            glm::vec2 uv;
            uv.x = (static_cast<float>(ix)/(static_cast<float>(segmentsX) - 1.0f));
            uv.y = 1.0f - (static_cast<float>(iy) / (static_cast<float>(segmentsY) - 1.0f));

            glm::vec3 position;
            position.x = uv.x * width - halfWidth;
            position.y = -(uv.y - 1.0f) * height - halfHeight;
            position.z = halfDepth;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4(0.0f));
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsX - 1; x++) {
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + vertOffset);            

            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Right Side Face
    normal = {1.0f, 0.0f, 0.0f};
    for (auto iy = 0; iy < segmentsY; iy++) {
        for (auto ix = 0; ix < segmentsZ; ix++) {

            glm::vec2 uv;
            uv.x = (static_cast<float>(ix) / (static_cast<float>(segmentsZ) - 1.0f));
            uv.y = 1.0f - (static_cast<float>(iy) / (static_cast<float>(segmentsY) - 1.0f));

            glm::vec3 position;
            position.x = halfWidth;
            position.y = -(uv.y - 1.0f) * height - halfHeight;
            position.z = uv.x * -depth + halfDepth;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsZ - 1; x++) {
            assetMeshData.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);            
            assetMeshData.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsZ + x + vertOffset);            

            assetMeshData.Indices.push_back((y + 1) * segmentsZ + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Left Side Face
    normal = {-1.0f, 0.0f, 0.0f};
    for (auto iy = 0; iy < segmentsY; iy++) {
        for (auto ix = 0; ix < segmentsZ; ix++) {

            glm::vec2 uv;
            uv.x = (static_cast<float>(ix) / (static_cast<float>(segmentsZ) - 1.0f));
            uv.y = 1.0f - (static_cast<float>(iy) / (static_cast<float>(segmentsY) - 1.0f));

            glm::vec3 position;
            position.x = -halfWidth;
            position.y = -(uv.y - 1.0f) * height - halfHeight;
            position.z = uv.x * depth - halfDepth;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsZ - 1; x++) {
            assetMeshData.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsZ + x + vertOffset);            

            assetMeshData.Indices.push_back((y + 1) * segmentsZ + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Back Face
    normal = {0.0f, 0.0f, -1.0f};
    for (auto iy = 0; iy < segmentsY; iy++) {
        for (auto ix = 0; ix < segmentsX; ix++) {

            glm::vec2 uv;
            uv.x = (static_cast<float>(ix) / (static_cast<float>(segmentsX) - 1.0f));
            uv.y = 1.0f - (static_cast<float>(iy) / (static_cast<float>(segmentsY) - 1.0f));

            glm::vec3 position;
            position.x = uv.x * -width + halfWidth;
            position.y = -(uv.y - 1.0f) * height - halfHeight;
            position.z = -halfDepth;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsX - 1; x++) {
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + vertOffset);            

            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Top Face
    normal = {0.0f, 1.0f, 0.0f};
    for (auto iy = 0; iy < segmentsZ; iy++) {
        for (auto ix = 0; ix < segmentsX; ix++) {

            glm::vec2 uv;
            uv.x = (static_cast<float>(ix) / (static_cast<float>(segmentsX) - 1.0f));
            uv.y = 1.f - (static_cast<float>(iy) / (static_cast<float>(segmentsZ) - 1.0f));

            glm::vec3 position;
            position.x = uv.x * width - halfWidth;
            position.y = -halfHeight;
            position.z = uv.y * depth - halfDepth;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsZ - 1; y++) {
        for (auto x = 0; x < segmentsX - 1; x++) {
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);            
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + vertOffset);

            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);            
        }
    }

    vertOffset = assetMeshData.Positions.size();

    // Bottom Face
    normal = {0.0f, -1.0f, 0.0f};
    for (auto iy = 0; iy < segmentsZ; iy++) {
        for (auto ix = 0; ix < segmentsX; ix++) {

            glm::vec2 uv;
            uv.x = static_cast<float>(ix) / static_cast<float>(segmentsX - 1.0f);
            uv.y = 1.0f - static_cast<float>(iy) / static_cast<float>(segmentsZ - 1.0f);

            glm::vec3 position;
            position.x = uv.x * width - halfWidth;
            position.y = halfHeight;
            position.z = uv.y * -depth + halfDepth;

            assetMeshData.Positions.push_back(position);
            assetMeshData.Uvs.push_back(uv);
            assetMeshData.Normals.push_back(normal);
            assetMeshData.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsZ-1; y++) {
        for (auto x = 0; x < segmentsX-1; x++) {
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + vertOffset);
            
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetMeshData.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetMeshData.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            
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

    TAssetModel cuboid = {};
    cuboid.Materials.push_back(materialName);
    cuboid.Meshes.push_back(name);
    cuboid.Hierarchy.push_back(TAssetModelNode{
        .Name = name,
        .LocalPosition = glm::vec3(0.0f, 0.0f, 0.0f),
        .LocalRotation = glm::identity<glm::quat>(),
        .LocalScale = glm::vec3(1.0f),
        .MeshName = name,
    });
   
    g_assetModels[name] = std::move(cuboid);
    g_assetMeshDates[name] = CreateCuboid(name, width, height, depth, segmentsX, segmentsY, segmentsZ);
}

auto AddAssetModel(const std::string& assetName, const TAssetModel& asset) -> void {
    if (!g_assetModels.contains(assetName)) {
        g_assetModels[assetName] = std::move(asset);
    }
}

auto AddDefaultAssets() -> void {

    AddImage("T_Default_B", "data/default/T_Default_B.png");
    AddImage("T_Default_N", "data/default/T_Default_N.png");
    AddImage("T_Default_S", "data/default/T_Default_S.png");
    AddImage("T_Default_MR", "data/default/T_Default_MR.png");

    AddImage("T_Purple", "data/default/T_Purple.png");
    AddImage("T_Orange", "data/default/T_Orange.png");

    AddImage("T_Yellow", "data/default/T_Yellow.png");
    AddImage("T_Blue", "data/default/T_Blue.png");
    AddImage("T_Gray", "data/default/T_Gray.png");

    auto defaultAssetSampler = TAssetSampler {
        .Name = "S_L_L_C2E_C2E",
        .MagFilter = TAssetSamplerMagFilter::Linear,
        .MinFilter = TAssetSamplerMinFilter::Linear,
        .WrapS = TAssetSamplerWrapMode::ClampToEdge,
        .WrapT = TAssetSamplerWrapMode::ClampToEdge,
    };
    g_assetSamplerDates["S_L_L_C2E_C2E"] = std::move(defaultAssetSampler);

    auto defaultMaterial = TAssetMaterial {
        .Name = "M_Default",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Purple"
        },
    };
    g_assetMaterialDates["M_Default"] = std::move(defaultMaterial);

    auto orangeMaterial = TAssetMaterial {
        .Name = "M_Orange",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Orange"
        },
    };
    g_assetMaterialDates["M_Orange"] = std::move(orangeMaterial);

    auto yellowMaterial = TAssetMaterial {
        .Name = "M_Yellow",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Yellow"
        },
    };
    g_assetMaterialDates["M_Yellow"] = std::move(yellowMaterial);

    auto blueMaterial = TAssetMaterial {
        .Name = "M_Blue",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Blue"
        },
    };
    g_assetMaterialDates["M_Blue"] = std::move(blueMaterial);

    auto grayMaterial = TAssetMaterial {
        .Name = "M_Gray",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Gray"
        },
    };
    g_assetMaterialDates["M_Gray"] = std::move(grayMaterial);

    AddImage("T_Mars_B", "data/default/2k_mars.jpg");
    auto marsMaterial = TAssetMaterial {
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
