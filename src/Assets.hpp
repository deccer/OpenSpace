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
#include <glm/gtc/quaternion.hpp>

#include <parallel_hashmap/phmap_fwd_decl.h>

namespace Assets {

enum class TAssetImageType {
    Uncompressed,
    CompressedKtx,
    CompressedDds
};

struct TAssetImage {

    int32_t Width = 0;
    int32_t Height = 0;
    int32_t PixelType = 0;
    int32_t Bits = 8;
    int32_t Components = 0;
    std::string Name;
    std::unique_ptr<unsigned char[]> Data = {};
    TAssetImageType ImageDataType = {};
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

struct TAssetSampler {
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

struct TAssetMaterial {
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

struct TAssetPrimitive {
    std::string Name;
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> Uvs;
    std::vector<glm::vec4> Tangents;
    std::vector<uint32_t> Indices;
    std::optional<std::string> MaterialName;
};

struct TAssetMesh {
    std::string Name;
    std::vector<TAssetPrimitive> Primitives;
};

struct TAssetModelNode {
    std::string Name;
    glm::vec3 LocalPosition;
    glm::quat LocalRotation;
    glm::vec3 LocalScale;
    std::optional<std::string> MeshName;
    std::vector<TAssetModelNode> Children;
};

struct TAssetModel {
    std::string Name;
    std::vector<std::string> Animations;
    std::vector<std::string> Skins;
    std::vector<std::string> Images;
    std::vector<std::string> Samplers;
    std::vector<std::string> Textures;
    std::vector<std::string> Materials;
    std::vector<std::string> Meshes;
    std::vector<TAssetModelNode> Hierarchy;
};

auto AddAssetModel(const std::string& assetName, const TAssetModel& asset) -> void;
auto AddAssetModelFromFile(const std::string& assetName, const std::filesystem::path& filePath) -> void;
auto GetAssetModels() -> std::unordered_map<std::string, TAssetModel>&;
auto GetAssetModel(const std::string& assetName) -> TAssetModel&;
auto IsAssetLoaded(const std::string& assetName) -> bool;

auto GetAssetImage(const std::string& imageDataName) -> TAssetImage&;
auto GetAssetSampler(const std::string& samplerDataName) -> TAssetSampler&;
auto GetAssetMaterial(const std::string& materialDataName) -> TAssetMaterial&;
auto GetAssetMesh(const std::string& meshDataName) -> TAssetMesh&;
auto GetAssetPrimitive(const std::string& assetPrimitiveName) -> TAssetPrimitive&;
auto AddDefaultAssets() -> void;

}
