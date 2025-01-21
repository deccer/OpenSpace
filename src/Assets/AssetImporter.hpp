#pragma once

#include <expected>
#include <filesystem>
#include <string>

struct IAssetImporter {
    virtual auto ImportModel(
        const std::string& modelName,
        const std::filesystem::path& filePath) const -> std::expected<std::string, std::string> = 0;
};
