//#include <mimalloc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <expected>
#include <filesystem>
#include <ranges>
#include <span>
#include <stack>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <debugbreak.h>
#include <spdlog/spdlog.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <entt/entt.hpp>

#include "Core.hpp"
#include "Profiler.hpp"
#include "Io.hpp"
#include "Images.hpp"
#include "Helpers.hpp"
#include "RHI.hpp"
#include "Assets.hpp"

#include "IconsMaterialDesign.h"
#include "IconsFontAwesome6.h"
#include "Renderer.hpp"

// - Core Types ---------------------------------------------------------------

enum class TWindowStyle {
    Windowed,
    Fullscreen,
    FullscreenExclusive
};

struct TWindowSettings {
    int32_t ResolutionWidth;
    int32_t ResolutionHeight;
    float ResolutionScale;
    TWindowStyle WindowStyle;
    bool IsDebug;
    bool IsVSyncEnabled;
};

// - Engine -------------------------------------------------------------------

struct TGpuVertexPosition {
    glm::vec3 Position;
};

struct TGpuPackedVertexNormalTangentUvTangentSign {
    uint32_t Normal;
    uint32_t Tangent;
    glm::vec4 UvAndTangentSign;
};

struct TGpuVertexPositionColor {
    glm::vec3 Position;
    glm::vec4 Color;
};

struct TGpuGlobalUniforms {

    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 CameraPosition;
    glm::vec4 CameraDirection;
};

struct TGpuGlobalLight {
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 Direction;
    glm::vec4 Strength;
};

struct TGpuDebugLine {
    glm::vec3 StartPosition;
    glm::vec4 StartColor;
    glm::vec3 EndPosition;
    glm::vec4 EndColor;
};

struct TGpuMesh {
    std::string_view Name;
    uint32_t VertexPositionBuffer;
    uint32_t VertexNormalUvTangentBuffer;
    uint32_t IndexBuffer;

    size_t VertexCount;
    size_t IndexCount;

    glm::mat4 InitialTransform;
};

struct TCpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = basecolor factor .y = normal strength, .z = metalness, .w = roughness

    uint32_t BaseColorTextureId;
    uint32_t BaseColorTextureSamplerId;
    uint32_t NormalTextureId;
    uint32_t NormalTextureSamplerId;

    uint32_t ArmTextureId;
    uint32_t ArmTextureSamplerId;
    uint32_t EmissiveTextureId;
    uint32_t EmissiveTextureSamplerId;
};

struct TGpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = basecolor factor .y = normal strength, .z = metalness, .w = roughness

    uint64_t BaseColorTexture;
    uint64_t NormalTexture;

    uint64_t ArmTexture;
    uint64_t EmissiveTexture;

    uint32_t BaseColorTextureId;
    uint32_t NormalTextureId;
};

struct TCpuGlobalLight {

    float Azimuth;
    float Elevation;
    glm::vec3 Color;
    float Strength;

    int32_t ShadowMapSize;
    float ShadowMapNear;
    float ShadowMapFar;

    bool ShowFrustum;
};

struct TGpuObject {
    glm::mat4x4 WorldMatrix;
    glm::ivec4 InstanceParameter;
};

struct TMeshInstance {
    glm::mat4 WorldMatrix;
};

struct TCamera {

    glm::vec3 Position = {0.0f, 0.0f, 5.0f};
    float Pitch = {};
    float Yaw = {glm::radians(-90.0f)}; // look at 0, 0, -1

    auto GetForwardDirection() -> const glm::vec3
    {
        return glm::vec3{cos(Pitch) * cos(Yaw), sin(Pitch), cos(Pitch) * sin(Yaw)};
    }

    auto GetViewMatrix() -> const glm::mat4
    {
        return glm::lookAt(Position, Position + GetForwardDirection(), glm::vec3(0, 1, 0));
    }
};

// - Assets -------------------------------------------------------------------

using TAssetMeshId = TId<struct AssetMeshIdMarker>;
using TAssetImageId = TId<struct AssetImageIdMarker>;
using TAssetTextureId = TId<struct AssetTextureIdMarker>;
using TAssetSamplerId = TId<struct AssetSamplerIdMarker>;
using TAssetMaterialId = TId<struct AssetMaterialIdMarker>;

enum class TMaterialChannelUsage {
    SRgb, // use sRGB Format
    Normal, // use RG-format or bc7 when compressed
    MetalnessRoughness // single channelism or also RG?
};

struct TAssetMesh {
    std::string_view Name;
    glm::mat4 InitialTransform;
    std::vector<TGpuVertexPosition> VertexPositions;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> VertexNormalUvTangents;
    std::vector<uint32_t> Indices;
    std::string MaterialName;
};

struct TAssetSampler {
    TTextureAddressMode AddressModeU = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeV = TTextureAddressMode::ClampToEdge;
    TTextureMagFilter MagFilter = TTextureMagFilter::Linear;
    TTextureMinFilter MinFilter = TTextureMinFilter::Linear;
};

struct TAssetImage {

    int32_t Width = 0;
    int32_t Height = 0;
    int32_t PixelType = 0;
    int32_t Bits = 8;
    int32_t Components = 0;
    std::string Name;

    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;

    std::unique_ptr<unsigned char[]> DecodedData = {};

    size_t Index = 0;
};

struct TAssetTexture {
    std::string ImageName;
    std::string SamplerName;
};

struct TAssetMaterialChannel {
    TMaterialChannelUsage Usage;
    std::string Image;
    std::string Sampler; 
};

struct TAssetMaterial {
    glm::vec4 BaseColorFactor;
    float MetallicFactor;
    glm::vec3 EmissiveFactor;
    float EmissiveStrength;
    float NormalStrength;
    std::optional<TAssetMaterialChannel> BaseColorChannel;
    std::optional<TAssetMaterialChannel> NormalsChannel;
    std::optional<TAssetMaterialChannel> OcclusionRoughnessMetallnessChannel;
    std::optional<TAssetMaterialChannel> EmissiveChannel;
};

std::unordered_map<std::string, TAssetImage> g_assetImages = {};
std::unordered_map<std::string, TAssetSampler> g_assetSamplers = {};
std::unordered_map<std::string, TAssetTexture> g_assetTextures = {};
std::unordered_map<std::string, TAssetMaterial> g_assetMaterials = {};
std::unordered_map<std::string, TAssetMesh> g_assetMeshes = {};
std::unordered_map<std::string, std::vector<std::string>> g_assetModelMeshes = {};

auto AddAssetMesh(
    const std::string& assetMeshName,
    const TAssetMesh& assetMesh) -> void {

    assert(!assetMeshName.empty());
    PROFILER_ZONESCOPEDN("AddAssetMesh");

    g_assetMeshes[assetMeshName] = assetMesh;
}

auto AddAssetImage(
    const std::string& assetImageName,
    TAssetImage&& assetImage) -> void {

    assert(!assetImageName.empty());
    PROFILER_ZONESCOPEDN("AddAssetImage");

    g_assetImages[assetImageName] = std::move(assetImage);
}

auto AddAssetMaterial(
    const std::string& assetMaterialName,
    const TAssetMaterial& assetMaterial) -> void {

    assert(!assetMaterialName.empty());
    PROFILER_ZONESCOPEDN("AddAssetMaterial");

    g_assetMaterials[assetMaterialName] = assetMaterial;
}

auto AddAssetTexture(
    const std::string& assetTextureName,
    const TAssetTexture& assetTexture) -> void {

    assert(!assetTextureName.empty());
    PROFILER_ZONESCOPEDN("AddAssetTexture");

    g_assetTextures[assetTextureName] = assetTexture;
}

auto AddAssetSampler(
    const std::string& assetSamplerName,
    const TAssetSampler& assetSampler) -> void {

    assert(!assetSamplerName.empty());
    PROFILER_ZONESCOPEDN("AddAssetSampler");

    g_assetSamplers[assetSamplerName] = assetSampler;
}

auto GetAssetImage(const std::string& assetImageName) -> TAssetImage& {

    assert(!assetImageName.empty() || g_assetImages.contains(assetImageName));
    PROFILER_ZONESCOPEDN("GetAssetImage");

    return g_assetImages.at(assetImageName);
}

auto GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& {

    assert(!assetMeshName.empty() || g_assetMeshes.contains(assetMeshName));
    PROFILER_ZONESCOPEDN("GetAssetMesh");

    return g_assetMeshes.at(assetMeshName);
}

auto GetAssetMaterial(const std::string& assetMaterialName) -> TAssetMaterial& {

    assert(!assetMaterialName.empty() || g_assetMaterials.contains(assetMaterialName));
    PROFILER_ZONESCOPEDN("GetAssetMaterial");

    return g_assetMaterials.at(assetMaterialName);
}

auto GetAssetModelMeshNames(const std::string& modelName) -> std::vector<std::string>& {

    assert(!modelName.empty() || g_assetModelMeshes.contains(modelName));
    PROFILER_ZONESCOPEDN("GetAssetModelMeshNames");

    return g_assetModelMeshes.at(modelName);
}

auto GetAssetSampler(const std::string& assetSamplerName) -> TAssetSampler& {

    assert(!assetSamplerName.empty() || g_assetSamplers.contains(assetSamplerName));
    PROFILER_ZONESCOPEDN("GetAssetSampler");

    return g_assetSamplers.at(assetSamplerName);
}

/*
auto GetAssetSampler(const SAssetSamplerId& assetSamplerId) -> TSamplerDescriptor {

    PROFILER_ZONESCOPEDN("GetAssetSampler");
    for (const auto& [key, value] : g_assetSamplers) {
        if (value == assetSamplerId) {
            return key;
        }
    }

    spdlog::error("Unable to find Asset Sampler");
    return TSamplerDescriptor {};
}
*/

