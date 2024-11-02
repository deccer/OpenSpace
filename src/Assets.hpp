#pragma once

#include <entt/entt.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

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

    size_t Index = 0;
};

struct TAsset {
    std::string Name;

    std::vector<TAssetImageData> Images;
};

auto LoadAssetFromFile(const std::filesystem::path& filePath) -> std::expected<TAsset, std::string>;
