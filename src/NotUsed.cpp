/// ASSETS - BEGIN -  REMOVE THIS BS

using TAssetMeshId = TId<struct AssetMeshIdMarker>;
using TAssetImageId = TId<struct AssetImageIdMarker>;
using TAssetTextureId = TId<struct AssetTextureIdMarker>;
using TAssetSamplerId = TId<struct AssetSamplerIdMarker>;
using TAssetMaterialId = TId<struct AssetMaterialIdMarker>;

enum class TMaterialChannelUsage {
    SRgb, // use sRGB Format
    Normal, // use RG-format or bc7 when compressed
    MetalnessRoughness // single channelism or also RG?
};

struct TAssetMesh {
    std::string_view Name;
    glm::mat4 InitialTransform;
    std::vector<TGpuVertexPosition> VertexPositions;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> VertexNormalUvTangents;
    std::vector<uint32_t> Indices;
    std::string MaterialName;
};

struct TAssetSampler {
    TTextureAddressMode AddressModeU = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeV = TTextureAddressMode::ClampToEdge;
    TTextureMagFilter MagFilter = TTextureMagFilter::Linear;
    TTextureMinFilter MinFilter = TTextureMinFilter::Linear;
};

struct TAssetImage {

    int32_t Width = 0;
    int32_t Height = 0;
    int32_t PixelType = 0;
    int32_t Bits = 8;
    int32_t Components = 0;
    std::string Name;

    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;

    std::unique_ptr<unsigned char[]> DecodedData = {};

    size_t Index = 0;
};

struct TAssetTexture {
    std::string ImageName;
    std::string SamplerName;
};

struct TAssetMaterialChannel {
    TMaterialChannelUsage Usage;
    std::string Image;
    std::string Sampler;
};

struct TAssetMaterial {
    glm::vec4 BaseColorFactor;
    float MetallicFactor;
    glm::vec3 EmissiveFactor;
    float EmissiveStrength;
    float NormalStrength;
    std::optional<TAssetMaterialChannel> BaseColorChannel;
    std::optional<TAssetMaterialChannel> NormalsChannel;
    std::optional<TAssetMaterialChannel> OcclusionRoughnessMetallnessChannel;
    std::optional<TAssetMaterialChannel> EmissiveChannel;
};

std::unordered_map<std::string, TAssetImage> g_assetImages = {};
std::unordered_map<std::string, TAssetSampler> g_assetSamplers = {};
std::unordered_map<std::string, TAssetTexture> g_assetTextures = {};
std::unordered_map<std::string, TAssetMaterial> g_assetMaterials = {};
std::unordered_map<std::string, TAssetMesh> g_assetMeshes = {};
std::unordered_map<std::string, std::vector<std::string>> g_assetModelMeshes = {};

auto AddAssetMesh(
    const std::string& assetMeshName,
    const TAssetMesh& assetMesh) -> void {

    assert(!assetMeshName.empty());
    PROFILER_ZONESCOPEDN("AddAssetMesh");

    g_assetMeshes[assetMeshName] = assetMesh;
}

auto AddAssetImage(
    const std::string& assetImageName,
    TAssetImage&& assetImage) -> void {

    assert(!assetImageName.empty());
    PROFILER_ZONESCOPEDN("AddAssetImage");

    g_assetImages[assetImageName] = std::move(assetImage);
}

auto AddAssetMaterial(
    const std::string& assetMaterialName,
    const TAssetMaterial& assetMaterial) -> void {

    assert(!assetMaterialName.empty());
    PROFILER_ZONESCOPEDN("AddAssetMaterial");

    g_assetMaterials[assetMaterialName] = assetMaterial;
}

auto AddAssetTexture(
    const std::string& assetTextureName,
    const TAssetTexture& assetTexture) -> void {

    assert(!assetTextureName.empty());
    PROFILER_ZONESCOPEDN("AddAssetTexture");

    g_assetTextures[assetTextureName] = assetTexture;
}

auto AddAssetSampler(
    const std::string& assetSamplerName,
    const TAssetSampler& assetSampler) -> void {

    assert(!assetSamplerName.empty());
    PROFILER_ZONESCOPEDN("AddAssetSampler");

    g_assetSamplers[assetSamplerName] = assetSampler;
}