auto GetAssetTexture(const std::string& assetTextureName) -> TAssetTexture& {

    PROFILER_ZONESCOPEDN("GetAssetTexture");
    assert(!assetTextureName.empty());

    return g_assetTextures[assetTextureName];
}

auto GetLocalTransform(const fastgltf::Node& node) -> glm::mat4 {

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

    return transform;
}

auto SignNotZero(glm::vec2 v) -> glm::vec2 {

    return glm::vec2((v.x >= 0.0f) ? +1.0f : -1.0f, (v.y >= 0.0f) ? +1.0f : -1.0f);
}

auto EncodeNormal(glm::vec3 normal) -> glm::vec2 {

    glm::vec2 encodedNormal = glm::vec2{normal.x, normal.y} * (1.0f / (abs(normal.x) + abs(normal.y) + abs(normal.z)));
    return (normal.z <= 0.0f)
        ? ((1.0f - glm::abs(glm::vec2{encodedNormal.x, encodedNormal.y})) * SignNotZero(encodedNormal)) //TODO(deccer) check if its encNor.y, encNor.x or not
        : encodedNormal;
}

auto GetVertices(
    const fastgltf::Asset& asset, 
    const fastgltf::Primitive& primitive) -> std::pair<std::vector<TGpuVertexPosition>, std::vector<TGpuPackedVertexNormalTangentUvTangentSign>> {

    PROFILER_ZONESCOPEDN("GetVertices");

    std::vector<glm::vec3> positions;
    auto& positionAccessor = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
    positions.resize(positionAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  positionAccessor,
                                                  [&](glm::vec3 position, std::size_t index) { positions[index] = position; });

    std::vector<glm::vec3> normals;
    auto& normalAccessor = asset.accessors[primitive.findAttribute("NORMAL")->accessorIndex];
    normals.resize(normalAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  normalAccessor,
                                                  [&](glm::vec3 normal, std::size_t index) { normals[index] = normal; });

    std::vector<glm::vec4> tangents;
    auto& tangentAccessor = asset.accessors[primitive.findAttribute("TANGENT")->accessorIndex];
    tangents.resize(tangentAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                  tangentAccessor,
                                                  [&](glm::vec4 tangent, std::size_t index) { tangents[index] = tangent; });

    std::vector<glm::vec3> uvs;
    if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
    {
        auto& uvAccessor = asset.accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];
        uvs.resize(uvAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                      uvAccessor,
                                                      [&](glm::vec2 uv, std::size_t index) { uvs[index] = glm::vec3{uv, 0.0f}; });
    }
    else
    {
        uvs.resize(positions.size(), {});
    }

    std::vector<TGpuVertexPosition> verticesPosition;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> verticesNormalUvTangent;
    verticesPosition.resize(positions.size());
    verticesNormalUvTangent.resize(positions.size());

    for (size_t i = 0; i < positions.size(); i++) {
        verticesPosition[i] = {
            positions[i],
        };
        uvs[i].z = tangents[i].w;
        verticesNormalUvTangent[i] = {
            glm::packSnorm2x16(EncodeNormal(normals[i])),
            glm::packSnorm2x16(EncodeNormal(tangents[i].xyz())),
            glm::vec4(uvs[i], 0.0f),
        };
    }

    return {verticesPosition, verticesNormalUvTangent};
}

auto GetIndices(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive) -> std::vector<uint32_t> {

    PROFILER_ZONESCOPEDN("GetIndices");

    auto indices = std::vector<uint32_t>();
    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](uint32_t value, size_t index)
    {
        indices[index] = value;
    });
    return indices;
}

auto CreateAssetMesh(
    const std::string& name,
    glm::mat4 initialTransform,
    const std::pair<std::vector<TGpuVertexPosition>, std::vector<TGpuPackedVertexNormalTangentUvTangentSign>>& vertices,
    const std::vector<uint32_t> indices,
    const std::string& materialName) -> TAssetMesh {

    PROFILER_ZONESCOPEDN("CreateAssetMesh");

    return TAssetMesh{
        .Name = name,
        .InitialTransform = initialTransform,
        .VertexPositions = std::move(vertices.first),
        .VertexNormalUvTangents = std::move(vertices.second),
        .Indices = std::move(indices),
        .MaterialName = materialName
    };
}

auto CreateAssetImage(
    const void* data, 
    std::size_t dataSize, 
    fastgltf::MimeType mimeType,
    std::string_view name) -> TAssetImage {

    PROFILER_ZONESCOPEDN("CreateAssetImage");

    auto dataCopy = std::make_unique<std::byte[]>(dataSize);
    std::copy_n(static_cast<const std::byte*>(data), dataSize, dataCopy.get());

    return TAssetImage {
        .Name = std::string(name),
        .EncodedData = std::move(dataCopy),
        .EncodedDataSize = dataSize,
    };
}

