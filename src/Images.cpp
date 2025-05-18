#include "Images.hpp"

#include "Profiler.hpp"
#include "Io.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

auto Image::FreeImage(void* pixels) -> void {
    if (pixels != nullptr) {
        stbi_image_free(pixels);
    }
}

auto Image::LoadImageFromMemory(
    const std::byte* encodedData,
    const size_t encodedDataSize,
    int32_t* width,
    int32_t* height,
    int32_t* components) -> unsigned char* {

    return stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(encodedData),
        static_cast<int32_t>(encodedDataSize),
        width,
        height,
        components,
        4);
}

auto Image::LoadImageFromFile(
    const std::filesystem::path& filePath,
    int32_t* width,
    int32_t* height,
    int32_t* components) -> unsigned char* {

    PROFILER_ZONESCOPEDN("LoadImageFromFile");

    auto [imageData, imageDataSize] = ReadBinaryFromFile(filePath);

    return LoadImageFromMemory(imageData.get(), imageDataSize, width, height, components);
}

auto Image::EnableFlipImageVertically() -> void {
    stbi_set_flip_vertically_on_load(1);
}

auto Image::DisableFlipImageVertically() -> void {
    stbi_set_flip_vertically_on_load(0);
}
