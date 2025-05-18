#pragma once

namespace Image {

    auto FreeImage(void* pixels) -> void;

    auto LoadImageFromMemory(
        const std::byte* encodedData,
        size_t encodedDataSize,
        int32_t* width,
        int32_t* height,
        int32_t* components) -> unsigned char*;

    auto LoadImageFromFile(
        const std::filesystem::path& filePath,
        int32_t* width,
        int32_t* height,
        int32_t* components) -> unsigned char*;

    auto EnableFlipImageVertically() -> void;
    auto DisableFlipImageVertically() -> void;

}