auto CreateAssetMaterial(
    const std::string& modelName,
    const fastgltf::Asset& asset,
    const fastgltf::Material& fgMaterial,
    size_t assetMaterialIndex) -> void {

    PROFILER_ZONESCOPEDN("CreateAssetMaterial");

    TAssetMaterial assetMaterial = {};
    assetMaterial.BaseColorFactor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
    assetMaterial.MetallicFactor = fgMaterial.pbrData.metallicFactor;
    assetMaterial.EmissiveFactor = glm::make_vec3(fgMaterial.emissiveFactor.data());
    assetMaterial.EmissiveStrength = fgMaterial.emissiveStrength;

    if (fgMaterial.pbrData.baseColorTexture.has_value()) {
        auto& fgBaseColorTexture = fgMaterial.pbrData.baseColorTexture.value();
        auto& fgTexture = asset.textures[fgBaseColorTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.normalTexture.has_value()) {
        auto& fgNormalTexture = fgMaterial.normalTexture.value();
        auto& fgTexture = asset.textures[fgNormalTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::Normal,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.pbrData.metallicRoughnessTexture.has_value()) {
        auto& fgMetallicRoughnessTexture = fgMaterial.pbrData.metallicRoughnessTexture.value();
        auto& fgTexture = asset.textures[fgMetallicRoughnessTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.OcclusionRoughnessMetallnessChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::MetalnessRoughness,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.emissiveTexture.has_value()) {
        auto& fgEmissiveTexture = fgMaterial.emissiveTexture.value();
        auto& fgTexture = asset.textures[fgEmissiveTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }
    
    auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", assetMaterialIndex);

    return AddAssetMaterial(materialName, assetMaterial);
}

auto LoadModelFromFile(
    std::string modelName,
    std::filesystem::path filePath) -> void {

    PROFILER_ZONESCOPEDN("LoadModelFromFile");

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
        return;
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
        return;
    }

    auto& fgAsset = loadResult.get();

    // images

    auto assetImageIds = std::vector<TAssetImageId>(fgAsset.images.size());
    const auto assetImageIndices = std::ranges::iota_view{(size_t)0, fgAsset.images.size()};

    std::transform(
        poolstl::execution::par,
        assetImageIndices.begin(),
        assetImageIndices.end(),
        assetImageIds.begin(),
        [&](size_t imageIndex) {

        PROFILER_ZONESCOPEDN("Create Image");
        const auto& fgImage = fgAsset.images[imageIndex];
        auto imageData = [&] {
            if (const auto* filePathUri = std::get_if<fastgltf::sources::URI>(&fgImage.data)) {
                auto filePathFixed = std::filesystem::path(filePathUri->uri.path());
                auto filePathParent = filePath.parent_path();
                if (filePathFixed.is_relative()) {
                    filePathFixed = filePath.parent_path() / filePathFixed;
                }
                auto fileData = ReadBinaryFromFile(filePathFixed);
                return CreateAssetImage(fileData.first.get(), fileData.second, filePathUri->mimeType, fgImage.name);
            }
            if (const auto* array = std::get_if<fastgltf::sources::Array>(&fgImage.data)) {
                return CreateAssetImage(array->bytes.data(), array->bytes.size(), array->mimeType, fgImage.name);
            }
            if (const auto* vector = std::get_if<fastgltf::sources::Vector>(&fgImage.data)) {
                return CreateAssetImage(vector->bytes.data(), vector->bytes.size(), vector->mimeType, fgImage.name);
            }
            if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&fgImage.data)) {
                auto& bufferView = fgAsset.bufferViews[view->bufferViewIndex];
                auto& buffer = fgAsset.buffers[bufferView.bufferIndex];
                if (const auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                    return CreateAssetImage(
                        array->bytes.data() + bufferView.byteOffset,
                        bufferView.byteLength,
                        view->mimeType,
                        fgImage.name);
                }
            }
            
            return TAssetImage{};
        }();

        auto assetImageId = imageIndex;

        int32_t width = 0;
        int32_t height = 0;
        int32_t components = 0;
        auto pixels = LoadImageFromMemory(
            imageData.EncodedData.get(),
            imageData.EncodedDataSize,
            &width,
            &height,
            &components);

        imageData.Width = width;
        imageData.Height = height;
        imageData.Components = 4; // LoadImageFromMemory is forcing 4 components
        imageData.DecodedData.reset(pixels);
        imageData.Index = assetImageId;

        AddAssetImage(GetSafeResourceName(modelName.data(), imageData.Name.data(), "image", imageIndex), std::move(imageData));
        return static_cast<TAssetImageId>(assetImageId);
    });

    // samplers

    auto samplerIds = std::vector<TAssetSamplerId>(fgAsset.samplers.size());
    const auto samplerIndices = std::ranges::iota_view{(size_t)0, fgAsset.samplers.size()};
    std::transform(poolstl::execution::par, samplerIndices.begin(), samplerIndices.end(), samplerIds.begin(), [&](size_t samplerIndex) {

        PROFILER_ZONESCOPEDN("Load Sampler");
        const fastgltf::Sampler& fgSampler = fgAsset.samplers[samplerIndex];

        const auto getAddressMode = [](const fastgltf::Wrap& wrap) {
            switch (wrap) {
                case fastgltf::Wrap::ClampToEdge: return TTextureAddressMode::ClampToEdge;
                case fastgltf::Wrap::MirroredRepeat: return TTextureAddressMode::RepeatMirrored;
                case fastgltf::Wrap::Repeat: return TTextureAddressMode::Repeat;
                default:
                    return TTextureAddressMode::ClampToEdge;
            }
        };

        //TODO(deccer) reinvent min/mag filterisms, this sucks here
        const auto getMinFilter = [](const fastgltf::Filter& minFilter) {
            switch (minFilter) {
                case fastgltf::Filter::Linear: return TTextureMinFilter::Linear;
                case fastgltf::Filter::LinearMipMapLinear: return TTextureMinFilter::LinearMipmapLinear;
                case fastgltf::Filter::LinearMipMapNearest: return TTextureMinFilter::LinearMipmapNearest;
                case fastgltf::Filter::Nearest: return TTextureMinFilter::Nearest;
                case fastgltf::Filter::NearestMipMapLinear: return TTextureMinFilter::NearestMipmapLinear;
                case fastgltf::Filter::NearestMipMapNearest: return TTextureMinFilter::NearestMipmapNearest;
                default: std::unreachable();
            }
        };

        const auto getMagFilter = [](const fastgltf::Filter& magFilter) {
            switch (magFilter) {
                case fastgltf::Filter::Linear: return TTextureMagFilter::Linear;
                case fastgltf::Filter::Nearest: return TTextureMagFilter::Nearest;
                default: std::unreachable();
            }
        };

        auto assetSampler = TAssetSampler {
            .AddressModeU = getAddressMode(fgSampler.wrapS),
            .AddressModeV = getAddressMode(fgSampler.wrapT),
            .MagFilter = getMagFilter(fgSampler.magFilter.has_value() ? *fgSampler.magFilter : fastgltf::Filter::Nearest),
            .MinFilter = getMinFilter(fgSampler.minFilter.has_value() ? *fgSampler.minFilter : fastgltf::Filter::Nearest),
        };

        auto assetSamplerId = g_assetSamplers.size() + samplerIndex;
        AddAssetSampler(GetSafeResourceName(modelName.data(), fgSampler.name.data(), "sampler", assetSamplerId), assetSampler);
        return static_cast<TAssetSamplerId>(assetSamplerId);
    });

    // textures

    auto assetTextures = std::vector<TAssetTextureId>(fgAsset.textures.size());
    const auto assetTextureIndices = std::ranges::iota_view{(size_t)0, fgAsset.textures.size()};
    for (std::weakly_incrementable auto assetTextureIndex : assetTextureIndices) {

        auto& fgTexture = fgAsset.textures[assetTextureIndex];
        auto textureName = GetSafeResourceName(modelName.data(), fgTexture.name.data(), "texture", assetTextureIndex);

        AddAssetTexture(textureName, TAssetTexture{
            .ImageName = GetSafeResourceName(modelName.data(), nullptr, "image", fgTexture.imageIndex.value_or(0)),
            .SamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0))
        });
    }

    // materials

    auto assetMaterialIds = std::vector<TAssetMaterialId>(fgAsset.materials.size());
    const auto assetMaterialIndices = std::ranges::iota_view{(size_t)0, fgAsset.materials.size()};
    for (std::weakly_incrementable auto assetMaterialIndex : assetMaterialIndices) {

        auto& fgMaterial = fgAsset.materials[assetMaterialIndex];

        CreateAssetMaterial(modelName, fgAsset, fgMaterial, assetMaterialIndex);
    }

    std::stack<std::pair<const fastgltf::Node*, glm::mat4>> nodeStack;
    glm::mat4 rootTransform = glm::mat4(1.0f);

    for (auto nodeIndex : fgAsset.scenes[0].nodeIndices)
    {
        nodeStack.emplace(&fgAsset.nodes[nodeIndex], rootTransform);
    }
   
    while (!nodeStack.empty())
    {
        PROFILER_ZONESCOPEDN("Load Primitive");

        decltype(nodeStack)::value_type top = nodeStack.top();
        const auto& [fgNode, parentGlobalTransform] = top;
        nodeStack.pop();

        glm::mat4 localTransform = GetLocalTransform(*fgNode);
        glm::mat4 globalTransform = parentGlobalTransform * localTransform;

        for (auto childNodeIndex : fgNode->children)
        {
            nodeStack.emplace(&fgAsset.nodes[childNodeIndex], globalTransform);
        }

        if (!fgNode->meshIndex.has_value()) {
            continue;
        }

        auto meshIndex = fgNode->meshIndex.value();
        auto& fgMesh = fgAsset.meshes[meshIndex];

        for (auto primitiveIndex = 0; const auto& fgPrimitive : fgMesh.primitives)
        {
            const auto primitiveMaterialIndex = fgPrimitive.materialIndex.value_or(0);

            auto vertices = GetVertices(fgAsset, fgPrimitive);
            auto indices = GetIndices(fgAsset, fgPrimitive);

            auto meshName = GetSafeResourceName(modelName.data(), fgMesh.name.data(), "mesh", primitiveIndex);

            auto materialIndex = fgPrimitive.materialIndex.value_or(0);
            auto& fgMaterial = fgAsset.materials[materialIndex];
            auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", materialIndex);

            auto assetMesh = CreateAssetMesh(meshName, globalTransform, std::move(vertices), std::move(indices), materialName);
            AddAssetMesh(meshName, assetMesh);
            g_assetModelMeshes[modelName].push_back(meshName);

            primitiveIndex++;
        }
    }
}

// - Renderer -----------------------------------------------------------------

using TGpuMeshId = TId<struct GpuMeshId>;
using TGpuSamplerId = TId<struct GpuSamplerId>;
using TGpuMaterialId = TId<struct GpuMaterialId>;

TIdGenerator<TGpuMeshId> g_gpuMeshCounter = {};

TFramebuffer g_depthPrePassFramebuffer = {};
TGraphicsPipeline g_depthPrePassGraphicsPipeline = {};

TFramebuffer g_geometryFramebuffer = {};
TGraphicsPipeline g_geometryGraphicsPipeline = {};

TFramebuffer g_resolveGeometryFramebuffer = {};
TGraphicsPipeline g_resolveGeometryGraphicsPipeline = {};

std::vector<TGpuGlobalLight> g_gpuGlobalLights;

bool g_drawDebugLines = true;
std::vector<TGpuDebugLine> g_debugLines;
uint32_t g_debugInputLayout = 0;
uint32_t g_debugLinesVertexBuffer = 0;
TGraphicsPipeline g_debugLinesGraphicsPipeline = {};

TFramebuffer g_fxaaFramebuffer = {};
TGraphicsPipeline g_fxaaGraphicsPipeline = {};

TGraphicsPipeline g_fstGraphicsPipeline = {};
TSamplerId g_fstSamplerNearestClampToEdge = {};

uint32_t g_atmosphereSettingsBuffer = 0;

std::unordered_map<std::string, TGpuMesh> g_gpuMeshes = {};
std::unordered_map<std::string, TSampler> g_gpuSamplers = {};
std::unordered_map<std::string, TCpuMaterial> g_cpuMaterials = {};
std::unordered_map<std::string, TGpuMaterial> g_gpuMaterials = {};

auto CreateGpuMesh(const std::string& assetMeshName) -> void {

    auto& assetMesh = GetAssetMesh(assetMeshName);

    uint32_t buffers[3] = {};
    {
        PROFILER_ZONESCOPEDN("Create GL Buffers + Upload Data");

        glCreateBuffers(3, buffers);
        SetDebugLabel(buffers[0], GL_BUFFER, std::format("{}_position", assetMeshName));
        SetDebugLabel(buffers[1], GL_BUFFER, std::format("{}_normal_uv_tangent", assetMeshName));
        SetDebugLabel(buffers[2], GL_BUFFER, std::format("{}_indices", assetMeshName));
        glNamedBufferStorage(buffers[0], sizeof(TGpuVertexPosition) * assetMesh.VertexPositions.size(),
                             assetMesh.VertexPositions.data(), 0);
        glNamedBufferStorage(buffers[1], sizeof(TGpuPackedVertexNormalTangentUvTangentSign) * assetMesh.VertexNormalUvTangents.size(),
                             assetMesh.VertexNormalUvTangents.data(), 0);
        glNamedBufferStorage(buffers[2], sizeof(uint32_t) * assetMesh.Indices.size(), assetMesh.Indices.data(), 0);
    }

    auto gpuMesh = TGpuMesh{
        .Name = assetMeshName,
        .VertexPositionBuffer = buffers[0],
        .VertexNormalUvTangentBuffer = buffers[1],
        .IndexBuffer = buffers[2],

        .VertexCount = assetMesh.VertexPositions.size(),
        .IndexCount = assetMesh.Indices.size(),

        .InitialTransform = assetMesh.InitialTransform,
    };

    {
        PROFILER_ZONESCOPEDN("Add gpu mesh");
        g_gpuMeshes[assetMeshName] = gpuMesh;
    }
}

auto GetGpuMesh(const std::string& assetMeshName) -> TGpuMesh& {
    assert(!assetMeshName.empty() && g_gpuMeshes.contains(assetMeshName));

    return g_gpuMeshes[assetMeshName];
}