auto GetAssetImage(const std::string& assetImageName) -> TAssetImage& {

    assert(!assetImageName.empty() || g_assetImages.contains(assetImageName));
    PROFILER_ZONESCOPEDN("GetAssetImage");

    return g_assetImages.at(assetImageName);
}

auto GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& {

    assert(!assetMeshName.empty() || g_assetMeshes.contains(assetMeshName));
    PROFILER_ZONESCOPEDN("GetAssetMesh");

    return g_assetMeshes.at(assetMeshName);
}

auto GetAssetMaterial(const std::string& assetMaterialName) -> TAssetMaterial& {

    assert(!assetMaterialName.empty() || g_assetMaterials.contains(assetMaterialName));
    PROFILER_ZONESCOPEDN("GetAssetMaterial");

    return g_assetMaterials.at(assetMaterialName);
}

auto GetAssetModelMeshNames(const std::string& modelName) -> std::vector<std::string>& {

    assert(!modelName.empty() || g_assetModelMeshes.contains(modelName));
    PROFILER_ZONESCOPEDN("GetAssetModelMeshNames");

    return g_assetModelMeshes.at(modelName);
}

auto GetAssetSampler(const std::string& assetSamplerName) -> TAssetSampler& {

    assert(!assetSamplerName.empty() || g_assetSamplers.contains(assetSamplerName));
    PROFILER_ZONESCOPEDN("GetAssetSampler");

    return g_assetSamplers.at(assetSamplerName);
}

/*
auto GetAssetSampler(const SAssetSamplerId& assetSamplerId) -> TSamplerDescriptor {

    PROFILER_ZONESCOPEDN("GetAssetSampler");
    for (const auto& [key, value] : g_assetSamplers) {
        if (value == assetSamplerId) {
            return key;
        }
    }

    spdlog::error("Unable to find Asset Sampler");
    return TSamplerDescriptor {};
}
*/

auto GetAssetTexture(const std::string& assetTextureName) -> TAssetTexture& {

    PROFILER_ZONESCOPEDN("GetAssetTexture");
    assert(!assetTextureName.empty());

    return g_assetTextures[assetTextureName];
}

auto GetLocalTransform(const fastgltf::Node& node) -> glm::mat4 {

    glm::mat4 transform{1.0};

    if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform))
    {
        auto rotation = glm::quat{trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]};
        auto scale = glm::make_vec3(trs->scale.data());
        auto translation = glm::make_vec3(trs->translation.data());

        auto rotationMatrix = glm::mat4_cast(rotation);

        // T * R * S
        transform = glm::scale(glm::translate(glm::mat4(1.0f), translation) * rotationMatrix, scale);
    }
    else if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform))
    {
        const auto& m = *mat;
        transform = glm::make_mat4x4(m.data());
    }

    return transform;
}

auto SignNotZero(glm::vec2 v) -> glm::vec2 {

    return glm::vec2((v.x >= 0.0f) ? +1.0f : -1.0f, (v.y >= 0.0f) ? +1.0f : -1.0f);
}

auto EncodeNormal(glm::vec3 normal) -> glm::vec2 {

    glm::vec2 encodedNormal = glm::vec2{normal.x, normal.y} * (1.0f / (abs(normal.x) + abs(normal.y) + abs(normal.z)));
    return (normal.z <= 0.0f)
           ? ((1.0f - glm::abs(glm::vec2{encodedNormal.x, encodedNormal.y})) * SignNotZero(encodedNormal)) //TODO(deccer) check if its encNor.y, encNor.x or not
           : encodedNormal;
}

