#include "Assets.hpp"
#include "Io.hpp"
#include "Images.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include <parallel_hashmap/phmap.h>

#include <spdlog/spdlog.h>
#include <mikktspace.h>

#include <algorithm>
#include <format>
#include <ranges>
#include <stack>
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
std::unordered_map<std::string, TAssetImage> g_assetImages = {};
std::unordered_map<std::string, TAssetSampler> g_assetSamplers = {};
std::unordered_map<std::string, TAssetMaterial> g_assetMaterials = {};
std::unordered_map<std::string, TAssetPrimitive> g_assetPrimitives = {};
std::unordered_map<std::string, TAssetMesh> g_assetMeshes = {};

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
        g_assetImages[assetImage.Name] = std::move(assetImage);
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
        if (!g_assetSamplers.contains(samplerName)) {
            g_assetSamplers[samplerName] = std::move(assetSamplerData);
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
        g_assetMaterials[assetMaterial.Name] = std::move(assetMaterial);
    }
}

auto LoadMeshes(
    const fastgltf::Asset& fgAsset,
    TAssetModel& assetModel) -> void {

    assetModel.Meshes.resize(fgAsset.meshes.size());
    auto meshes = std::vector<TAssetMesh>(fgAsset.meshes.size(), TAssetMesh());

    static auto globalPrimitiveIndex = 0;
    for (auto meshIndex = 0; meshIndex < fgAsset.meshes.size(); ++meshIndex) {
        const auto& fgMesh = fgAsset.meshes[meshIndex];
        const auto& meshName = GetSafeResourceName(assetModel.Name.data(), fgMesh.name.data(), "mesh", meshIndex);

        auto& assetMesh = meshes[meshIndex];
        assetMesh.Primitives.resize(fgMesh.primitives.size());
        assetMesh.Name = meshName;
        assetModel.Meshes[meshIndex] = meshName;

        for (size_t primitiveIndex = 0; primitiveIndex < fgMesh.primitives.size(); ++primitiveIndex) {

            const auto& fgPrimitive = fgAsset.meshes[meshIndex].primitives[primitiveIndex];
            auto& assetPrimitive = assetMesh.Primitives[primitiveIndex];
            auto assetPrimitiveName = GetSafeResourceName(assetModel.Name.data(), nullptr, "primitive", globalPrimitiveIndex++);

            assetPrimitive.Name = assetPrimitiveName;
            assetPrimitive.MaterialName = fgPrimitive.materialIndex.has_value()
                ? assetModel.Materials[fgPrimitive.materialIndex.value()]
                : "T-Default";

            auto& indices = fgAsset.accessors[fgPrimitive.indicesAccessor.value()];
            assetPrimitive.Indices.resize(indices.count);
            fastgltf::copyFromAccessor<uint32_t>(fgAsset, indices, assetPrimitive.Indices.data());

            auto& positions = fgAsset.accessors[fgPrimitive.findAttribute("POSITION")->accessorIndex];
            assetPrimitive.Positions.resize(positions.count);
            fastgltf::copyFromAccessor<glm::vec3>(fgAsset, positions, assetPrimitive.Positions.data());
            if (auto* normalsAttribute = fgPrimitive.findAttribute("NORMAL"); normalsAttribute != fgPrimitive.attributes.end()) {
                auto& normals = fgAsset.accessors[normalsAttribute->accessorIndex];
                assetPrimitive.Normals.resize(normals.count);
                fastgltf::copyFromAccessor<glm::vec3>(fgAsset, normals, assetPrimitive.Normals.data());
            } else {
                assetPrimitive.Normals.resize(assetPrimitive.Positions.size());
                std::fill_n(assetPrimitive.Normals.data(), assetPrimitive.Positions.size(), glm::vec3(0.5f, 0.5f, 1.0f));
            }

            if (auto* uv0Attribute = fgPrimitive.findAttribute("TEXCOORD_0"); uv0Attribute != fgPrimitive.attributes.end()) {
                auto& uv0s = fgAsset.accessors[uv0Attribute->accessorIndex];
                assetPrimitive.Uvs.resize(uv0s.count);
                fastgltf::copyFromAccessor<glm::vec2>(fgAsset, uv0s, assetPrimitive.Uvs.data());
            } else {
                assetPrimitive.Uvs.resize(assetPrimitive.Positions.size());
                std::fill_n(assetPrimitive.Uvs.begin(), assetPrimitive.Positions.size(), glm::vec2(0.0f, 0.0f));
            }

            if (auto* tangentAttribute = fgPrimitive.findAttribute("TANGENT"); tangentAttribute != fgPrimitive.attributes.end()) {
                auto& tangents = fgAsset.accessors[tangentAttribute->accessorIndex];
                assetPrimitive.Tangents.resize(tangents.count);
                fastgltf::copyFromAccessor<glm::vec4>(fgAsset, tangents, assetPrimitive.Tangents.data());
            } else  {
                assetPrimitive.Tangents.resize(assetPrimitive.Positions.size());
                std::fill_n(assetPrimitive.Tangents.begin(), assetPrimitive.Positions.size(), glm::vec4{1.0f});
            }

            g_assetPrimitives[assetPrimitiveName] = assetPrimitive;
        }
    }

    for (const auto& mesh : meshes) {
        g_assetMeshes[mesh.Name] = std::move(mesh);
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

    // the whole thing non recursive
    /*
    struct TStackEntry {
        size_t NodeIndex;
        TAssetModelNode* Parent;
    };

    std::stack<TStackEntry> stack;

    for (const auto rootIndex : fgAsset.scenes[0].nodeIndices) {
        stack.push({ rootIndex, nullptr });
    }

    auto nodeId = 0;

    while (!stack.empty()) {
        auto [nodeIndex, parent] = stack.top();
        stack.pop();

        const auto& srcNode = fgAsset.nodes[nodeIndex];
        TAssetModelNode newNode;
        newNode.Name = GetSafeResourceName(assetModel.Name.data(), srcNode.name.data(), "node", nodeId++);

        const auto& [translation, rotation, scale] = std::get<fastgltf::TRS>(srcNode.transform);
        newNode.LocalPosition = glm::make_vec3(translation.data());;
        newNode.LocalRotation = glm::quat{rotation[3], rotation[0], rotation[1], rotation[2]};
        newNode.LocalScale = glm::make_vec3(scale.data());

        if (srcNode.meshIndex && !assetModel.Meshes.empty()) {
            const size_t meshId = *srcNode.meshIndex;
            if (meshId < assetModel.Meshes.size()) {
                newNode.MeshName = assetModel.Meshes[meshId];
            } else {
                newNode.MeshName = std::nullopt;
            }
        } else {
            newNode.MeshName = std::nullopt;
        }

        if (parent) {
            parent->Children.push_back(std::move(newNode));
            parent = &parent->Children.back();
        } else {
            assetModel.Hierarchy.push_back(std::move(newNode));
            parent = &assetModel.Hierarchy.back();
        }

        // Push children
        //if (srcNode.children..has_value()) {
        for (const auto childIndex : srcNode.children) {
            stack.push({ childIndex, parent });
        }
        //}
    }
    */

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

auto GetAssetPrimitive(const std::string& assetPrimitiveName) -> TAssetPrimitive& {
    return g_assetPrimitives[assetPrimitiveName];
}

auto IsAssetLoaded(const std::string& assetName) -> bool {
    return g_assetModels.contains(assetName);
}

auto GetAssetImage(const std::string& imageDataName) -> TAssetImage& {
    return g_assetImages[imageDataName];
}

auto GetAssetSampler(const std::string& samplerDataName) -> TAssetSampler& {
    return g_assetSamplers[samplerDataName];
}

auto GetAssetMaterial(const std::string& materialDataName) -> TAssetMaterial& {
    return g_assetMaterials[materialDataName];
}

auto GetAssetMesh(const std::string& meshDataName) -> TAssetMesh& {
    return g_assetMeshes[meshDataName];
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

    g_assetImages[imageName] = std::move(assetImage);
}

auto CalculateTangents(TAssetPrimitive& assetPrimitive) -> void {

    auto getNumFaces = [](const SMikkTSpaceContext* context) -> int32_t {

        auto* primitive = static_cast<TAssetPrimitive*>(context->m_pUserData);
        return primitive->Indices.size() / 3;
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

        auto* primitive = static_cast<TAssetPrimitive*>(context->m_pUserData);
        auto index = primitive->Indices[faceIndex * 3 + vertIndex];
        const glm::vec3& pos = primitive->Positions[index];
        posOut[0] = pos.x;
        posOut[1] = pos.y;
        posOut[2] = pos.z;
    };

    auto getNormal = [](
        const SMikkTSpaceContext* context,
        float normOut[],
        const int32_t faceIndex,
        const int32_t vertIndex) -> void {

        auto* primitive = static_cast<TAssetPrimitive*>(context->m_pUserData);
        auto index = primitive->Indices[faceIndex * 3 + vertIndex];
        const glm::vec3& normal = primitive->Normals[index];
        normOut[0] = normal.x;
        normOut[1] = normal.y;
        normOut[2] = normal.z;
    };

    auto getUv = [](
        const SMikkTSpaceContext* context,
        float uvOut[],
        const int32_t faceIndex,
        const int32_t vertIndex) -> void {

        auto* primitive = static_cast<TAssetPrimitive*>(context->m_pUserData);
        auto index = primitive->Indices[faceIndex * 3 + vertIndex];
        const glm::vec2& uv = primitive->Uvs[index];
        uvOut[0] = uv.x;
        uvOut[1] = uv.y;
    };

    auto setTSpaceBasic = [](
        const SMikkTSpaceContext* context,
        const float tangent[],
        const float sign,
        const int32_t faceIndex,
        const int32_t vertIndex) {

        auto* primitive = static_cast<TAssetPrimitive*>(context->m_pUserData);
        auto index = primitive->Indices[faceIndex * 3 + vertIndex];

        glm::vec3 t(tangent[0], tangent[1], tangent[2]);
        primitive->Tangents[index] = glm::vec4(glm::normalize(t), sign);
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
        .m_pUserData = &assetPrimitive
    };

    genTangSpaceDefault(&context);
}

auto CreateUvSphereMeshData(
    const std::string& name,
    float radius,
    int32_t rings,
    int32_t segments) -> TAssetMesh {

    TAssetPrimitive assetPrimitive;
    assetPrimitive.Positions.resize(rings * segments * 4);
    assetPrimitive.Normals.resize(rings * segments * 4);
    assetPrimitive.Uvs.resize(rings * segments * 4);
    assetPrimitive.Tangents.resize(rings * segments * 4);

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

            assetPrimitive.Positions[index] = position;
            assetPrimitive.Normals[index] = glm::normalize(normal);
            assetPrimitive.Uvs[index] = uv;
            assetPrimitive.Tangents[index] = glm::vec4{0.0f};
            index++;
        }
    }

    assetPrimitive.Indices.resize(rings * segments * 6);

    index = 0;
    for (auto ring = 0; ring < rings; ++ring) {
        for (auto segment = 0; segment < segments; ++segment) {
            auto first = (ring * (segments + 1)) + segment;
            auto second = first + segments + 1;

            assetPrimitive.Indices[index + 0] = first;
            assetPrimitive.Indices[index + 1] = first + 1;
            assetPrimitive.Indices[index + 2] = second;

            assetPrimitive.Indices[index + 3] = second;
            assetPrimitive.Indices[index + 4] = first + 1;
            assetPrimitive.Indices[index + 5] = second + 1;

            index += 6;
        }
    }

    assetPrimitive.MaterialName = std::nullopt;
    CalculateTangents(assetPrimitive);

    TAssetMesh assetMeshData;
    assetMeshData.Name = name;
    assetMeshData.Primitives.push_back(assetPrimitive);

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

    //assetMeshData.InitialTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, halfHeight, 0));

    std::size_t vertOffset = 0;
    TAssetPrimitive assetPrimitiveFront;
    assetPrimitiveFront.MaterialName = std::nullopt;

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

            assetPrimitiveFront.Positions.push_back(position);
            assetPrimitiveFront.Uvs.push_back(uv);
            assetPrimitiveFront.Normals.push_back(normal);
            assetPrimitiveFront.Tangents.push_back(glm::vec4(0.0f));
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsX - 1; x++) {
            assetPrimitiveFront.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveFront.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveFront.Indices.push_back((y) * segmentsX + x + vertOffset);

            assetPrimitiveFront.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveFront.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveFront.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
        }
    }

    //vertOffset = assetMeshData.Positions.size();
    vertOffset = 0;
    TAssetPrimitive assetPrimitiveRight;
    assetPrimitiveRight.MaterialName = std::nullopt;

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

            assetPrimitiveRight.Positions.push_back(position);
            assetPrimitiveRight.Uvs.push_back(uv);
            assetPrimitiveRight.Normals.push_back(normal);
            assetPrimitiveRight.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsZ - 1; x++) {
            assetPrimitiveRight.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);
            assetPrimitiveRight.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetPrimitiveRight.Indices.push_back((y) * segmentsZ + x + vertOffset);

            assetPrimitiveRight.Indices.push_back((y + 1) * segmentsZ + x + 1 + vertOffset);
            assetPrimitiveRight.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetPrimitiveRight.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);
        }
    }

    //vertOffset = assetMeshData.Positions.size();
    vertOffset = 0;
    TAssetPrimitive assetPrimitiveLeft;
    assetPrimitiveLeft.MaterialName = std::nullopt;

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

            assetPrimitiveLeft.Positions.push_back(position);
            assetPrimitiveLeft.Uvs.push_back(uv);
            assetPrimitiveLeft.Normals.push_back(normal);
            assetPrimitiveLeft.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsZ - 1; x++) {
            assetPrimitiveLeft.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);
            assetPrimitiveLeft.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetPrimitiveLeft.Indices.push_back((y) * segmentsZ + x + vertOffset);

            assetPrimitiveLeft.Indices.push_back((y + 1) * segmentsZ + x + 1 + vertOffset);
            assetPrimitiveLeft.Indices.push_back((y + 1) * segmentsZ + x + vertOffset);
            assetPrimitiveLeft.Indices.push_back((y) * segmentsZ + x + 1 + vertOffset);
        }
    }

    //vertOffset = assetMeshData.Positions.size();
    vertOffset = 0;
    TAssetPrimitive assetPrimitiveBack;
    assetPrimitiveBack.MaterialName = std::nullopt;

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

            assetPrimitiveBack.Positions.push_back(position);
            assetPrimitiveBack.Uvs.push_back(uv);
            assetPrimitiveBack.Normals.push_back(normal);
            assetPrimitiveBack.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsY - 1; y++) {
        for (auto x = 0; x < segmentsX - 1; x++) {
            assetPrimitiveBack.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveBack.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveBack.Indices.push_back((y) * segmentsX + x + vertOffset);

            assetPrimitiveBack.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveBack.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveBack.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
        }
    }

    //vertOffset = assetMeshData.Positions.size();
    vertOffset = 0;

    TAssetPrimitive assetPrimitiveTop;
    assetPrimitiveTop.MaterialName = std::nullopt;

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

            assetPrimitiveTop.Positions.push_back(position);
            assetPrimitiveTop.Uvs.push_back(uv);
            assetPrimitiveTop.Normals.push_back(normal);
            assetPrimitiveTop.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsZ - 1; y++) {
        for (auto x = 0; x < segmentsX - 1; x++) {
            assetPrimitiveTop.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveTop.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveTop.Indices.push_back((y) * segmentsX + x + vertOffset);

            assetPrimitiveTop.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveTop.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveTop.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
        }
    }

    //vertOffset = assetMeshData.Positions.size();
    vertOffset = 0;

    TAssetPrimitive assetPrimitiveBottom;
    assetPrimitiveBottom.MaterialName = std::nullopt;
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

            assetPrimitiveBottom.Positions.push_back(position);
            assetPrimitiveBottom.Uvs.push_back(uv);
            assetPrimitiveBottom.Normals.push_back(normal);
            assetPrimitiveBottom.Tangents.push_back(glm::vec4{0.0f});
        }
    }

    for (auto y = 0; y < segmentsZ-1; y++) {
        for (auto x = 0; x < segmentsX-1; x++) {
            assetPrimitiveBottom.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveBottom.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveBottom.Indices.push_back((y) * segmentsX + x + vertOffset);
            
            assetPrimitiveBottom.Indices.push_back((y + 1) * segmentsX + x + vertOffset);
            assetPrimitiveBottom.Indices.push_back((y + 1) * segmentsX + x + 1 + vertOffset);
            assetPrimitiveBottom.Indices.push_back((y) * segmentsX + x + 1 + vertOffset);
        }
    }

    TAssetMesh assetMeshData;
    assetMeshData.Name = name;
    assetMeshData.Primitives.push_back(assetPrimitiveFront);
    assetMeshData.Primitives.push_back(assetPrimitiveBack);
    assetMeshData.Primitives.push_back(assetPrimitiveLeft);
    assetMeshData.Primitives.push_back(assetPrimitiveRight);
    assetMeshData.Primitives.push_back(assetPrimitiveTop);
    assetMeshData.Primitives.push_back(assetPrimitiveBottom);

    for (auto& assetPrimitive : assetMeshData.Primitives) {
        CalculateTangents(assetPrimitive);
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
    g_assetMeshes[name] = CreateCuboid(name, width, height, depth, segmentsX, segmentsY, segmentsZ);
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
    g_assetSamplers["S_L_L_C2E_C2E"] = std::move(defaultAssetSampler);

    auto defaultMaterial = TAssetMaterial {
        .Name = "M_Default",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Purple"
        },
    };
    g_assetMaterials["M_Default"] = std::move(defaultMaterial);

    auto orangeMaterial = TAssetMaterial {
        .Name = "M_Orange",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Orange"
        },
    };
    g_assetMaterials["M_Orange"] = std::move(orangeMaterial);

    auto yellowMaterial = TAssetMaterial {
        .Name = "M_Yellow",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Yellow"
        },
    };
    g_assetMaterials["M_Yellow"] = std::move(yellowMaterial);

    auto blueMaterial = TAssetMaterial {
        .Name = "M_Blue",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Blue"
        },
    };
    g_assetMaterials["M_Blue"] = std::move(blueMaterial);

    auto grayMaterial = TAssetMaterial {
        .Name = "M_Gray",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Gray"
        },
    };
    g_assetMaterials["M_Gray"] = std::move(grayMaterial);

    AddImage("T_Mars_B", "data/default/2k_mars.jpg");
    auto marsMaterial = TAssetMaterial {
        .Name = "M_Mars",
        .BaseColorTextureChannel = TAssetMaterialChannelData {
            .Channel = TAssetMaterialChannel::Color,
            .SamplerName = "S_L_L_C2E_C2E",
            .TextureName = "T_Mars_B"
        }
    };
    g_assetMaterials["M_Mars"] = std::move(marsMaterial);

    auto geodesic = CreateUvSphereMeshData("SM_Geodesic", 1, 64, 64);
    g_assetMeshes["SM_Geodesic"] = std::move(geodesic);

    for (auto i = 1; i < 11; i++) {
        CreateCuboid(std::format("SM_Cuboid_x{}_y1_z1", i), i, 1.0f, 1.0f, i + 1, i + 1, i + 1, "M_Orange");
        CreateCuboid(std::format("SM_Cuboid_x1_y{}_z1", i), 1.0f, i, 1.0f, i + 1, i + 1, i + 1, "M_Orange");
        CreateCuboid(std::format("SM_Cuboid_x1_y1_z{}", i), 1.0f, 1.0f, i, i + 1, i + 1, i + 1, "M_Orange");
    }

    CreateCuboid("SM_Cuboid_x50_y1_z50", 50.0f, 1.0f, 50.0f, 4, 4, 4, "M_Orange");
}

}