auto GetCpuMaterial(const std::string& assetMaterialName) -> TCpuMaterial& {
    assert(!assetMaterialName.empty() && g_cpuMaterials.contains(assetMaterialName));

    return g_cpuMaterials[assetMaterialName];
}

auto GetGpuMaterial(const std::string& assetMaterialName) -> TGpuMaterial& {
    assert(!assetMaterialName.empty() && g_gpuMaterials.contains(assetMaterialName));

    return g_gpuMaterials[assetMaterialName];
}

auto CreateResidentTextureForMaterialChannel(TAssetMaterialChannel& assetMaterialChannel) -> int64_t {

    PROFILER_ZONESCOPEDN("CreateResidentTextureForMaterialChannel");

    auto& image = GetAssetImage(assetMaterialChannel.Image);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(image.Width), static_cast<float>(image.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = image.Name,
    });
    
    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0},
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = image.DecodedData.get()
    });

    GenerateMipmaps(textureId);

    //auto& sampler = GetAssetSampler(assetMaterialChannel.Sampler);

    return MakeTextureResident(textureId);
}

auto CreateTextureForMaterialChannel(TAssetMaterialChannel& assetMaterialChannel) -> uint32_t {

    PROFILER_ZONESCOPEDN("CreateTextureForMaterialChannel");

    auto& image = GetAssetImage(assetMaterialChannel.Image);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(image.Width), static_cast<float>(image.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = image.Name,
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0},
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = image.DecodedData.get()
    });

    GenerateMipmaps(textureId);

    //auto& sampler = GetAssetSampler(assetMaterialChannel.Sampler);

    return GetTexture(textureId).Id;
}

auto CreateGpuMaterial(const std::string& assetMaterialName) -> void {

    PROFILER_ZONESCOPEDN("CreateGpuMaterial");

    auto& assetMaterial = GetAssetMaterial(assetMaterialName);

    uint64_t baseColorTextureHandle = assetMaterial.BaseColorChannel.has_value()
        ? CreateResidentTextureForMaterialChannel(assetMaterial.BaseColorChannel.value())
        : 0;

    uint64_t normalTextureHandle = assetMaterial.NormalsChannel.has_value()
        ? CreateResidentTextureForMaterialChannel(assetMaterial.NormalsChannel.value())
        : 0;

    auto gpuMaterial = TGpuMaterial{
        .BaseColor = assetMaterial.BaseColorFactor,
        .BaseColorTexture = baseColorTextureHandle,
        .NormalTexture = normalTextureHandle
    };

    g_gpuMaterials[assetMaterialName] = gpuMaterial;
}

auto CreateSamplerDescriptor(const TAssetSampler& assetSampler) -> TSamplerDescriptor {

    return TSamplerDescriptor{
        .AddressModeU = assetSampler.AddressModeU,
        .AddressModeV = assetSampler.AddressModeV,
        .MagFilter = assetSampler.MagFilter,
        .MinFilter = assetSampler.MinFilter,
    };
}

auto CreateCpuMaterial(const std::string& assetMaterialName) -> void {

    PROFILER_ZONESCOPEDN("CreateCpuMaterial");

    auto& assetMaterial = GetAssetMaterial(assetMaterialName);

    auto cpuMaterial = TCpuMaterial{
        .BaseColor = assetMaterial.BaseColorFactor,
    };

    auto& baseColorChannel = assetMaterial.BaseColorChannel;
    if (baseColorChannel.has_value()) {
        auto& baseColor = *baseColorChannel;
        auto& baseColorSampler = GetAssetSampler(baseColor.Sampler);

        cpuMaterial.BaseColorTextureId = CreateTextureForMaterialChannel(baseColor);
        auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(baseColorSampler));
        auto& sampler = GetSampler(samplerId);
        cpuMaterial.BaseColorTextureSamplerId = sampler.Id;
    }

    auto& normalTextureChannel = assetMaterial.NormalsChannel;
    if (normalTextureChannel.has_value()) {
        auto& normalTexture = *normalTextureChannel;
        auto& normalTextureSampler = GetAssetSampler(normalTexture.Sampler);

        cpuMaterial.NormalTextureId = CreateTextureForMaterialChannel(normalTexture);
        auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(normalTextureSampler));
        auto& sampler = GetSampler(samplerId);
        cpuMaterial.NormalTextureSamplerId = sampler.Id;
    }

    g_cpuMaterials[assetMaterialName] = cpuMaterial;
}

// - Game ---------------------------------------------------------------------

TCamera g_mainCamera = {};
float g_cameraSpeed = 4.0f;

std::vector<TCpuGlobalLight> g_globalLights;
bool g_globalLightsWasModified = true;

entt::registry g_gameRegistry = {};

// - Scene --------------------------------------------------------------------

struct TComponentTransform : public glm::mat4x4
{
    using glm::mat4x4::mat4x4;
    using glm::mat4x4::operator=;
    TComponentTransform() = default;
    TComponentTransform(glm::mat4x4 const& g) : glm::mat4x4(g) {}
    TComponentTransform(glm::mat4x4&& g) : glm::mat4x4(std::move(g)) {}
};

struct TComponentParent {
    std::vector<entt::entity> Children;
};

struct TComponentChildOf {
    entt::entity Parent;
};

struct TComponentMesh {
    std::string Mesh;
};

struct TComponentMaterial {
    std::string Material;
};

struct TComponentCreateGpuResourcesNecessary {
};

struct TComponentGpuMesh {
    std::string GpuMesh;
};

struct TComponentGpuMaterial {
    std::string GpuMaterial;
};

auto AddEntity(
    std::optional<entt::entity> parent,
    const std::string& assetMeshName,
    const std::string& assetMaterialName,
    glm::mat4x4 initialTransform) -> entt::entity {

    PROFILER_ZONESCOPEDN("AddEntity");

    auto entityId = g_gameRegistry.create();
    if (parent.has_value()) {
        auto parentComponent = g_gameRegistry.get_or_emplace<TComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);
        g_gameRegistry.emplace<TComponentChildOf>(entityId, parent.value());
    }
    g_gameRegistry.emplace<TComponentMesh>(entityId, assetMeshName);
    g_gameRegistry.emplace<TComponentMaterial>(entityId, assetMaterialName);
    g_gameRegistry.emplace<TComponentTransform>(entityId, initialTransform);
    g_gameRegistry.emplace<TComponentCreateGpuResourcesNecessary>(entityId);

    return entityId;
}

auto AddEntity(
    std::optional<entt::entity> parent,
    const std::string& modelName,
    glm::mat4 initialTransform) -> entt::entity {

    PROFILER_ZONESCOPEDN("AddEntity");

    auto entityId = g_gameRegistry.create();
    if (parent.has_value()) {
        auto& parentComponent = g_gameRegistry.get_or_emplace<TComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);

        g_gameRegistry.emplace<TComponentChildOf>(entityId, parent.value());
    }

    if (g_assetModelMeshes.contains(modelName)) {
        auto& modelMeshesNames = g_assetModelMeshes[modelName];
        for(auto& modelMeshName : modelMeshesNames) {

            auto& assetMesh = GetAssetMesh(modelMeshName);
            
            AddEntity(entityId, modelMeshName, assetMesh.MaterialName, initialTransform);
        }
    }

    return entityId;
}

// - Application --------------------------------------------------------------

const glm::vec3 g_unitX = glm::vec3{1.0f, 0.0f, 0.0f};
const glm::vec3 g_unitY = glm::vec3{0.0f, 1.0f, 0.0f};
const glm::vec3 g_unitZ = glm::vec3{0.0f, 0.0f, 1.0f};

constexpr ImVec2 g_imvec2UnitX = ImVec2(1, 0);
constexpr ImVec2 g_imvec2UnitY = ImVec2(0, 1);

GLFWwindow* g_window = nullptr;
ImGuiContext* g_imguiContext = nullptr;

bool g_isEditor = false;
bool g_sleepWhenWindowHasNoFocus = true;
bool g_windowHasFocus = false;
bool g_cursorJustEntered = false;
bool g_cursorIsActive = true;
float g_cursorSensitivity = 0.0025f;

glm::dvec2 g_cursorPosition = {};
glm::dvec2 g_cursorFrameOffset = {};

glm::ivec2 g_windowFramebufferSize = {};
glm::ivec2 g_windowFramebufferScaledSize = {};
bool g_windowFramebufferResized = false;

glm::ivec2 g_sceneViewerSize = {};
glm::ivec2 g_sceneViewerScaledSize = {};
bool g_sceneViewerResized = false;

bool g_isFxaaEnabled = false;

// - Implementation -----------------------------------------------------------

auto OnWindowKey(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t key,
    [[maybe_unused]] int32_t scancode,
    const int32_t action,
    [[maybe_unused]] int32_t mods) -> void {

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        g_isEditor = !g_isEditor;
        g_windowFramebufferResized = !g_isEditor;
        g_sceneViewerResized = g_isEditor;
    }
}

auto OnWindowCursorEntered(
    [[maybe_unused]] GLFWwindow* window,
    int entered) -> void {

    if (entered) {
        g_cursorJustEntered = true;
    }

    g_windowHasFocus = entered == GLFW_TRUE;
}

auto OnWindowCursorPosition(
    [[maybe_unused]] GLFWwindow* window,
    double cursorPositionX,
    double cursorPositionY) -> void {

    if (g_cursorJustEntered)
    {
        g_cursorPosition = {cursorPositionX, cursorPositionY};
        g_cursorJustEntered = false;
    }        

    g_cursorFrameOffset += glm::dvec2{
        cursorPositionX - g_cursorPosition.x, 
        g_cursorPosition.y - cursorPositionY};
    g_cursorPosition = {cursorPositionX, cursorPositionY};
}

