#pragma once

#include <entt/entt.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace Assets {

enum class TAssetImageDataType {
    Uncompressed,
    CompressedKtx,
    CompressedDds
};

struct TAssetImageData {

    int32_t Width = 0;
    int32_t Height = 0;
    int32_t PixelType = 0;
    int32_t Bits = 8;
    int32_t Components = 0;
    std::string Name;
    std::unique_ptr<unsigned char[]> Data = {};
    TAssetImageDataType ImageDataType = {};
};

enum class TAssetMaterialChannel {
    Color,
    Normals,
    Scalar
};

enum class TAssetSamplerMagFilter {
    Nearest,
    Linear,
};

enum class TAssetSamplerMinFilter {
    Nearest,
    Linear,
    NearestMipMapNearest,
    LinearMipMapNearest,
    NearestMipMapLinear,
    LinearMipMapLinear,
};

enum class TAssetSamplerWrapMode {
    ClampToEdge,
    MirroredRepeat,
    Repeat,
};

struct TAssetSamplerData {
    std::string Name;
    std::optional<TAssetSamplerMagFilter> MagFilter;
    std::optional<TAssetSamplerMinFilter> MinFilter;
    TAssetSamplerWrapMode WrapS;
    TAssetSamplerWrapMode WrapT;
};

struct TAssetMaterialChannelData {
    TAssetMaterialChannel Channel;    
    std::string SamplerName;
    std::string TextureName;
};

struct TAssetMaterialData {
    std::string Name;
    glm::vec4 BaseColor = {};
    float NormalStrength = 1.0f;
    float Metalness = 0.0f;
    float Roughness = 0.0f;
    float EmissiveFactor = 1.0f;
    std::optional<TAssetMaterialChannelData> BaseColorTextureChannel = {};
    std::optional<TAssetMaterialChannelData> NormalTextureChannel = {};
    std::optional<TAssetMaterialChannelData> ArmTextureChannel = {};
    std::optional<TAssetMaterialChannelData> EmissiveTextureChannel = {};
};

struct TAssetMeshData {
    std::string Name;
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> Uvs;
    std::vector<glm::vec4> Tangents;
    std::vector<uint32_t> Indices;
    std::optional<size_t> MaterialIndex;
    glm::mat4 InitialTransform;
};

struct TAssetInstanceData {
    glm::mat4 WorldMatrix;
    size_t MeshIndex;
};

struct TAsset {
    std::vector<std::string> Images;
    std::vector<std::string> Samplers;
    std::vector<std::string> Materials;
    std::vector<std::string> Meshes;
    std::vector<TAssetInstanceData> Instances;
};

auto AddAssetFromFile(const std::string& assetName, const std::filesystem::path& filePath) -> void;
auto GetAssets() -> std::unordered_map<std::string, TAsset>&;
auto GetAsset(const std::string& assetName) -> TAsset&;
auto IsAssetLoaded(const std::string& assetName) -> bool;

auto GetAssetImageData(const std::string& imageDataName) -> TAssetImageData&;
auto GetAssetSamplerData(const std::string& samplerDataName) -> TAssetSamplerData&;
auto GetAssetMaterialData(const std::string& materialDataName) -> TAssetMaterialData&;
auto GetAssetMeshData(const std::string& meshDataName) -> TAssetMeshData&;
auto AddDefaultAssets() -> void;

}