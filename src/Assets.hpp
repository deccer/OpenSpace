#pragma once

#include <entt/entt.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glm/vec4.hpp>

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

struct TAssetMaterialData {
    std::string Name;
    glm::vec4 BaseColor = {};
    float NormalStrength = 1.0f;
    float Metalness = 0.0f;
    float Roughness = 0.0f;
    float EmissiveFactor = 1.0f;
    std::optional<size_t> BaseColorTextureIndex = {};
    std::optional<size_t> NormalTextureIndex = {};
    std::optional<size_t> ArmTextureIndex = {};
    std::optional<size_t> EmissiveTextureIndex = {};
};

struct TAsset {
    std::string Name;

    std::vector<TAssetImageData> Images;
    std::vector<TAssetMaterialData> Materials;
};

auto AddAssetFromFile(const std::string& assetName, const std::filesystem::path& filePath) -> void;
auto GetAssets() -> std::unordered_map<std::string, TAsset>&;
auto LoadAssetFromFile(const std::string& modelName, const std::filesystem::path& filePath) -> std::expected<TAsset, std::string>;