auto GetVertices(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive) -> std::pair<std::vector<TGpuVertexPosition>, std::vector<TGpuPackedVertexNormalTangentUvTangentSign>> {

    PROFILER_ZONESCOPEDN("GetVertices");

    std::vector<glm::vec3> positions;
    auto& positionAccessor = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
    positions.resize(positionAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  positionAccessor,
                                                  [&](glm::vec3 position, std::size_t index) { positions[index] = position; });

    std::vector<glm::vec3> normals;
    auto& normalAccessor = asset.accessors[primitive.findAttribute("NORMAL")->accessorIndex];
    normals.resize(normalAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  normalAccessor,
                                                  [&](glm::vec3 normal, std::size_t index) { normals[index] = normal; });

    std::vector<glm::vec4> tangents;
    auto& tangentAccessor = asset.accessors[primitive.findAttribute("TANGENT")->accessorIndex];
    tangents.resize(tangentAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                  tangentAccessor,
                                                  [&](glm::vec4 tangent, std::size_t index) { tangents[index] = tangent; });

    std::vector<glm::vec3> uvs;
    if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
    {
        auto& uvAccessor = asset.accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];
        uvs.resize(uvAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                      uvAccessor,
                                                      [&](glm::vec2 uv, std::size_t index) { uvs[index] = glm::vec3{uv, 0.0f}; });
    }
    else
    {
        uvs.resize(positions.size(), {});
    }

    std::vector<TGpuVertexPosition> verticesPosition;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> verticesNormalUvTangent;
    verticesPosition.resize(positions.size());
    verticesNormalUvTangent.resize(positions.size());

    for (size_t i = 0; i < positions.size(); i++) {
        verticesPosition[i] = {
            positions[i],
        };
        uvs[i].z = tangents[i].w;
        verticesNormalUvTangent[i] = {
            glm::packSnorm2x16(EncodeNormal(normals[i])),
            glm::packSnorm2x16(EncodeNormal(tangents[i].xyz())),
            glm::vec4(uvs[i], 0.0f),
        };
    }

    return {verticesPosition, verticesNormalUvTangent};
}

auto GetIndices(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive) -> std::vector<uint32_t> {

    PROFILER_ZONESCOPEDN("GetIndices");

    auto indices = std::vector<uint32_t>();
    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](uint32_t value, size_t index)
    {
        indices[index] = value;
    });
    return indices;
}

auto CreateAssetMesh(
    const std::string& name,
    glm::mat4 initialTransform,
    const std::pair<std::vector<TGpuVertexPosition>, std::vector<TGpuPackedVertexNormalTangentUvTangentSign>>& vertices,
    const std::vector<uint32_t> indices,
    const std::string& materialName) -> TAssetMesh {

    PROFILER_ZONESCOPEDN("CreateAssetMesh");

    return TAssetMesh{
        .Name = name,
        .InitialTransform = initialTransform,
        .VertexPositions = std::move(vertices.first),
        .VertexNormalUvTangents = std::move(vertices.second),
        .Indices = std::move(indices),
        .MaterialName = materialName
    };
}

auto CreateAssetImage(
    const void* data,
    std::size_t dataSize,
    fastgltf::MimeType mimeType,
    std::string_view name) -> TAssetImage {

    PROFILER_ZONESCOPEDN("CreateAssetImage");

    auto dataCopy = std::make_unique<std::byte[]>(dataSize);
    std::copy_n(static_cast<const std::byte*>(data), dataSize, dataCopy.get());

    return TAssetImage {
        .Name = std::string(name),
        .EncodedData = std::move(dataCopy),
        .EncodedDataSize = dataSize,
    };
}

