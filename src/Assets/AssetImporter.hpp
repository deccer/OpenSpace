#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "AssetModel.hpp"

struct IAssetImporter {
    virtual auto ImportModel(
        const std::string& modelName,
        const std::filesystem::path& filePath) const -> std::expected<TAssetModel, std::string> = 0;
};
