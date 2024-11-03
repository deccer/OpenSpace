#include "Assets.hpp"
#include "Io.hpp"
#include "Images.hpp"
#include "Helpers.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <algorithm>
#include <format>
#include <ranges>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#include "Profiler.hpp"

auto MimeTypeToImageDataType(fastgltf::MimeType mimeType) -> TAssetImageDataType {
    if (mimeType == fastgltf::MimeType::KTX2) {
        return TAssetImageDataType::CompressedKtx;
    } else if (mimeType == fastgltf::MimeType::DDS) {
        return TAssetImageDataType::CompressedDds;
    } else {
        return TAssetImageDataType::Uncompressed;
    }
}

struct TAssetRawImageData {
    std::string Name;
    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;
    TAssetImageDataType ImageDataType = {};
};

auto CreateAssetRawImageData(
    const void* data,
    std::size_t dataSize,
    fastgltf::MimeType mimeType,
    std::string_view name) -> TAssetRawImageData {

    PROFILER_ZONESCOPEDN("CreateAssetImage");

    auto dataCopy = std::make_unique<std::byte[]>(dataSize);
    std::copy_n(static_cast<const std::byte*>(data), dataSize, dataCopy.get());

    return TAssetRawImageData {
        .Name = std::string(name),
        .EncodedData = std::move(dataCopy),
        .EncodedDataSize = dataSize,
        .ImageDataType = MimeTypeToImageDataType(mimeType)
    };
}

auto LoadImages(const std::string& modelName, TAsset& asset, fastgltf::Asset& fgAsset, const std::filesystem::path& filePath) -> void {

    asset.Images.resize(fgAsset.images.size());

    const auto imageIndices = std::ranges::iota_view{0uz, fgAsset.images.size()};
    std::for_each(
        poolstl::execution::par,
        imageIndices.begin(),
        imageIndices.end(),
        [&](size_t imageIndex) -> void {

        const auto& fgImage = fgAsset.images[imageIndex];

        const auto imageData = [&]{

            auto imageName = GetSafeResourceName(modelName.data(), fgImage.name.data(), "image", imageIndex);
            if (const auto* filePathUri = std::get_if<fastgltf::sources::URI>(&fgImage.data)) {
                auto filePathFixed = std::filesystem::path(filePathUri->uri.path());
                auto filePathParent = filePath.parent_path();
                if (filePathFixed.is_relative()) {
                    filePathFixed = filePath.parent_path() / filePathFixed;
                }
                auto fileData = ReadBinaryFromFile(filePathFixed);
                return CreateAssetRawImageData(fileData.first.get(), fileData.second, filePathUri->mimeType, imageName);
            }
            if (const auto* array = std::get_if<fastgltf::sources::Array>(&fgImage.data)) {
                return CreateAssetRawImageData(array->bytes.data(), array->bytes.size(), array->mimeType, imageName);
            }
            if (const auto* vector = std::get_if<fastgltf::sources::Vector>(&fgImage.data)) {
                return CreateAssetRawImageData(vector->bytes.data(), vector->bytes.size(), vector->mimeType, imageName);
            }
            if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&fgImage.data)) {
                auto& bufferView = fgAsset.bufferViews[view->bufferViewIndex];
                auto& buffer = fgAsset.buffers[bufferView.bufferIndex];
                if (const auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                    return CreateAssetRawImageData(
                        array->bytes.data() + bufferView.byteOffset,
                        bufferView.byteLength,
                        view->mimeType,
                        imageName);
                }
            }

            return TAssetRawImageData{};
        }();

        auto& assetImage = asset.Images[imageIndex];
        assetImage.ImageDataType = imageData.ImageDataType;
        if (assetImage.ImageDataType == TAssetImageDataType::CompressedKtx) {

        } else if (assetImage.ImageDataType == TAssetImageDataType::CompressedDds) {

        } else {
            int32_t width = 0;
            int32_t height = 0;
            int32_t components = 0;
            auto* pixels = LoadImageFromMemory(imageData.EncodedData.get(), imageData.EncodedDataSize, &width, &height, &components);

            assetImage.Name = imageData.Name;
            assetImage.Width = width;
            assetImage.Height = height;
            assetImage.Components = components;
            assetImage.Data.reset(pixels);
        }
    });
}

auto LoadMaterials(const std::string& modelName, TAsset& asset, fastgltf::Asset& fgAsset) -> void {

    auto GetImageIndex = [&](fastgltf::Asset& fgAsset, const std::optional<fastgltf::TextureInfo>& textureInfo) -> std::optional<size_t> {
        if (!textureInfo.has_value()) {
            return std::nullopt;
        }

        const auto& fgTexture = fgAsset.textures[textureInfo->textureIndex];
        return fgTexture.ddsImageIndex.value_or(fgTexture.basisuImageIndex.value_or(fgTexture.imageIndex.value()));
    };

    const auto materialIndices = std::ranges::iota_view{0uz, fgAsset.materials.size()};
    std::for_each(
        poolstl::execution::par,
        materialIndices.begin(),
        materialIndices.end(),
        [&](size_t materialIndex) -> void {

        const auto& fgMaterial = fgAsset.materials[materialIndex];

        auto& material = asset.Materials[materialIndex];
        material.BaseColorTextureIndex = GetImageIndex(fgAsset, fgMaterial.pbrData.baseColorTexture);
        material.EmissiveTextureIndex = GetImageIndex(fgAsset, fgMaterial.emissiveTexture);
    });
}

auto LoadAssetFromFile(const std::string& modelName, const std::filesystem::path& filePath) -> std::expected<TAsset, std::string> {
    if (!std::filesystem::exists(filePath)) {
        return std::unexpected(std::format("Unable to load asset. File '{}' does not exist", filePath.string()));
    }

    fastgltf::Parser parser(
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::EXT_mesh_gpu_instancing);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadGLBBuffers |
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    auto assetResult = parser.loadGltf(&data, filePath.parent_path(), gltfOptions);
    if (assetResult.error() != fastgltf::Error::None)
    {
        return std::unexpected(std::format("Failed to load glTF. {}", fastgltf::getErrorMessage(assetResult.error())));
    }

    auto& fgAsset = assetResult.get();

    TAsset asset = {};

    LoadImages(modelName, asset, fgAsset, filePath);
    LoadMaterials(modelName, asset, fgAsset);

    return asset;
}