auto OnWindowFramebufferSizeChanged(
    [[maybe_unused]] GLFWwindow* window,
    const int width,
    const int height) -> void {

    g_windowFramebufferSize = glm::ivec2{width, height};
    g_windowFramebufferResized = true;
}

auto OnOpenGLDebugMessage(
    [[maybe_unused]] uint32_t source,
    uint32_t type,
    [[maybe_unused]] uint32_t id,
    [[maybe_unused]] uint32_t severity, 
    [[maybe_unused]] int32_t length,
    const char* message,
    [[maybe_unused]] const void* userParam) -> void {

    if (type == GL_DEBUG_TYPE_ERROR) {
        spdlog::error(message);
        debug_break();
    }
}

auto HandleCamera(float deltaTime) -> void {

    g_cursorIsActive = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE;
    glfwSetInputMode(g_window, GLFW_CURSOR, g_cursorIsActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (!g_cursorIsActive) {
        glfwSetCursorPos(g_window, 0, 0);
        g_cursorPosition.x = 0;
        g_cursorPosition.y = 0;
    }

    const auto forward = g_mainCamera.GetForwardDirection();
    const auto right = glm::normalize(glm::cross(forward, g_unitY));

    auto tempCameraSpeed = g_cameraSpeed;
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        tempCameraSpeed *= 4.0f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        tempCameraSpeed *= 40.0f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        tempCameraSpeed *= 0.25f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) {
        g_mainCamera.Position += forward * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) {
        g_mainCamera.Position -= forward * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) {
        g_mainCamera.Position += right * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) {
        g_mainCamera.Position -= right * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS) {
        g_mainCamera.Position.y -= deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS) {
        g_mainCamera.Position.y += deltaTime * tempCameraSpeed;
    }

    if (!g_cursorIsActive) {

        g_mainCamera.Yaw += static_cast<float>(g_cursorFrameOffset.x * g_cursorSensitivity);
        g_mainCamera.Pitch += static_cast<float>(g_cursorFrameOffset.y * g_cursorSensitivity);
        g_mainCamera.Pitch = glm::clamp(g_mainCamera.Pitch, -glm::half_pi<float>() + 1e-4f, glm::half_pi<float>() - 1e-4f);    
    }

    g_cursorFrameOffset = {0.0, 0.0};
}

auto DeleteRendererFramebuffers() -> void {

    DeleteFramebuffer(g_depthPrePassFramebuffer);
    DeleteFramebuffer(g_geometryFramebuffer);
    DeleteFramebuffer(g_resolveGeometryFramebuffer);
    DeleteFramebuffer(g_fxaaFramebuffer);
}

auto CreateRendererFramebuffers(const glm::vec2& scaledFramebufferSize) -> void {

    PROFILER_ZONESCOPEDN("CreateRendererFramebuffers");

    g_depthPrePassFramebuffer = CreateFramebuffer({
        .Label = "Depth PrePass",
        .DepthStencilAttachment = TFramebufferDepthStencilAttachmentDescriptor{
            .Label = "Depth",
            .Format = TFormat::D24_UNORM_S8_UINT,
            .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
            .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
            .ClearDepthStencil = { 1.0f, 0 },
        }
    });

    g_geometryFramebuffer = CreateFramebuffer({
        .Label = "Geometry",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "GeometryAlbedo",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = "GeometryNormals",
                .Format = TFormat::R32G32B32A32_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        },
        .DepthStencilAttachment = TFramebufferExistingDepthStencilAttachmentDescriptor{
            .ExistingDepthTexture = g_depthPrePassFramebuffer.DepthStencilAttachment.value().Texture,
        }
    });

    g_resolveGeometryFramebuffer = CreateFramebuffer({
        .Label = "ResolveGeometry",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "ResolvedGeometry",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        },
    });

    g_fxaaFramebuffer = CreateFramebuffer({
        .Label = "PostFX-FXAA",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "PostFX-FXAA-Buffer",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f }
            }
        }
    });
}

struct TAtmosphereSettings {
    glm::vec4 SunPositionAndPlanetRadius;
    glm::vec4 RayOriginAndSunIntensity;
    glm::vec4 RayleighScatteringCoefficientAndAtmosphereRadius;
    float MieScatteringCoefficient;
    float MieScaleHeight;
    float MiePreferredScatteringDirection;
    float RayleighScaleHeight;
};

glm::vec3 g_atmosphereSunPosition = {10000.0f, 6400.0f, 10000.0f};
float g_atmospherePlanetRadius = 6371.0f;
glm::vec3 g_atmosphereRayOrigin = {0.0f, 6372.0f, 0.0f};
float g_atmosphereSunIntensity = 22.0f;
glm::vec3 g_atmosphereRayleighScatteringCoefficient = {5.5e-6, 13.0e-6, 22.4e-6};
float g_atmosphereAtmosphereRadius = 6471.0f;
float g_atmosphereMieScatteringCoefficient = 21e-6;
float g_atmosphereMieScaleHeight = 1.2e3;
float g_atmosphereMiePreferredScatteringDirection = 0.758;
float g_atmosphereRayleighScaleHeight = 8e3;