auto CreateAssetMaterial(
    const std::string& modelName,
    const fastgltf::Asset& asset,
    const fastgltf::Material& fgMaterial,
    size_t assetMaterialIndex) -> void {

    PROFILER_ZONESCOPEDN("CreateAssetMaterial");

    TAssetMaterial assetMaterial = {};
    assetMaterial.BaseColorFactor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
    assetMaterial.MetallicFactor = fgMaterial.pbrData.metallicFactor;
    assetMaterial.EmissiveFactor = glm::make_vec3(fgMaterial.emissiveFactor.data());
    assetMaterial.EmissiveStrength = fgMaterial.emissiveStrength;

    if (fgMaterial.pbrData.baseColorTexture.has_value()) {
        auto& fgBaseColorTexture = fgMaterial.pbrData.baseColorTexture.value();
        auto& fgTexture = asset.textures[fgBaseColorTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.normalTexture.has_value()) {
        auto& fgNormalTexture = fgMaterial.normalTexture.value();
        auto& fgTexture = asset.textures[fgNormalTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::Normal,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.pbrData.metallicRoughnessTexture.has_value()) {
        auto& fgMetallicRoughnessTexture = fgMaterial.pbrData.metallicRoughnessTexture.value();
        auto& fgTexture = asset.textures[fgMetallicRoughnessTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.OcclusionRoughnessMetallnessChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::MetalnessRoughness,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.emissiveTexture.has_value()) {
        auto& fgEmissiveTexture = fgMaterial.emissiveTexture.value();
        auto& fgTexture = asset.textures[fgEmissiveTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", assetMaterialIndex);

    return AddAssetMaterial(materialName, assetMaterial);
}

auto LoadModelFromFile(
    std::string modelName,
    std::filesystem::path filePath) -> void {

    PROFILER_ZONESCOPEDN("LoadModelFromFile");

    constexpr auto parserOptions =
        fastgltf::Extensions::EXT_mesh_gpu_instancing |
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::EXT_meshopt_compression |
        fastgltf::Extensions::KHR_lights_punctual |
        fastgltf::Extensions::EXT_texture_webp |
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_texture_basisu |
        fastgltf::Extensions::MSFT_texture_dds |
        fastgltf::Extensions::KHR_materials_specular |
        fastgltf::Extensions::KHR_materials_ior |
        fastgltf::Extensions::KHR_materials_iridescence |
        fastgltf::Extensions::KHR_materials_volume |
        fastgltf::Extensions::KHR_materials_transmission |
        fastgltf::Extensions::KHR_materials_clearcoat |
        fastgltf::Extensions::KHR_materials_emissive_strength |
        fastgltf::Extensions::KHR_materials_sheen |
        fastgltf::Extensions::KHR_materials_unlit;
    fastgltf::Parser parser(parserOptions);

    auto dataResult = fastgltf::GltfDataBuffer::FromPath(filePath);
    if (dataResult.error() != fastgltf::Error::None) {
        return;
    }

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages;
    const auto parentPath = filePath.parent_path();
    auto loadResult = parser.loadGltf(dataResult.get(), parentPath, gltfOptions);
    if (loadResult.error() != fastgltf::Error::None)
    {
        return;
    }

    auto& fgAsset = loadResult.get();

    // images

    auto assetImageIds = std::vector<TAssetImageId>(fgAsset.images.size());
    const auto assetImageIndices = std::ranges::iota_view{(size_t)0, fgAsset.images.size()};

    std::transform(
        poolstl::execution::par,
        assetImageIndices.begin(),
        assetImageIndices.end(),
        assetImageIds.begin(),
        [&](size_t imageIndex) {

            PROFILER_ZONESCOPEDN("Create Image");
            const auto& fgImage = fgAsset.images[imageIndex];
            auto imageData = [&] {
                if (const auto* filePathUri = std::get_if<fastgltf::sources::URI>(&fgImage.data)) {
                    auto filePathFixed = std::filesystem::path(filePathUri->uri.path());
                    auto filePathParent = filePath.parent_path();
                    if (filePathFixed.is_relative()) {
                        filePathFixed = filePath.parent_path() / filePathFixed;
                    }
                    auto fileData = ReadBinaryFromFile(filePathFixed);
                    return CreateAssetImage(fileData.first.get(), fileData.second, filePathUri->mimeType, fgImage.name);
                }
                if (const auto* array = std::get_if<fastgltf::sources::Array>(&fgImage.data)) {
                    return CreateAssetImage(array->bytes.data(), array->bytes.size(), array->mimeType, fgImage.name);
                }
                if (const auto* vector = std::get_if<fastgltf::sources::Vector>(&fgImage.data)) {
                    return CreateAssetImage(vector->bytes.data(), vector->bytes.size(), vector->mimeType, fgImage.name);
                }
                if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&fgImage.data)) {
                    auto& bufferView = fgAsset.bufferViews[view->bufferViewIndex];
                    auto& buffer = fgAsset.buffers[bufferView.bufferIndex];
                    if (const auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                        return CreateAssetImage(
                            array->bytes.data() + bufferView.byteOffset,
                            bufferView.byteLength,
                            view->mimeType,
                            fgImage.name);
                    }
                }

                return TAssetImage{};
            }();

            auto assetImageId = imageIndex;

            int32_t width = 0;
            int32_t height = 0;
            int32_t components = 0;
            auto pixels = LoadImageFromMemory(
                imageData.EncodedData.get(),
                imageData.EncodedDataSize,
                &width,
                &height,
                &components);

            imageData.Width = width;
            imageData.Height = height;
            imageData.Components = 4; // LoadImageFromMemory is forcing 4 components
            imageData.DecodedData.reset(pixels);
            imageData.Index = assetImageId;

            AddAssetImage(GetSafeResourceName(modelName.data(), imageData.Name.data(), "image", imageIndex), std::move(imageData));
            return static_cast<TAssetImageId>(assetImageId);
        });

    // samplers

    auto samplerIds = std::vector<TAssetSamplerId>(fgAsset.samplers.size());
    const auto samplerIndices = std::ranges::iota_view{(size_t)0, fgAsset.samplers.size()};
    std::transform(poolstl::execution::par, samplerIndices.begin(), samplerIndices.end(), samplerIds.begin(), [&](size_t samplerIndex) {

        PROFILER_ZONESCOPEDN("Load Sampler");
        const fastgltf::Sampler& fgSampler = fgAsset.samplers[samplerIndex];

        const auto getAddressMode = [](const fastgltf::Wrap& wrap) {
            switch (wrap) {
                case fastgltf::Wrap::ClampToEdge: return TTextureAddressMode::ClampToEdge;
                case fastgltf::Wrap::MirroredRepeat: return TTextureAddressMode::RepeatMirrored;
                case fastgltf::Wrap::Repeat: return TTextureAddressMode::Repeat;
                default:
                    return TTextureAddressMode::ClampToEdge;
            }
        };

        //TODO(deccer) reinvent min/mag filterisms, this sucks here
        const auto getMinFilter = [](const fastgltf::Filter& minFilter) {
            switch (minFilter) {
                case fastgltf::Filter::Linear: return TTextureMinFilter::Linear;
                case fastgltf::Filter::LinearMipMapLinear: return TTextureMinFilter::LinearMipmapLinear;
                case fastgltf::Filter::LinearMipMapNearest: return TTextureMinFilter::LinearMipmapNearest;
                case fastgltf::Filter::Nearest: return TTextureMinFilter::Nearest;
                case fastgltf::Filter::NearestMipMapLinear: return TTextureMinFilter::NearestMipmapLinear;
                case fastgltf::Filter::NearestMipMapNearest: return TTextureMinFilter::NearestMipmapNearest;
                default: std::unreachable();
            }
        };

        const auto getMagFilter = [](const fastgltf::Filter& magFilter) {
            switch (magFilter) {
                case fastgltf::Filter::Linear: return TTextureMagFilter::Linear;
                case fastgltf::Filter::Nearest: return TTextureMagFilter::Nearest;
                default: std::unreachable();
            }
        };

        auto assetSampler = TAssetSampler {
            .AddressModeU = getAddressMode(fgSampler.wrapS),
            .AddressModeV = getAddressMode(fgSampler.wrapT),
            .MagFilter = getMagFilter(fgSampler.magFilter.has_value() ? *fgSampler.magFilter : fastgltf::Filter::Nearest),
            .MinFilter = getMinFilter(fgSampler.minFilter.has_value() ? *fgSampler.minFilter : fastgltf::Filter::Nearest),
        };

        auto assetSamplerId = g_assetSamplers.size() + samplerIndex;
        AddAssetSampler(GetSafeResourceName(modelName.data(), fgSampler.name.data(), "sampler", assetSamplerId), assetSampler);
        return static_cast<TAssetSamplerId>(assetSamplerId);
    });

    // textures

    auto assetTextures = std::vector<TAssetTextureId>(fgAsset.textures.size());
    const auto assetTextureIndices = std::ranges::iota_view{(size_t)0, fgAsset.textures.size()};
    for (std::weakly_incrementable auto assetTextureIndex : assetTextureIndices) {

        auto& fgTexture = fgAsset.textures[assetTextureIndex];
        auto textureName = GetSafeResourceName(modelName.data(), fgTexture.name.data(), "texture", assetTextureIndex);

        AddAssetTexture(textureName, TAssetTexture{
            .ImageName = GetSafeResourceName(modelName.data(), nullptr, "image", fgTexture.imageIndex.value_or(0)),
            .SamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0))
        });
    }

    // materials

    auto assetMaterialIds = std::vector<TAssetMaterialId>(fgAsset.materials.size());
    const auto assetMaterialIndices = std::ranges::iota_view{(size_t)0, fgAsset.materials.size()};
    for (std::weakly_incrementable auto assetMaterialIndex : assetMaterialIndices) {

        auto& fgMaterial = fgAsset.materials[assetMaterialIndex];

        CreateAssetMaterial(modelName, fgAsset, fgMaterial, assetMaterialIndex);
    }

    std::stack<std::pair<const fastgltf::Node*, glm::mat4>> nodeStack;
    glm::mat4 rootTransform = glm::mat4(1.0f);

    for (auto nodeIndex : fgAsset.scenes[0].nodeIndices)
    {
        nodeStack.emplace(&fgAsset.nodes[nodeIndex], rootTransform);
    }

    while (!nodeStack.empty())
    {
        PROFILER_ZONESCOPEDN("Load Primitive");

        decltype(nodeStack)::value_type top = nodeStack.top();
        const auto& [fgNode, parentGlobalTransform] = top;
        nodeStack.pop();

        glm::mat4 localTransform = GetLocalTransform(*fgNode);
        glm::mat4 globalTransform = parentGlobalTransform * localTransform;

        for (auto childNodeIndex : fgNode->children)
        {
            nodeStack.emplace(&fgAsset.nodes[childNodeIndex], globalTransform);
        }

        if (!fgNode->meshIndex.has_value()) {
            continue;
        }

        auto meshIndex = fgNode->meshIndex.value();
        auto& fgMesh = fgAsset.meshes[meshIndex];

        for (auto primitiveIndex = 0; const auto& fgPrimitive : fgMesh.primitives)
        {
            const auto primitiveMaterialIndex = fgPrimitive.materialIndex.value_or(0);

            auto vertices = GetVertices(fgAsset, fgPrimitive);
            auto indices = GetIndices(fgAsset, fgPrimitive);

            auto meshName = GetSafeResourceName(modelName.data(), fgMesh.name.data(), "mesh", primitiveIndex);

            auto materialIndex = fgPrimitive.materialIndex.value_or(0);
            auto& fgMaterial = fgAsset.materials[materialIndex];
            auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", materialIndex);

            auto assetMesh = CreateAssetMesh(meshName, globalTransform, std::move(vertices), std::move(indices), materialName);
            AddAssetMesh(meshName, assetMesh);
            g_assetModelMeshes[modelName].push_back(meshName);

            primitiveIndex++;
        }
    }
}

/// ASSETS - END - REMOVE THIS BS


auto CreateGpuMaterial(const std::string& assetMaterialName) -> void {

    PROFILER_ZONESCOPEDN("CreateGpuMaterial");

    auto& assetMaterial = GetAssetMaterialData(assetMaterialName);

    uint64_t baseColorTextureHandle = assetMaterial.BaseColorChannel.has_value()
                                      ? CreateResidentTextureForMaterialChannel(assetMaterial.BaseColorChannel.value())
                                      : 0;

    uint64_t normalTextureHandle = assetMaterial.NormalsChannel.has_value()
                                   ? CreateResidentTextureForMaterialChannel(assetMaterial.NormalsChannel.value())
                                   : 0;

    auto gpuMaterial = TGpuMaterial{
        .BaseColor = assetMaterial.BaseColorFactor,
        .BaseColorTexture = baseColorTextureHandle,
        .NormalTexture = normalTextureHandle
    };

    g_gpuMaterials[assetMaterialName] = gpuMaterial;
}