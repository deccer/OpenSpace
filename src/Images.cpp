#include "Images.hpp"

#include "Profiler.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

auto FreeImage(void* pixels) -> void {
    if (pixels != nullptr) {
        stbi_image_free(pixels);
    }
}

auto LoadImageFromMemory(
    std::byte* encodedData,
    size_t encodedDataSize,
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

auto LoadImageFromFile(
    const std::filesystem::path& filePath,
    int32_t* width,
    int32_t* height,
    int32_t* components) -> unsigned char* {

    PROFILER_ZONESCOPEDN("LoadImageFromFile");

    auto imageFile = fopen(filePath.c_str(), "rb");
    if (imageFile != nullptr) {
        auto* pixels = stbi_load_from_file(imageFile, width, height, components, 4);
        fclose(imageFile);
        return pixels;
    } else {
        return nullptr;
    }
}