auto main(
    [[maybe_unused]] int32_t argc,
    [[maybe_unused]] char* argv[]) -> int32_t {

    PROFILER_ZONESCOPEDN("main");
    auto previousTimeInSeconds = glfwGetTime();

    /*
    AddAssetFromFile("fform1", "data/basic/fform_1.glb");
    AddAssetFromFile("fform2", "data/basic/fform_2.glb");
    AddAssetFromFile("fform3", "data/basic/fform_3.glb");
    AddAssetFromFile("fform4", "data/basic/fform_4.glb");
    AddAssetFromFile("fform5", "data/basic/fform_5.glb");
    AddAssetFromFile("fform6", "data/basic/fform_6.glb");
    AddAssetFromFile("fform7", "data/basic/fform_7.glb");
    AddAssetFromFile("fform8", "data/basic/fform_8.glb");
    AddAssetFromFile("fform9", "data/basic/fform_9.glb");
    AddAssetFromFile("fform10", "data/basic/fform_10.glb");
     */

    TWindowSettings windowSettings = {
        .ResolutionWidth = 1920,
        .ResolutionHeight = 1080,
        .ResolutionScale = 1.0f,
        .WindowStyle = TWindowStyle::Windowed,
        .IsDebug = true,
        .IsVSyncEnabled = false,
    };

    if (glfwInit() == GLFW_FALSE) {
        spdlog::error("Glfw: Unable to initialize");
        return 0;
    }

    /*
     * Setup Application
     */
    const auto isWindowWindowed = windowSettings.WindowStyle == TWindowStyle::Windowed;
    glfwWindowHint(GLFW_DECORATED, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (windowSettings.IsDebug) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    const auto primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        spdlog::error("Glfw: Unable to get the primary monitor");
        return 0;
    }

    const auto primaryMonitorVideoMode = glfwGetVideoMode(primaryMonitor);
    const auto screenWidth = primaryMonitorVideoMode->width;
    const auto screenHeight = primaryMonitorVideoMode->height;

    const auto windowWidth = windowSettings.ResolutionWidth;
    const auto windowHeight = windowSettings.ResolutionHeight;

    auto monitor = windowSettings.WindowStyle == TWindowStyle::FullscreenExclusive
        ? primaryMonitor
        : nullptr;

    g_window = glfwCreateWindow(windowWidth, windowHeight, "How To Script", monitor, nullptr);
    if (g_window == nullptr) {
        spdlog::error("Glfw: Unable to create a window");
        return 0;
    }

    int32_t monitorLeft = 0;
    int32_t monitorTop = 0;
    glfwGetMonitorPos(primaryMonitor, &monitorLeft, &monitorTop);
    if (isWindowWindowed) {
        glfwSetWindowPos(g_window, screenWidth / 2 - windowWidth / 2 + monitorLeft, screenHeight / 2 - windowHeight / 2 + monitorTop);
    } else {
        glfwSetWindowPos(g_window, monitorLeft, monitorTop);
    }

    glfwSetKeyCallback(g_window, OnWindowKey);
    glfwSetCursorPosCallback(g_window, OnWindowCursorPosition);
    glfwSetCursorEnterCallback(g_window, OnWindowCursorEntered);    
    glfwSetFramebufferSizeCallback(g_window, OnWindowFramebufferSizeChanged);
    glfwSetInputMode(g_window, GLFW_CURSOR, g_cursorIsActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    int32_t windowFramebufferWidth = 0;
    int32_t windowFramebufferHeight = 0;
    glfwGetFramebufferSize(g_window, &windowFramebufferWidth, &windowFramebufferHeight);
    OnWindowFramebufferSizeChanged(g_window, windowFramebufferWidth, windowFramebufferHeight);

    glfwMakeContextCurrent(g_window);
    if (!RendererInitialize()) {
        spdlog::error("Glad: Unable to initialize");
        return 0;
    }

    if (windowSettings.IsDebug) {
        glDebugMessageCallback(OnOpenGLDebugMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

#ifdef USE_PROFILER
    TracyGpuContext
#endif

    g_imguiContext = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // this little shit doesn't do anything
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    constexpr auto fontSize = 18.0f;

    io.Fonts->AddFontFromFileTTF("data/fonts/HurmitNerdFont-Regular.otf", fontSize);
    {
        constexpr float iconFontSize = fontSize * 4.0f / 5.0f;
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        icons_config.GlyphOffset.y = 0; // Hack to realign the icons
        io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_FAR, iconFontSize, &icons_config, icons_ranges);
    }
    {
        constexpr float iconFontSize = fontSize;
        static const ImWchar icons_ranges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        icons_config.GlyphOffset.y = 4; // Hack to realign the icons
        io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_MD, iconFontSize, &icons_config, icons_ranges);
    }

    auto& imguiStyle = ImGui::GetStyle();
    imguiStyle.WindowMenuButtonPosition = ImGuiDir_None;
    imguiStyle.DisabledAlpha = 0.3f;
    imguiStyle.PopupRounding = 0;
    imguiStyle.WindowPadding = ImVec2(4, 4);
    imguiStyle.FramePadding = ImVec2(6, 4);
    imguiStyle.ItemSpacing = ImVec2(6, 2);
    imguiStyle.ScrollbarSize = 18;
    imguiStyle.WindowBorderSize = 1;
    imguiStyle.ChildBorderSize = 1;
    imguiStyle.PopupBorderSize = 1;
    imguiStyle.FrameBorderSize = 1;
    imguiStyle.WindowRounding = 0;
    imguiStyle.ChildRounding = 0;
    imguiStyle.FrameRounding = 0;
    imguiStyle.ScrollbarRounding = 0;
    imguiStyle.GrabRounding = 0;
    imguiStyle.AntiAliasedFill = true;
    imguiStyle.AntiAliasedLines = true;

    auto* colors = imguiStyle.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.90f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.42f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.32f, 0.32f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.12f, 0.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.67f, 0.60f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.46f, 0.24f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.69f, 0.30f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.00f, 0.96f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    if (!ImGui_ImplGlfw_InitForOpenGL(g_window, true)) {
        spdlog::error("ImGui: Unable to initialize the GLFW backend");
        return 0;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        spdlog::error("ImGui: Unable to initialize the OpenGL backend");
        return 0;
    }

    glfwSwapInterval(windowSettings.IsVSyncEnabled ? 1 : 0);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.05f, 0.07f, 1.0f);

    std::vector<float> frameTimes;
    frameTimes.resize(512);
    std::fill_n(frameTimes.begin(), frameTimes.size(), 60.0f);

    auto isSrgbDisabled = false;
    auto isCullfaceDisabled = false;

    g_sceneViewerSize = g_windowFramebufferSize;
    auto scaledFramebufferSize = glm::vec2(g_sceneViewerSize) * windowSettings.ResolutionScale;

    /*
     * Renderer - Initialize Framebuffers
     */
    CreateRendererFramebuffers(scaledFramebufferSize);

    /*
     * Renderer - Initialize Pipelines
     */
    auto depthPrePassGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Depth PrePass",
        .VertexShaderFilePath = "data/shaders/DepthPrePass.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/Depth.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
            .IsDepthBiasEnabled = false,
            .IsScissorEnabled = false,
            .IsRasterizerDisabled = false
        },
        .OutputMergerState = {
            .ColorMask = TColorMaskBits::None,
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = true,
                .DepthFunction = TDepthFunction::Less
            },
        }
    });
    g_depthPrePassGraphicsPipeline = *depthPrePassGraphicsPipelineResult;

    auto geometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "GeometryPipeline",
        .VertexShaderFilePath = "data/shaders/SimpleDeferred.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/SimpleDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = false,
                .DepthFunction = TDepthFunction::Equal,
            }
        }
    });
    if (!geometryGraphicsPipelineResult) {
        spdlog::error(geometryGraphicsPipelineResult.error());
        return -1;
    }
    g_geometryGraphicsPipeline = *geometryGraphicsPipelineResult;

    auto resolveGeometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "ResolveGeometryPipeline",
        .VertexShaderFilePath = "data/shaders/ResolveDeferred.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/ResolveDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
            }
        }
    });
    if (!resolveGeometryGraphicsPipelineResult) {
        spdlog::error(resolveGeometryGraphicsPipelineResult.error());
        return -1;
    }
    g_resolveGeometryGraphicsPipeline = *resolveGeometryGraphicsPipelineResult;

    auto fstGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FST",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.GammaCorrected.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles
        },
    });

    if (!fstGraphicsPipelineResult) {
        spdlog::error(fstGraphicsPipelineResult.error());
        return 0;
    }
    g_fstGraphicsPipeline = *fstGraphicsPipelineResult;

    auto debugLinesGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "DebugLinesPipeline",
        .VertexShaderFilePath = "data/shaders/DebugLines.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/DebugLines.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Lines,
            .IsPrimitiveRestartEnabled = false,
        },
        .VertexInput = TVertexInputDescriptor{
            .VertexInputAttributes = {
                TVertexInputAttributeDescriptor{
                    .Location = 0,
                    .Binding = 0,
                    .Format = TFormat::R32G32B32_FLOAT,
                    .Offset = offsetof(TGpuDebugLine, StartPosition),
                },
                TVertexInputAttributeDescriptor{
                    .Location = 1,
                    .Binding = 0,
                    .Format = TFormat::R32G32B32A32_FLOAT,
                    .Offset = offsetof(TGpuDebugLine, StartColor),
                },
            }
        }
    });

    if (!debugLinesGraphicsPipelineResult) {
        spdlog::error(debugLinesGraphicsPipelineResult.error());
        return 0;
    }
    g_debugLinesGraphicsPipeline = *debugLinesGraphicsPipelineResult;

    auto fxaaGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FXAA",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
    });
    if (!fxaaGraphicsPipelineResult) {
        spdlog::error(fxaaGraphicsPipelineResult.error());
        return 0;
    }

    g_fxaaGraphicsPipeline = *fxaaGraphicsPipelineResult;

    g_debugLinesVertexBuffer = CreateBuffer("VertexBuffer-DebugLines", sizeof(TGpuDebugLine) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    auto cameraPosition = glm::vec3{0.0f, 0.0f, 20.0f};
    auto cameraDirection = glm::vec3{0.0f, 0.0f, -1.0f};
    auto cameraUp = glm::vec3{0.0f, 1.0f, 0.0f};
    auto fieldOfView = glm::radians(70.0f);
    auto aspectRatio = scaledFramebufferSize.x / static_cast<float>(scaledFramebufferSize.y);
    TGpuGlobalUniforms globalUniforms = {
        .ProjectionMatrix = glm::infinitePerspective(fieldOfView, aspectRatio, 0.1f),
        .ViewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp),
        .CameraPosition = glm::vec4{cameraPosition, fieldOfView},
        .CameraDirection = glm::vec4{cameraDirection, aspectRatio}
    };
    auto globalUniformsBuffer = CreateBuffer("TGpuGlobalUniforms", sizeof(TGpuGlobalUniforms), &globalUniforms, GL_DYNAMIC_STORAGE_BIT);
    auto objectsBuffer = CreateBuffer("TGpuObjects", sizeof(TGpuObject) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    g_gpuGlobalLights.push_back(TGpuGlobalLight{
        .ProjectionMatrix = glm::mat4(1.0f),
        .ViewMatrix = glm::mat4(1.0f),
        .Direction = glm::vec4(10.0f, 20.0f, 10.0f, 0.0f),
        .Strength = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
    });

    auto globalLightsBuffer = CreateBuffer("SGpuGlobalLights", g_gpuGlobalLights.size() * sizeof(TGpuGlobalLight), g_gpuGlobalLights.data(), GL_DYNAMIC_STORAGE_BIT);

    g_fstSamplerNearestClampToEdge = GetOrCreateSampler({
       .AddressModeU = TTextureAddressMode::ClampToEdge,
       .AddressModeV = TTextureAddressMode::ClampToEdge,
       .MagFilter = TTextureMagFilter::Nearest,
       .MinFilter = TTextureMinFilter::NearestMipmapNearest
    });

    /*
     * Load Assets
     */

    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/Sponza/glTF/Sponza.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/_Random/SM_Cube_OneMaterialPerFace.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Downloads/modular_ruins_c/modular_ruins_c.glb");
    LoadModelFromFile("Test", "data/default/SM_Deccer_Cubes_Textured.glb");
    //LoadModelFromFile("SM_Tower", "data/scenes/Tower/scene.gltf");
    
    //LoadModelFromFile("SM_DeccerCube", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube1", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube_1by1by1", "data/basic/SM_Cube_1by1by1.glb");
    
    //LoadModelFromFile("SM_Cube_1by1_x5", "data/basic/SM_Cube_1by1_x5.glb");
    //LoadModelFromFile("SM_Cube_1by1_y5", "data/basic/SM_Cube_1by1_y5.glb");
    //LoadModelFromFile("SM_Cube_1by1_z5", "data/basic/SM_Cube_1by1_z5.glb");
    //LoadModelFromFile("Test", "data/basic/Cubes/SM_Cube_1by1_5y.gltf");
    //LoadModelFromFile("Test2", "data/basic/Cubes2/SM_Cube_1by1_5y.gltf");
    
    //LoadModelFromFile("SM_IntelSponza", "data/scenes/IntelSponza/NewSponza_Main_glTF_002.gltf");

    uint32_t g_gridTexture;

    int32_t iconWidth = 0;
    int32_t iconHeight = 0;
    int32_t iconComponents = 0;

    auto gridTexturePixels = LoadImageFromFile("data/basic/T_Grid_1024_D.png", &iconWidth, &iconHeight, &iconComponents);
    if (gridTexturePixels != nullptr) {
        glCreateTextures(GL_TEXTURE_2D, 1, &g_gridTexture);
        auto mipLevels = static_cast<int32_t>(1 + glm::floor(glm::log2(glm::max(static_cast<float>(iconWidth), static_cast<float>(iconHeight)))));
        glTextureStorage2D(g_gridTexture, mipLevels, GL_SRGB8_ALPHA8, iconWidth, iconHeight);
        glTextureSubImage2D(g_gridTexture, 0, 0, 0, iconWidth, iconHeight, GL_RGBA, GL_UNSIGNED_BYTE, gridTexturePixels);
        glGenerateTextureMipmap(g_gridTexture);
        FreeImage(gridTexturePixels);
    }
    
    /// Setup Scene ////////////

    AddEntity(std::nullopt, "Test", glm::mat4(1.0f));

    //AddEntity(std::nullopt, "Test2", glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -10.0f}));
    //AddEntity(std::nullopt, "SM_Tower", glm::scale(glm::mat4(1.0f), {0.01f, 0.01f, 0.01f}) * glm::translate(glm::mat4(1.0f), {-5.0f, -2.0f, -10.0f}));
    //AddEntity(std::nullopt, "SM_Cube1", glm::translate(glm::mat4(1.0f), {0.0f, 10.0f, 10.0f}));
    //AddEntity(std::nullopt, "SM_IntelSponza", glm::translate(glm::mat4(1.0f), {0.0f, -10.0f, 0.0f}));

    /// Start Render Loop ///////

    uint64_t frameCounter = 0;
    auto accumulatedTimeInSeconds = 0.0;
    auto averageFramesPerSecond = 0.0f;

    auto atmosphereSettings = TAtmosphereSettings{
        .SunPositionAndPlanetRadius = glm::vec4(g_atmosphereSunPosition, g_atmospherePlanetRadius),
        .RayOriginAndSunIntensity = glm::vec4(g_atmosphereRayOrigin, g_atmosphereSunIntensity),
        .RayleighScatteringCoefficientAndAtmosphereRadius = glm::vec4(g_atmosphereRayleighScatteringCoefficient, g_atmosphereAtmosphereRadius),
        .MieScatteringCoefficient = g_atmosphereMieScatteringCoefficient,
        .MieScaleHeight = g_atmosphereMieScaleHeight,
        .MiePreferredScatteringDirection = g_atmosphereMiePreferredScatteringDirection,
        .RayleighScaleHeight = g_atmosphereRayleighScaleHeight
    };
    g_atmosphereSettingsBuffer = CreateBuffer("AtmosphereSettings", sizeof(TAtmosphereSettings), &atmosphereSettings, GL_DYNAMIC_STORAGE_BIT);

    auto updateInterval = 1.0f;
    while (!glfwWindowShouldClose(g_window)) {

        PROFILER_ZONESCOPEDN("Frame");
        auto currentTimeInSeconds = glfwGetTime();
        auto deltaTimeInSeconds = currentTimeInSeconds - previousTimeInSeconds;
        auto framesPerSecond = 1.0f / deltaTimeInSeconds;
        accumulatedTimeInSeconds += deltaTimeInSeconds;

        frameTimes[frameCounter % frameTimes.size()] = framesPerSecond;
        if (accumulatedTimeInSeconds >= updateInterval) {
            auto sum = 0.0f;
            for (auto i = 0; i < frameTimes.size(); i++) {
                sum += frameTimes[i];
            }
            averageFramesPerSecond = sum / frameTimes.size();
            accumulatedTimeInSeconds = 0.0f;
        }

        ///////////////////////
        // Create Gpu Resources if necessary
        ///////////////////////
        auto createGpuResourcesNecessaryView = g_gameRegistry.view<TComponentCreateGpuResourcesNecessary>();
        for (auto& entity : createGpuResourcesNecessaryView) {

            PROFILER_ZONESCOPEDN("Create Gpu Resources");
            auto& meshComponent = g_gameRegistry.get<TComponentMesh>(entity);
            auto& materialComponent = g_gameRegistry.get<TComponentMaterial>(entity);

            CreateGpuMesh(meshComponent.Mesh);
            CreateCpuMaterial(materialComponent.Material);

            g_gameRegistry.emplace<TComponentGpuMesh>(entity, meshComponent.Mesh);
            g_gameRegistry.emplace<TComponentGpuMaterial>(entity, materialComponent.Material);

            g_gameRegistry.remove<TComponentCreateGpuResourcesNecessary>(entity);
        }

        // Resize if necessary
        if (g_windowFramebufferResized || g_sceneViewerResized) {

            PROFILER_ZONESCOPEDN("Resized");

            g_windowFramebufferScaledSize = glm::ivec2{g_windowFramebufferSize.x * windowSettings.ResolutionScale, g_windowFramebufferSize.y * windowSettings.ResolutionScale};
            g_sceneViewerScaledSize = glm::ivec2{g_sceneViewerSize.x * windowSettings.ResolutionScale, g_sceneViewerSize.y * windowSettings.ResolutionScale};

            if (g_isEditor) {
                scaledFramebufferSize = g_sceneViewerScaledSize;
            } else {
                scaledFramebufferSize = g_windowFramebufferScaledSize;
            }

            if ((scaledFramebufferSize.x * scaledFramebufferSize.y) > 0.0f) {
                DeleteRendererFramebuffers();
                CreateRendererFramebuffers(scaledFramebufferSize);

                g_windowFramebufferResized = false;
                g_sceneViewerResized = false;

            }
        }

        // Update State
        HandleCamera(static_cast<float>(deltaTimeInSeconds));

        // Update Per Frame Uniforms
        fieldOfView = glm::radians(70.0f);
        aspectRatio = scaledFramebufferSize.x / static_cast<float>(scaledFramebufferSize.y);
        globalUniforms.ProjectionMatrix = glm::infinitePerspective(fieldOfView, aspectRatio, 0.1f);
        globalUniforms.ViewMatrix = g_mainCamera.GetViewMatrix();
        globalUniforms.CameraPosition = glm::vec4(g_mainCamera.Position, fieldOfView);
        globalUniforms.CameraDirection = glm::vec4(g_mainCamera.GetForwardDirection(), aspectRatio);
        UpdateBuffer(globalUniformsBuffer, 0, sizeof(TGpuGlobalUniforms), &globalUniforms);

        //
        // CLEAR DEBUG LINES
        //
        if (g_drawDebugLines) {
            g_debugLines.clear();

            g_debugLines.push_back(TGpuDebugLine{
                .StartPosition = glm::vec3{-150, 30, 4},
                .StartColor = glm::vec4{0.3f, 0.95f, 0.1f, 1.0f},
                .EndPosition = glm::vec3{150, -40, -4},
                .EndColor = glm::vec4{0.6f, 0.1f, 0.0f, 1.0f}
            });
        }

        if (isSrgbDisabled) {
            glEnable(GL_FRAMEBUFFER_SRGB);
            isSrgbDisabled = false;
        }

        {
            PROFILER_ZONESCOPEDN("All Depth PrePass Geometry");
            PushDebugGroup("Depth PrePass");
            BindFramebuffer(g_depthPrePassFramebuffer);
            {
                g_depthPrePassGraphicsPipeline.Bind();
                g_depthPrePassGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);

                const auto& renderablesView = g_gameRegistry.view<TComponentGpuMesh, TComponentTransform>();
                renderablesView.each([](
                    const auto& meshComponent,
                    const auto& transformComponent) {

                    PROFILER_ZONESCOPEDN("Draw PrePass Geometry");

                    auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                    g_depthPrePassGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                    g_depthPrePassGraphicsPipeline.SetUniform(0, transformComponent * gpuMesh.InitialTransform);

                    g_depthPrePassGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
                });
            }
            PopDebugGroup();
        }
        {
            PROFILER_ZONESCOPEDN("Draw Geometry All");
            PushDebugGroup("Geometry");
            BindFramebuffer(g_geometryFramebuffer);
            {
                g_geometryGraphicsPipeline.Bind();
                g_geometryGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);

                const auto& renderablesView = g_gameRegistry.view<TComponentGpuMesh, TComponentGpuMaterial, TComponentTransform>();
                renderablesView.each([](
                    const auto& meshComponent,
                    const auto& materialComponent,
                    const auto& transformComponent) {

                    PROFILER_ZONESCOPEDN("Draw Geometry");

                    auto& cpuMaterial = GetCpuMaterial(materialComponent.GpuMaterial);
                    auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                    g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                    g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexNormalUvTangentBuffer, 2);
                    g_geometryGraphicsPipeline.SetUniform(0, transformComponent * gpuMesh.InitialTransform);

                    g_geometryGraphicsPipeline.BindTextureAndSampler(8, cpuMaterial.BaseColorTextureId,
                                                                     cpuMaterial.BaseColorTextureSamplerId);
                    g_geometryGraphicsPipeline.BindTextureAndSampler(9, cpuMaterial.NormalTextureId, cpuMaterial.NormalTextureSamplerId);

                    g_geometryGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
                });
            }
            PopDebugGroup();
        }
        {
            PROFILER_ZONESCOPEDN("Draw ResolveGeometry");
            PushDebugGroup("ResolveGeometry");
            BindFramebuffer(g_resolveGeometryFramebuffer);
            {
                g_resolveGeometryGraphicsPipeline.Bind();
                //auto& samplerId = g_samplers[size_t(g_fullscreenSamplerNearestClampToEdge.Id)]
                g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);
                g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(globalLightsBuffer, 2);
                g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(g_atmosphereSettingsBuffer, 3);
                g_resolveGeometryGraphicsPipeline.BindTexture(0, g_geometryFramebuffer.ColorAttachments[0]->Texture.Id);
                g_resolveGeometryGraphicsPipeline.BindTexture(1, g_geometryFramebuffer.ColorAttachments[1]->Texture.Id);
                g_resolveGeometryGraphicsPipeline.BindTexture(2, g_depthPrePassFramebuffer.DepthStencilAttachment->Texture.Id);

                g_resolveGeometryGraphicsPipeline.SetUniform(0, g_mainCamera.GetForwardDirection());

                g_resolveGeometryGraphicsPipeline.DrawArrays(0, 3);
            }
            PopDebugGroup();
        }

        /////////////// Debug Lines // move out to some LineRenderer

        if (g_drawDebugLines && !g_debugLines.empty()) {

            PROFILER_ZONESCOPEDN("Draw DebugLines");

            PushDebugGroup("Debug Lines");
            glDisable(GL_CULL_FACE);

            UpdateBuffer(g_debugLinesVertexBuffer, 0, sizeof(TGpuDebugLine) * g_debugLines.size(), g_debugLines.data());

            g_debugLinesGraphicsPipeline.Bind();
            g_debugLinesGraphicsPipeline.BindBufferAsVertexBuffer(g_debugLinesVertexBuffer, 0, 0, sizeof(TGpuDebugLine) / 2);
            g_debugLinesGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);
            g_debugLinesGraphicsPipeline.DrawArrays(0, g_debugLines.size() * 2);

            glEnable(GL_CULL_FACE);
            PopDebugGroup();
        }

        if (g_isFxaaEnabled) {
            PROFILER_ZONESCOPEDN("PostFX FXAA");
            PushDebugGroup("PostFX FXAA");
            BindFramebuffer(g_fxaaFramebuffer);
            {
                g_fxaaGraphicsPipeline.Bind();
                g_fxaaGraphicsPipeline.BindTexture(0, g_resolveGeometryFramebuffer.ColorAttachments[0]->Texture.Id);
                g_fxaaGraphicsPipeline.SetUniform(1, 1);
                g_fxaaGraphicsPipeline.DrawArrays(0, 3);
            }
            PopDebugGroup();
        }

        /////////////// UI

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (g_isEditor) {
            glClear(GL_COLOR_BUFFER_BIT);
        } else {
            glBlitNamedFramebuffer(g_isFxaaEnabled
                ? g_fxaaFramebuffer.Id
                : g_resolveGeometryFramebuffer.Id,
                0,
                0, 0, static_cast<int32_t>(scaledFramebufferSize.x), static_cast<int32_t>(scaledFramebufferSize.y),
                0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (g_isEditor) {
            ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
        }

        if (!g_isEditor) {
            ImGui::SetNextWindowPos({32, 32});
            ImGui::SetNextWindowSize({168, 192});
            auto windowBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            windowBackgroundColor.w = 0.4f;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
            if (ImGui::Begin("#InGameStatistics", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoDecoration)) {

                ImGui::TextColored(ImVec4{0.9f, 0.7f, 0.0f, 1.0f}, "F1 to toggle editor");
                ImGui::SeparatorText("Frame Statistics");

                ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * framesPerSecond);
                ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * framesPerSecond));
                ImGui::Text("mfps: %.0f", averageFramesPerSecond);
                ImGui::Text("rfps: %.0f", framesPerSecond);
                ImGui::Text("rpms: %.0f", framesPerSecond * 60.0f);
                ImGui::Text("  ft: %.2f ms", deltaTimeInSeconds * 1000.0f);
                ImGui::Text("   f: %lu", frameCounter);
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        if (ImGui::Begin(ICON_MD_SUNNY " Atmosphere")) {

            auto atmosphereModified = false;
            if (ImGui::SliderFloat3("Sun Position", glm::value_ptr(g_atmosphereSunPosition), -10000.0f, 10000.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Planet Radius", &g_atmospherePlanetRadius, 1.0f, 7000.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat3("Ray Origin", glm::value_ptr(g_atmosphereRayOrigin), -10000.0f, 10000.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Sun Intensity", &g_atmosphereSunIntensity, 0.1f, 40.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat3("Coefficients", glm::value_ptr(g_atmosphereRayleighScatteringCoefficient), 0.0f, 0.01f, "%.5f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Mie Scattering Coeff", &g_atmosphereMieScatteringCoefficient, 0.001f, 0.1f, "%.5f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Mie Scale Height", &g_atmosphereMieScaleHeight, 0.01f, 6000.0f, "%.2f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Mie Scattering Direction", &g_atmosphereMiePreferredScatteringDirection, -1.0f, 1.0f, "%.001f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Rayleigh Scale Height", &g_atmosphereRayleighScaleHeight, -1000, 1000, "%0.01f")) {
                atmosphereModified = true;
            }

            if (atmosphereModified) {

                atmosphereSettings = TAtmosphereSettings{
                    .SunPositionAndPlanetRadius = glm::vec4(g_atmosphereSunPosition, g_atmospherePlanetRadius),
                    .RayOriginAndSunIntensity = glm::vec4(g_atmosphereRayOrigin, g_atmosphereSunIntensity),
                    .RayleighScatteringCoefficientAndAtmosphereRadius = glm::vec4(g_atmosphereRayleighScatteringCoefficient, g_atmosphereAtmosphereRadius),
                    .MieScatteringCoefficient = g_atmosphereMieScatteringCoefficient,
                    .MieScaleHeight = g_atmosphereMieScaleHeight,
                    .MiePreferredScatteringDirection = g_atmosphereMiePreferredScatteringDirection,
                    .RayleighScaleHeight = g_atmosphereRayleighScaleHeight
                };
                UpdateBuffer(g_atmosphereSettingsBuffer, 0, sizeof(TAtmosphereSettings), &atmosphereSettings);

                atmosphereModified = false;
            }
        }
        ImGui::End();

        if (g_isEditor) {
            if (ImGui::BeginMainMenuBar()) {
                //ImGui::Image(reinterpret_cast<ImTextureID>(g_iconPackageGreen), ImVec2(16, 16), g_imvec2UnitY, g_imvec2UnitX);
                ImGui::SetCursorPosX(20.0f);
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Quit")) {

                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            /*
             * UI - Scene Viewer
             */
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if (ImGui::Begin(ICON_MD_GRID_VIEW "Scene")) {
                auto currentSceneWindowSize = ImGui::GetContentRegionAvail();
                if ((currentSceneWindowSize.x != g_sceneViewerSize.x || currentSceneWindowSize.y != g_sceneViewerSize.y)) {
                    g_sceneViewerSize = glm::ivec2(currentSceneWindowSize.x, currentSceneWindowSize.y);
                    g_sceneViewerResized = true;
                }

                auto texture = g_geometryFramebuffer.ColorAttachments[1].value().Texture.Id;
                auto imagePosition = ImGui::GetCursorPos();
                ImGui::Image(static_cast<intptr_t>(texture), currentSceneWindowSize, g_imvec2UnitY, g_imvec2UnitX);
                ImGui::SetCursorPos(imagePosition);
                //if (ImGui::BeginChild(1, ImVec2{192, -1})) {
                //    if (ImGui::CollapsingHeader("Statistics")) {
                //    }
                //}
                //ImGui::EndChild();
            }
            ImGui::PopStyleVar();
            ImGui::End();

            if (ImGui::Begin(ICON_MD_SETTINGS " Settings")) {
                if (ImGui::SliderFloat("Resolution Scale", &windowSettings.ResolutionScale, 0.05f, 4.0f, "%.2f")) {

                    g_sceneViewerResized = g_isEditor;
                    g_windowFramebufferResized = !g_isEditor;
                }
                ImGui::Checkbox("Enable FXAA", &g_isFxaaEnabled);
            }
            ImGui::End();



            /*
             * UI - Assets Viewer
             */
            if (ImGui::Begin(ICON_MD_DATA_OBJECT " Assets")) {
                auto& assets = GetAssets();
                ImGui::BeginTable("##Assets", 2, ImGuiTableFlags_RowBg);
                ImGui::TableSetupColumn("Asset");
                ImGui::TableSetupColumn("X");
                ImGui::TableNextRow();
                for (auto& [assetName, asset] : assets) {
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(assetName.data());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(assetName.data());
                    if (ImGui::Button(ICON_MD_ADD_BOX " Create")) {

                    }
                    ImGui::PopID();
                    ImGui::TableNextRow();
                }
                ImGui::EndTable();
            }
            ImGui::End();

            /*
             * UI - Scene Hierarchy
             */
            if (ImGui::Begin(ICON_MD_APP_REGISTRATION "Hierarchy")) {

            }
            ImGui::End();

            /*
             * UI - Properties
             */
            if (ImGui::Begin(ICON_MD_ALIGN_HORIZONTAL_LEFT "Properties")) {

            }
            ImGui::End();
        }        
        {
            PROFILER_ZONESCOPEDN("Draw ImGUI");
            ImGui::Render();
            auto* imGuiDrawData = ImGui::GetDrawData();
            if (imGuiDrawData != nullptr) {
                PushDebugGroup("UI");
                glDisable(GL_FRAMEBUFFER_SRGB);
                isSrgbDisabled = true;
                glViewport(0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y);
                ImGui_ImplOpenGL3_RenderDrawData(imGuiDrawData);
                PopDebugGroup();
            }
        }
        {
            PROFILER_ZONESCOPEDN("SwapBuffers");
            glfwSwapBuffers(g_window);
            frameCounter++;
        }

#ifdef USE_PROFILER
        TracyGpuCollect
        FrameMark;
#endif

        glfwPollEvents();

        if (!g_windowHasFocus && g_sleepWhenWindowHasNoFocus) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        previousTimeInSeconds = currentTimeInSeconds;
    }

    /// Cleanup Resources //////

    DeleteBuffer(globalLightsBuffer);
    DeleteBuffer(globalUniformsBuffer);
    DeleteBuffer(objectsBuffer);

    DeleteRendererFramebuffers();

    DeletePipeline(g_debugLinesGraphicsPipeline);
    DeletePipeline(g_fstGraphicsPipeline);

    DeletePipeline(g_depthPrePassGraphicsPipeline);
    DeletePipeline(g_geometryGraphicsPipeline);
    DeletePipeline(g_resolveGeometryGraphicsPipeline);
    DeletePipeline(g_fxaaGraphicsPipeline);

    RendererUnload();

    return 0;
}
