#pragma once

#include "Core/Types.hpp"

#include <filesystem>

class Image {
public:
    static auto FreeImage(void* pixels) -> void;
    static auto LoadImageFromMemory(
        const std::byte* encodedData,
        const size_t encodedDataSize,
        int32_t* width,
        int32_t* height,
        int32_t* components) -> unsigned char*;
    static auto LoadImageFromFile(
        const std::filesystem::path& filePath,
        int32_t* width,
        int32_t* height,
        int32_t* components) -> unsigned char*;

    static auto EnableFlipImageVertically() -> void;
    static auto DisableFlipImageVertically() -> void;
};
