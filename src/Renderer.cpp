#include "Renderer.hpp"
#include "RHI.hpp"
#include "Components.hpp"
#include "Assets.hpp"
#include "Images.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <entt/entity/fwd.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "imgui_internal.h"
#include <ImGuizmo.h>
#include "Fonts/CompressedFont-JetBrainsMono-Regular.inl"
#include "Fonts/CompressedFont-MaterialDesign.inl"
#include "Fonts/CompressedFont-RobotoBold.inl"
#include "Fonts/CompressedFont-RobotoMedium.inl"
#include "Fonts/CompressedFont-RobotoRegular.inl"
#include "Fonts/IconsMaterialDesignIcons.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

enum class TImGuizmoOperation
{
    Translate,
    Rotate,
    Scale
};

enum class TImGuizmoMode
{
    Local,
    World
};

struct TGpuDebugLine {
    glm::vec3 StartPosition;
    glm::vec4 StartColor;
    glm::vec3 EndPosition;
    glm::vec4 EndColor;
};

struct TGpuGlobalUniforms {
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::mat4 CurrentJitteredViewProjectionMatrix;
    glm::mat4 PreviousJitteredViewProjectionMatrix;
    glm::vec4 CameraPosition;
    glm::vec4 CameraDirection;
};

struct TGpuObject {
    glm::mat4x4 WorldMatrix;
    glm::ivec4 InstanceParameter;
};

struct TGpuGlobalLight {
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 Direction;
    glm::vec4 Strength;
};

entt::entity g_playerEntity = entt::null;

TWindowSettings g_windowSettings = {};

int32_t g_sceneViewerTextureIndex = {};
ImVec2 g_sceneViewerImagePosition = {};

TFramebuffer g_depthPrePassFramebuffer = {};
TGraphicsPipeline g_depthPrePassGraphicsPipeline = {};

TFramebuffer g_geometryFramebuffer = {};
TGraphicsPipeline g_geometryGraphicsPipeline = {};

TFramebuffer g_resolveGeometryFramebuffer = {};
TGraphicsPipeline g_resolveGeometryGraphicsPipeline = {};

TFramebuffer g_taaFramebuffer1 = {};
TFramebuffer g_taaFramebuffer2 = {};
TGraphicsPipeline g_taaGraphicsPipeline = {};
int32_t g_taaHistoryIndex = 0;
TSamplerId g_taaSampler = {};

TFramebuffer g_fxaaFramebuffer = {};
TGraphicsPipeline g_fxaaGraphicsPipeline = {};

TTexture g_skyBoxTexture = {};
TTexture g_skyBoxConvolvedTexture = {};
uint32_t g_skyBoxSHCoefficients = {};

std::vector<TGpuGlobalLight> g_gpuGlobalLights;
uint32_t g_globalLightsBuffer = {};

TGpuGlobalUniforms g_globalUniforms = {};
glm::mat4 g_previousViewMatrix = {};
glm::mat4 g_previousJitteredProjectionMatrix = {};
glm::mat4 g_currentJitteredProjectionMatrix = {};

uint32_t g_globalUniformsBuffer = {};
uint32_t g_objectsBuffer = {};
glm::vec3 g_sunPosition = glm::vec3{100, 110, 120};

bool g_drawDebugLines = true;
std::vector<TGpuDebugLine> g_debugLines;
uint32_t g_debugInputLayout = 0;
uint32_t g_debugLinesVertexBuffer = 0;
TGraphicsPipeline g_debugLinesGraphicsPipeline = {};

TGraphicsPipeline g_fstGraphicsPipeline = {};
TSamplerId g_fstSamplerNearestClampToEdge = {};

bool g_isFxaaEnabled = false;
bool g_isTaaEnabled = true;
float g_taaBlendFactor = 0.1f;

glm::vec2 g_scaledFramebufferSize = {};

glm::ivec2 g_windowFramebufferSize = {};
glm::ivec2 g_windowFramebufferScaledSize = {};
bool g_windowFramebufferResized = false;

glm::ivec2 g_sceneViewerSize = {};
glm::ivec2 g_sceneViewerScaledSize = {};
bool g_sceneViewerResized = false;
bool g_isEditor = false;

entt::entity g_selectedEntity = entt::null;

/*
 * UI/ImGui
 */
constexpr auto g_uiUnitX = ImVec2(1, 0);
constexpr auto g_uiUnitY = ImVec2(0, 1);
ImGuiContext* g_uiContext = nullptr;
ImFont* g_uiFontRobotoRegular = nullptr;
ImFont* g_uiFontRobotoRegularSmaller = nullptr;
ImFont* g_uiFontRobotoBold = nullptr;
ImFont* g_uiFontJetbrainsMonoRegular = nullptr;
auto UiInitialize(GLFWwindow* window) -> bool;
auto UiUnload() -> void;
auto UiRender(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry) -> void;auto ImGuiSetDarkStyle() -> void;
auto UiMagnifier(
    Renderer::TRenderContext& renderContext,
    const glm::vec2 viewportContentSize,
    const glm::vec2 viewportContentOffset,
    const bool isViewportHovered) -> void;
auto UiSetDarkStyle() -> void;
auto UiSetValveStyle() -> void;
auto UiSetDarkWithFuchsiaStyle() -> void;

/*
 * Jitter
 */

static constexpr int32_t g_jitterCount = 16;
glm::vec2 g_jitter[g_jitterCount];
int32_t g_jitterIndex = 0;

entt::entity g_selectedEntity = entt::null;

auto GetHaltonSequence(
    const int prime,
    const int index = 1) -> float {

    float r = 0.f;
    float f = 1.f;
    int i = index;
    while (i > 0)
    {
        f /= prime;
        r += f * (i % prime);
        i = static_cast<int>(floor(i / static_cast<float>(prime)));
    }
    return r;
}

struct TGpuVertexPosition {
    glm::vec3 Position;
};

struct TGpuPackedVertexNormalTangentUvTangentSign {
    uint32_t Normal;
    uint32_t Tangent;
    glm::vec4 UvAndTangentSign;
};

struct TGpuVertexPositionColor {
    glm::vec3 Position;
    glm::vec4 Color;
};

struct TGpuMesh {
    std::string_view Name;
    uint32_t VertexPositionBuffer;
    uint32_t VertexNormalUvTangentBuffer;
    uint32_t IndexBuffer;

    size_t VertexCount;
    size_t IndexCount;

    glm::mat4 InitialTransform;
};

struct TCpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = base color factor .y = normal strength, .z = metalness, .w = roughness

    uint32_t BaseColorTextureId;
    uint32_t BaseColorTextureSamplerId;
    uint32_t NormalTextureId;
    uint32_t NormalTextureSamplerId;

    uint32_t ArmTextureId;
    uint32_t ArmTextureSamplerId;
    uint32_t EmissiveTextureId;
    uint32_t EmissiveTextureSamplerId;
};

struct TGpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = base color factor .y = normal strength, .z = metalness, .w = roughness

    uint64_t BaseColorTexture;
    uint64_t NormalTexture;

    uint64_t ArmTexture;
    uint64_t EmissiveTexture;

    uint32_t BaseColorTextureId;
    uint32_t NormalTextureId;
};

struct TCpuGlobalLight {

    float Azimuth;
    float Elevation;
    glm::vec3 Color;
    float Strength;

    int32_t ShadowMapSize;
    float ShadowMapNear;
    float ShadowMapFar;

    bool ShowFrustum;
};

std::vector<TCpuGlobalLight> g_globalLights;
bool g_globalLightsWasModified = true;

std::unordered_map<std::string, TGpuMesh> g_gpuMeshes = {};
std::unordered_map<std::string, TSampler> g_gpuSamplers = {};
std::unordered_map<std::string, TCpuMaterial> g_cpuMaterials = {};
std::unordered_map<std::string, TGpuMaterial> g_gpuMaterials = {};

auto LoadSkyTexture(const std::string& skyBoxName) -> TTextureId {

    const std::array<std::string, 6> skyBoxNames = {
        std::format("data/sky/TC_{}_Xp.png", skyBoxName),
        std::format("data/sky/TC_{}_Xn.png", skyBoxName),
        std::format("data/sky/TC_{}_Yp.png", skyBoxName),
        std::format("data/sky/TC_{}_Yn.png", skyBoxName),
        std::format("data/sky/TC_{}_Zp.png", skyBoxName),
        std::format("data/sky/TC_{}_Zn.png", skyBoxName),
    };

    TTextureId textureId = {};

    //EnableFlipImageVertically();
    for (auto imageIndex = 0; imageIndex < skyBoxNames.size(); imageIndex++) {

        auto imageName = skyBoxNames[imageIndex];
        int32_t imageWidth = 0;
        int32_t imageHeight = 0;
        int32_t imageComponents = 0;
        const auto imageData = Image::LoadImageFromFile(imageName, &imageWidth, &imageHeight, &imageComponents);

        if (imageIndex == 0) {
            textureId = CreateTexture(TCreateTextureDescriptor{
                .TextureType = TTextureType::TextureCube,
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent3D{ static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1u},
                .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageWidth), static_cast<float>(imageHeight))))),
                .Layers = 6,
                .SampleCount = TSampleCount::One,
                .Label = skyBoxName,
            });
        }

        UploadTexture(textureId, TUploadTextureDescriptor{
            .Level = 0,
            .Offset = TOffset3D{0, 0, static_cast<uint32_t>(imageIndex)},
            .Extent = TExtent3D{static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1u},
            .UploadFormat = TUploadFormat::Auto,
            .UploadType = TUploadType::Auto,
            .PixelData = imageData
        });

        Image::FreeImage(imageData);
    }
    //DisableFlipImageVertically();

    GenerateMipmaps(textureId);

    return textureId;
}

auto SignNotZero(const glm::vec2 v) -> glm::vec2 {

    return glm::vec2((v.x >= 0.0f) ? +1.0f : -1.0f, (v.y >= 0.0f) ? +1.0f : -1.0f);
}

auto EncodeNormal(const glm::vec3 normal) -> glm::vec2 {

    const auto encodedNormal = glm::vec2{normal.x, normal.y} * (1.0f / (abs(normal.x) + abs(normal.y) + abs(normal.z)));
    return (normal.z <= 0.0f)
           ? (1.0f - glm::abs(glm::vec2{encodedNormal.x, encodedNormal.y})) * SignNotZero(encodedNormal) //TODO(deccer) check if its encNor.y, encNor.x or not
           : encodedNormal;
}

auto RendererCreateGpuMesh(
    const Assets::TAssetPrimitive& assetPrimitive,
    const std::string& label) -> void {

    std::vector<TGpuVertexPosition> vertexPositions;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> vertexNormalUvTangents;
    vertexPositions.resize(assetPrimitive.Positions.size());
    vertexNormalUvTangents.resize(assetPrimitive.Positions.size());

    for (size_t i = 0; i < assetPrimitive.Positions.size(); i++) {
        vertexPositions[i] = {
            assetPrimitive.Positions[i],
        };

        vertexNormalUvTangents[i] = {
            glm::packSnorm2x16(EncodeNormal(assetPrimitive.Normals[i])),
            glm::packSnorm2x16(EncodeNormal(assetPrimitive.Tangents[i].xyz())),
            glm::vec4(assetPrimitive.Uvs[i], assetPrimitive.Tangents[i].w, 0.0f),
        };
    }

    uint32_t buffers[3] = {};
    {
        PROFILER_ZONESCOPEDN("Create GL Buffers + Upload Data");

        glCreateBuffers(3, buffers);
        SetDebugLabel(buffers[0], GL_BUFFER, std::format("{}_position", label));
        SetDebugLabel(buffers[1], GL_BUFFER, std::format("{}_normal_uv_tangent", label));
        SetDebugLabel(buffers[2], GL_BUFFER, std::format("{}_indices", label));
        glNamedBufferStorage(buffers[0], sizeof(TGpuVertexPosition) * vertexPositions.size(),
                                vertexPositions.data(), 0);
        glNamedBufferStorage(buffers[1], sizeof(TGpuPackedVertexNormalTangentUvTangentSign) * vertexNormalUvTangents.size(),
                                vertexNormalUvTangents.data(), 0);
        glNamedBufferStorage(buffers[2], sizeof(uint32_t) * assetPrimitive.Indices.size(), assetPrimitive.Indices.data(), 0);
    }

    const auto gpuMesh = TGpuMesh{
        .Name = label,
        .VertexPositionBuffer = buffers[0],
        .VertexNormalUvTangentBuffer = buffers[1],
        .IndexBuffer = buffers[2],

        .VertexCount = vertexPositions.size(),
        .IndexCount = assetPrimitive.Indices.size(),
    };

    {
        PROFILER_ZONESCOPEDN("Add Gpu Mesh");
        g_gpuMeshes[label] = gpuMesh;
    }
}

auto GetGpuMesh(const std::string& assetMeshName) -> TGpuMesh& {
    assert(!assetMeshName.empty());

    return g_gpuMeshes[assetMeshName];
}

auto GetCpuMaterial(const std::string& assetMaterialName) -> TCpuMaterial& {
    assert(!assetMaterialName.empty());

    return g_cpuMaterials[assetMaterialName];
}

auto GetGpuMaterial(const std::string& assetMaterialName) -> TGpuMaterial& {
    assert(!assetMaterialName.empty());

    return g_gpuMaterials[assetMaterialName];
}

auto ConvolveTextureCube(const TTextureId textureId) -> std::expected<TTextureId, std::string> {

    auto convolveComputePipelineResult = CreateComputePipeline(TComputePipelineDescriptor{
        .Label = "ConvolveTexture",
        .ComputeShaderFilePath = "data/shaders/ConvolveTexture.cs.glsl",
    });

    if (!convolveComputePipelineResult) {
        return std::unexpected(convolveComputePipelineResult.error());
    }

    const auto& environmentTexture = GetTexture(textureId);

    auto convolveComputePipeline = *convolveComputePipelineResult;

    constexpr auto width = 128;
    constexpr auto height = 128;
    constexpr auto format = TFormat::R16G16B16A16_FLOAT;
    
    auto convolvedTextureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::TextureCube,
        .Format = format,
        .Extent = TExtent3D(width, height, 1),
        .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(width), static_cast<float>(height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = std::format("TextureCube-{}x{}-Irradiance", width, height),
    });

    const auto& convolvedTexture = GetTexture(convolvedTextureId);

    constexpr auto groupX = static_cast<uint32_t>(width / 32);
    constexpr auto groupY = static_cast<uint32_t>(height / 32);
    constexpr auto groupZ = 6u;

    convolveComputePipeline.Bind();
    convolveComputePipeline.BindTexture(0, environmentTexture.Id);
    convolveComputePipeline.BindImage(0, convolvedTexture.Id, 0, 0, TMemoryAccess::WriteOnly, format);
    convolveComputePipeline.Dispatch(groupX, groupY, groupZ);

    convolveComputePipeline.InsertMemoryBarrier(TMemoryBarrierMaskBits::ShaderImageAccess);
    GenerateMipmaps(convolvedTextureId);

    return convolvedTextureId;
}

auto ComputeSHCoefficients(const TTextureId textureId) -> std::expected<uint32_t, std::string> {

    auto computeSHCoefficientsComputePipelineResult = CreateComputePipeline(TComputePipelineDescriptor{
        .Label = "ComputeSHCoefficients",
        .ComputeShaderFilePath = "data/shaders/ComputeSHCoefficients.cs.glsl",
    });

    if (!computeSHCoefficientsComputePipelineResult) {
        return std::unexpected(computeSHCoefficientsComputePipelineResult.error());
    }

    auto computeSHCoefficientsComputePipeline = *computeSHCoefficientsComputePipelineResult;

    constexpr auto faceSize = 128;

    auto shCoefficientBuffer = CreateBuffer("CoefficientBuffer", sizeof(float) * 3 * 9, nullptr, GL_DYNAMIC_STORAGE_BIT);
    constexpr auto zeroValue = 0.0f;
    UpdateBuffer(shCoefficientBuffer, 0, sizeof(float) * 3 * 9, &zeroValue);

    const auto& convolvedTexture = GetTexture(textureId);

    constexpr auto groupX = static_cast<uint32_t>(faceSize / 8);
    constexpr auto groupY = static_cast<uint32_t>(faceSize / 8);
    constexpr auto groupZ = 6u;

    computeSHCoefficientsComputePipeline.Bind();
    computeSHCoefficientsComputePipeline.BindTexture(0, convolvedTexture.Id);
    computeSHCoefficientsComputePipeline.BindBufferAsShaderStorageBuffer(shCoefficientBuffer, 1);
    computeSHCoefficientsComputePipeline.SetUniform(0, faceSize);
    computeSHCoefficientsComputePipeline.Dispatch(groupX, groupY, groupZ);

    computeSHCoefficientsComputePipeline.InsertMemoryBarrier(TMemoryBarrierMaskBits::ShaderImageAccess);

    return shCoefficientBuffer;
}

auto CreateResidentTextureForMaterialChannel(const std::string& materialDataName) -> int64_t {

    PROFILER_ZONESCOPEDN("CreateResidentTextureForMaterialChannel");

    const auto& imageData = Assets::GetAssetImage(materialDataName);

    const auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = imageData.Name,
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0},
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = imageData.Data.get()
    });

    GenerateMipmaps(textureId);

    //auto& sampler = GetAssetSampler(assetMaterialChannel.Sampler);

    return MakeTextureResident(textureId);
}

auto CreateTextureForMaterialChannel(
    const std::string& imageDataName,
    const Assets::TAssetMaterialChannel channel) -> uint32_t {

    PROFILER_ZONESCOPEDN("CreateTextureForMaterialChannel");

    const auto& imageData = Assets::GetAssetImage(imageDataName);

    const auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = channel == Assets::TAssetMaterialChannel::Color ? TFormat::R8G8B8A8_SRGB : TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = imageData.Name,
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0},
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = imageData.Data.get()
    });

    GenerateMipmaps(textureId);

    //auto& sampler = GetAssetSampler(assetMaterialChannel.Sampler);

    return GetTexture(textureId).Id;
}

constexpr auto ToAddressMode(const Assets::TAssetSamplerWrapMode wrapMode) -> TTextureAddressMode {
    switch (wrapMode) {
        case Assets::TAssetSamplerWrapMode::ClampToEdge: return TTextureAddressMode::ClampToEdge;
        case Assets::TAssetSamplerWrapMode::MirroredRepeat: return TTextureAddressMode::RepeatMirrored;
        case Assets::TAssetSamplerWrapMode::Repeat: return TTextureAddressMode::Repeat;
        default: std::unreachable();
    }
}

constexpr auto ToMagFilter(const std::optional<Assets::TAssetSamplerMagFilter> magFilter) -> TTextureMagFilter {
    if (!magFilter.has_value()) {
        return TTextureMagFilter::Linear;
    }

    switch (*magFilter) {
        case Assets::TAssetSamplerMagFilter::Linear: return TTextureMagFilter::Linear;
        case Assets::TAssetSamplerMagFilter::Nearest: return TTextureMagFilter::Nearest;
        default: std::unreachable();
    }
}

constexpr auto ToMinFilter(const std::optional<Assets::TAssetSamplerMinFilter> minFilter) -> TTextureMinFilter {
    if (!minFilter.has_value()) {
        return TTextureMinFilter::Linear;
    }
    
    switch (*minFilter) {
        case Assets::TAssetSamplerMinFilter::Linear: return TTextureMinFilter::Linear;
        case Assets::TAssetSamplerMinFilter::Nearest: return TTextureMinFilter::Nearest;
        case Assets::TAssetSamplerMinFilter::NearestMipMapLinear: return TTextureMinFilter::NearestMipmapLinear;
        case Assets::TAssetSamplerMinFilter::NearestMipMapNearest: return TTextureMinFilter::NearestMipmapNearest;
        case Assets::TAssetSamplerMinFilter::LinearMipMapNearest: return TTextureMinFilter::LinearMipmapNearest;
        case Assets::TAssetSamplerMinFilter::LinearMipMapLinear: return TTextureMinFilter::LinearMipmapLinear;
        default: std::unreachable();
    }
}

auto CreateSamplerDescriptor(const Assets::TAssetSampler& assetSampler) -> TSamplerDescriptor {

    return TSamplerDescriptor{
        .Label = assetSampler.Name,
        .AddressModeU = ToAddressMode(assetSampler.WrapS),
        .AddressModeV = ToAddressMode(assetSampler.WrapT),
        .MagFilter = ToMagFilter(assetSampler.MagFilter),
        .MinFilter = ToMinFilter(assetSampler.MinFilter),
    };
}

auto RendererCreateCpuMaterial(const std::string& assetMaterialName) -> void {

    PROFILER_ZONESCOPEDN("CreateCpuMaterial");

    if (g_cpuMaterials.contains(assetMaterialName)) {
        return;
    }

    const auto& assetMaterialData = Assets::GetAssetMaterial(assetMaterialName);

    auto cpuMaterial = TCpuMaterial{
        .BaseColor = assetMaterialData.BaseColor,
    };

    const auto& baseColorChannel = assetMaterialData.BaseColorTextureChannel;
    if (baseColorChannel.has_value()) {
        const auto& baseColor = *baseColorChannel;
        const auto& baseColorSampler = Assets::GetAssetSampler(baseColor.SamplerName);

        cpuMaterial.BaseColorTextureId = CreateTextureForMaterialChannel(baseColor.TextureName, baseColor.Channel);
        const auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(baseColorSampler));
        const auto& sampler = GetSampler(samplerId);
        cpuMaterial.BaseColorTextureSamplerId = sampler.Id;
    }

    const auto& normalTextureChannel = assetMaterialData.NormalTextureChannel;
    if (normalTextureChannel.has_value()) {
        const auto& normalTexture = *normalTextureChannel;
        const auto& normalTextureSampler = Assets::GetAssetSampler(normalTexture.SamplerName);

        cpuMaterial.NormalTextureId = CreateTextureForMaterialChannel(normalTexture.TextureName, normalTexture.Channel);
        const auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(normalTextureSampler));
        const auto& sampler = GetSampler(samplerId);
        cpuMaterial.NormalTextureSamplerId = sampler.Id;
    }

    g_cpuMaterials[assetMaterialName] = cpuMaterial;
}

auto DeleteRendererFramebuffers() -> void {

    DeleteFramebuffer(g_depthPrePassFramebuffer);
    DeleteFramebuffer(g_geometryFramebuffer);
    DeleteFramebuffer(g_resolveGeometryFramebuffer);
    DeleteFramebuffer(g_fxaaFramebuffer);
    DeleteFramebuffer(g_taaFramebuffer1);
    DeleteFramebuffer(g_taaFramebuffer2);
}

auto CreateRendererFramebuffers(const glm::vec2& scaledFramebufferSize) -> void {

    PROFILER_ZONESCOPEDN("CreateRendererFramebuffers");

    g_depthPrePassFramebuffer = CreateFramebuffer({
        .Label = "Depth PrePass FBO",
        .DepthStencilAttachment = TFramebufferDepthStencilAttachmentDescriptor{
            .Label = "Depth PrePass FBO Depth",
            .Format = TFormat::D24_UNORM_S8_UINT,
            .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
            .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
            .ClearDepthStencil = { 1.0f, 0 },
        }
    });

    g_geometryFramebuffer = CreateFramebuffer({
        .Label = "Geometry FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "Geometry FBO Albedo",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = "Geometry FBO Normals",
                .Format = TFormat::R32G32B32A32_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = "Geometry FBO TAA Velocity",
                .Format = TFormat::R16G16B16A16_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Load,
            }
        },
        .DepthStencilAttachment = TFramebufferExistingDepthStencilAttachmentDescriptor{
            .ExistingDepthTexture = g_depthPrePassFramebuffer.DepthStencilAttachment.value().Texture,
        }
    });

    g_resolveGeometryFramebuffer = CreateFramebuffer({
        .Label = "ResolveGeometry FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "ResolveGeometry FBO Output",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        },
    });

    g_fxaaFramebuffer = CreateFramebuffer({
        .Label = "FXAA FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "FXAA FBO Output",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f }
            }
        }
    });

    g_taaFramebuffer1 = CreateFramebuffer({
        .Label = "TAA1 FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "TAA1 FBO History",
                .Format = TFormat::R16G16B16A16_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            }
        }
    });

    g_taaFramebuffer2 = CreateFramebuffer({
        .Label = "TAA2 FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "TAA2 FBO History",
                .Format = TFormat::R16G16B16A16_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            }
        }
    });
}

auto Renderer::Initialize(
    const TWindowSettings& windowSettings,
    GLFWwindow* window,
    const glm::vec2& initialFramebufferSize) -> bool {

    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        return false;
    }

    RhiInitialize(windowSettings.IsDebug);

    g_windowSettings = windowSettings;

#ifdef USE_PROFILER
    TracyGpuContext
#endif

    if (!UiInitialize(window)) {
        return false;
    }

    glfwSwapInterval(g_windowSettings.IsVSyncEnabled ? 1 : 0);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.05f, 0.07f, 1.0f);

    Assets::AddDefaultAssets();

    auto skyTextureId = LoadSkyTexture("Miramar");
    const auto convolveSkyTextureResult = ConvolveTextureCube(skyTextureId);
    if (!convolveSkyTextureResult) {
        spdlog::error("Unable to convolve sky surface texture {}", convolveSkyTextureResult.error());
        return false;
    }
    g_skyBoxTexture = GetTexture(skyTextureId);
    g_skyBoxConvolvedTexture = GetTexture(*convolveSkyTextureResult);

    const auto computeSHCoefficientsResult = ComputeSHCoefficients(*convolveSkyTextureResult);
    if (!computeSHCoefficientsResult) {
        spdlog::error("Unable to compute spherical harmonics coefficients {}", computeSHCoefficientsResult.error());
        return false;
    }
    g_skyBoxSHCoefficients = *computeSHCoefficientsResult;

    /*
     * Renderer - Initialize Framebuffers
     */
    g_scaledFramebufferSize = initialFramebufferSize;
    CreateRendererFramebuffers(g_scaledFramebufferSize);

    /*
     * Renderer - Initialize Pipelines
     */
    auto depthPrePassGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Depth PrePass",
        .VertexShaderFilePath = "data/shaders/DepthPrePass.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/Depth.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
            .IsDepthBiasEnabled = false,
            .IsScissorEnabled = false,
            .IsRasterizerDisabled = false
        },
        .OutputMergerState = {
            .ColorMask = TColorMaskBits::None,
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = true,
                .DepthFunction = TDepthFunction::Less
            },
        }
    });
    g_depthPrePassGraphicsPipeline = *depthPrePassGraphicsPipelineResult;

    auto geometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "GeometryPipeline",
        .VertexShaderFilePath = "data/shaders/SimpleDeferred.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/SimpleDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = false,
                .DepthFunction = TDepthFunction::Equal,
            }
        }
    });
    if (!geometryGraphicsPipelineResult) {
        spdlog::error(geometryGraphicsPipelineResult.error());
        return false;
    }
    g_geometryGraphicsPipeline = *geometryGraphicsPipelineResult;

    auto resolveGeometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "ResolveGeometryPipeline",
        .VertexShaderFilePath = "data/shaders/ResolveDeferred.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/ResolveDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
            }
        }
    });
    if (!resolveGeometryGraphicsPipelineResult) {
        spdlog::error(resolveGeometryGraphicsPipelineResult.error());
        return false;
    }
    g_resolveGeometryGraphicsPipeline = *resolveGeometryGraphicsPipelineResult;

    auto fstGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FST",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.GammaCorrected.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles
        },
    });

    if (!fstGraphicsPipelineResult) {
        spdlog::error(fstGraphicsPipelineResult.error());
        return false;
    }
    g_fstGraphicsPipeline = *fstGraphicsPipelineResult;

    auto debugLinesGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "DebugLinesPipeline",
        .VertexShaderFilePath = "data/shaders/DebugLines.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/DebugLines.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Lines,
            .IsPrimitiveRestartEnabled = false,
        },
        .VertexInput = TVertexInputDescriptor{
            .VertexInputAttributes = {
                TVertexInputAttributeDescriptor{
                    .Location = 0,
                    .Binding = 0,
                    .Format = TFormat::R32G32B32_FLOAT,
                    .Offset = offsetof(TGpuDebugLine, StartPosition),
                },
                TVertexInputAttributeDescriptor{
                    .Location = 1,
                    .Binding = 0,
                    .Format = TFormat::R32G32B32A32_FLOAT,
                    .Offset = offsetof(TGpuDebugLine, StartColor),
                },
            }
        }
    });

    if (!debugLinesGraphicsPipelineResult) {
        spdlog::error(debugLinesGraphicsPipelineResult.error());
        return false;
    }
    g_debugLinesGraphicsPipeline = *debugLinesGraphicsPipelineResult;

    auto fxaaGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FXAA",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
    });
    if (!fxaaGraphicsPipelineResult) {
        spdlog::error(fxaaGraphicsPipelineResult.error());
        return false;
    }

    g_fxaaGraphicsPipeline = *fxaaGraphicsPipelineResult;

    auto taaGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "TAA",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/TAA.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
    });
    if (!taaGraphicsPipelineResult) {
        spdlog::error(taaGraphicsPipelineResult.error());
        return false;
    }

    g_taaGraphicsPipeline = *taaGraphicsPipelineResult;

    g_taaSampler = GetOrCreateSampler(TSamplerDescriptor{
        .Label = "TAA Sampler",
        .AddressModeU = TTextureAddressMode::ClampToEdge,
        .AddressModeV = TTextureAddressMode::ClampToEdge,
        .MagFilter = TTextureMagFilter::Linear,
        .MinFilter = TTextureMinFilter::Linear,
    });

    g_debugLinesVertexBuffer = CreateBuffer("VertexBuffer-DebugLines", sizeof(TGpuDebugLine) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    for (int i = 0; i < g_jitterCount; ++i) {
        g_jitter[i].x = GetHaltonSequence(2, i + 1) - 0.5f;
        g_jitter[i].y = GetHaltonSequence(3, i + 1) - 0.5f;
    }

    auto cameraPosition = glm::vec3{-60.0f, -3.0f, 0.0f};
    auto cameraDirection = glm::vec3{0.0f, 0.0f, -1.0f};
    auto cameraUp = glm::vec3{0.0f, 1.0f, 0.0f};
    auto fieldOfView = glm::radians(160.0f);
    auto aspectRatio = g_scaledFramebufferSize.x / g_scaledFramebufferSize.y;

    g_globalUniforms.ProjectionMatrix = glm::infinitePerspective(fieldOfView, aspectRatio, 0.1f);
    g_globalUniforms.ViewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp);
    g_globalUniforms.CurrentJitteredViewProjectionMatrix = g_globalUniforms.ProjectionMatrix * g_globalUniforms.ViewMatrix;
    g_globalUniforms.PreviousJitteredViewProjectionMatrix = g_globalUniforms.ProjectionMatrix * g_globalUniforms.ViewMatrix;
    g_globalUniforms.CameraPosition = glm::vec4{cameraPosition, fieldOfView};
    g_globalUniforms.CameraDirection = glm::vec4{cameraDirection, aspectRatio};

    g_previousViewMatrix = g_globalUniforms.ViewMatrix;
    g_previousJitteredProjectionMatrix = g_globalUniforms.ProjectionMatrix;

    g_globalUniformsBuffer = CreateBuffer("TGpuGlobalUniforms", sizeof(TGpuGlobalUniforms), &g_globalUniforms, GL_DYNAMIC_STORAGE_BIT);
    g_objectsBuffer = CreateBuffer("TGpuObjects", sizeof(TGpuObject) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    g_gpuGlobalLights.push_back(TGpuGlobalLight{
        .ProjectionMatrix = glm::mat4(1.0f),
        .ViewMatrix = glm::mat4(1.0f),
        .Direction = glm::vec4(10.0f, 20.0f, 10.0f, 0.0f),
        .Strength = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
    });

    g_globalLightsBuffer = CreateBuffer("TGpuGlobalLights", g_gpuGlobalLights.size() * sizeof(TGpuGlobalLight), g_gpuGlobalLights.data(), GL_DYNAMIC_STORAGE_BIT);

    g_fstSamplerNearestClampToEdge = GetOrCreateSampler({
        .AddressModeU = TTextureAddressMode::ClampToEdge,
        .AddressModeV = TTextureAddressMode::ClampToEdge,
        .MagFilter = TTextureMagFilter::Nearest,
        .MinFilter = TTextureMinFilter::Nearest
    });

    return true;
}

auto Renderer::Unload() -> void {

    DeleteBuffer(g_globalLightsBuffer);
    DeleteBuffer(g_globalUniformsBuffer);
    DeleteBuffer(g_objectsBuffer);

    DeleteRendererFramebuffers();

    DeletePipeline(g_debugLinesGraphicsPipeline);
    DeletePipeline(g_fstGraphicsPipeline);

    DeletePipeline(g_depthPrePassGraphicsPipeline);
    DeletePipeline(g_geometryGraphicsPipeline);
    DeletePipeline(g_resolveGeometryGraphicsPipeline);
    DeletePipeline(g_fxaaGraphicsPipeline);
    DeletePipeline(g_taaGraphicsPipeline);

    UiUnload();

    RhiShutdown();
}

auto RenderEntityHierarchy(
    entt::registry& registry,
    entt::entity entity) -> void {

    const auto& entityName = registry.get<TComponentName>(entity);
    const auto* icon = registry.any_of<TComponentMesh>(entity)
        ? (char*)ICON_MDI_CUBE
        : (char*)ICON_MDI_CUBE_OUTLINE;
    ImGui::PushID(static_cast<int32_t>(entity));

    auto treeNodeFlags = g_selectedEntity == entity
        ? ImGuiTreeNodeFlags_Selected
        : 0;
    treeNodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (registry.all_of<TComponentHierarchy>(entity)) {

        bool isOpen = false;
        bool isRootEntity = false;
        if (entity == static_cast<entt::entity>(0)) {
            isOpen = true;
            isRootEntity = true;
        } else {
            isOpen = ImGui::TreeNodeEx(std::format("{}##{}", icon, HashString(entityName.Name)).data(), treeNodeFlags);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                g_selectedEntity = entity;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(entityName.Name.data());
        }
        if (isOpen) {
            auto& children = registry.get<TComponentHierarchy>(entity).Children;
            for (auto& child : children) {
                RenderEntityHierarchy(registry, child);
            }
            if (!isRootEntity) {
                ImGui::TreePop();
            }
        }
    } else {

        treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
        ImGui::TreeNodeEx(std::format("{}##{}", icon, HashString(entityName.Name)).data(), treeNodeFlags);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            g_selectedEntity = entity;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(entityName.Name.data());

        ImGui::TreePop();
        
    }

    ImGui::PopID();
}

auto RenderTransformComponent(
    const char* name,
    glm::vec3& vector,
    const float width,
    const float defaultElementValue) {

    const float labelIndentation = ImGui::GetFontSize();
    bool updated = false;

    auto& style = ImGui::GetStyle();

    const auto showFloat = [&](int axis, float* value)
    {
        const float labelFloatSpacing = ImGui::GetFontSize();
        const float step = 0.01f;
        static const std::string format = "%.4f";

        ImGui::AlignTextToFramePadding();
        if(ImGui::Button(axis == 0 ? "X  " : axis == 1 ? "Y  " : "Z  "))
        {
            *value  = defaultElementValue;
            updated = true;
        }

        ImGui::SameLine(labelFloatSpacing);
        ImVec2 posPostLabel = ImGui::GetCursorScreenPos();

        ImGui::PushItemWidth(width);
        ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));

        if(ImGui::InputFloat("##no_label", value, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), format.c_str())) {
            updated = true;
        }

        ImGui::PopID();
        ImGui::PopItemWidth();

        static const ImU32 colourX = IM_COL32(168, 46, 2, 255);
        static const ImU32 colourY = IM_COL32(112, 162, 22, 255);
        static const ImU32 colourZ = IM_COL32(51, 122, 210, 255);

        const auto size = ImVec2(ImGui::GetFontSize() / 4.0f, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f);
        posPostLabel = ImVec2(posPostLabel.x - 1.0f, posPostLabel.y + 0.0f); // ImGui::GetStyle().FramePadding.y / 2.0f);
        const auto axisColorRectangle = ImRect(posPostLabel.x, posPostLabel.y, posPostLabel.x + size.x, posPostLabel.y + size.y);
        ImGui::GetWindowDrawList()->AddRectFilled(axisColorRectangle.Min, axisColorRectangle.Max, axis == 0 ? colourX : axis == 1 ? colourY : colourZ);
    };

    ImGui::BeginGroup();
    ImGui::Indent(labelIndentation);
    ImGui::TextUnformatted(name);
    ImGui::Unindent(labelIndentation);
    showFloat(0, &vector.x);
    showFloat(1, &vector.y);
    showFloat(2, &vector.z);
    ImGui::EndGroup();

    return updated;
}

auto RenderEntityProperties(
    entt::registry& registry,
    const entt::entity entity) -> void {

    if (registry.all_of<TComponentName>(entity)) {

        if (ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Name", ImGuiTreeNodeFlags_Leaf)) {

            ImGui::Indent();
            auto& name = registry.get<TComponentName>(entity);

            ImGui::PushItemWidth(-1.0f);
            ImGui::InputText("##Name", name.Name.data(), name.Name.size());
            ImGui::PopItemWidth();

            ImGui::Unindent();
        }
    }

    if (registry.all_of<TComponentHierarchy>(entity)) {

        if (ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Parent", ImGuiTreeNodeFlags_Leaf)) {

            ImGui::Indent();
            auto& hierarchy = registry.get<TComponentHierarchy>(entity);
            auto& parentName = registry.get<TComponentName>(hierarchy.Parent);

            ImGui::TextUnformatted(parentName.Name.data());
            ImGui::Unindent();
        }
    }

    if (registry.all_of<TComponentTransform>(entity)) {

        if (ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Transform", ImGuiTreeNodeFlags_Leaf)) {

            ImGui::AlignTextToFramePadding();
            float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;                

            ImGui::Indent();
            if (registry.any_of<TComponentPosition>(entity)) {

                auto& position = registry.get<TComponentPosition>(entity);

                glm::vec3 tempPosition = position;
                if (ImGui::InputFloat3("Position", &tempPosition.x)) {
                    position = tempPosition;
                }
            }

            if (registry.any_of<TComponentOrientationEuler>(entity)) {
                auto& eulerAngles = registry.get<TComponentOrientationEuler>(entity);

                glm::vec3 tempRotation = {eulerAngles.Pitch, eulerAngles.Yaw, eulerAngles.Roll};
                if (ImGui::InputFloat3("Rotation", &tempRotation.x)) {
                    eulerAngles.Pitch = tempRotation.x;
                    eulerAngles.Yaw = tempRotation.y;
                    eulerAngles.Roll = tempRotation.z;
                }
            }

            if (registry.any_of<TComponentScale>(entity)) {
                auto& scale = registry.get<TComponentScale>(entity);

                glm::vec3 tempScale = scale;
                if (ImGui::InputFloat3("Scale", &tempScale.x)) {
                    scale = tempScale;
                }
            }
            ImGui::Unindent();
        }
    }    

    if (registry.all_of<TComponentCamera>(entity)) {

        if (ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Camera", ImGuiTreeNodeFlags_Leaf)) {
            auto& camera = registry.get<TComponentCamera>(entity);

            ImGui::Indent();
            ImGui::Unindent();
        }
    }

    if (registry.all_of<TComponentMesh>(entity)) {

        if (ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Mesh", ImGuiTreeNodeFlags_Leaf)) {
            const auto& mesh = registry.get<TComponentMesh>(entity);

            ImGui::Indent();
            ImGui::PushItemWidth(-1.0f);
            ImGui::TextUnformatted(mesh.Mesh.data());
            ImGui::PopItemWidth();
            ImGui::Unindent();
        }
    }

    if (registry.all_of<TComponentMaterial>(entity)) {

        if (ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Material", ImGuiTreeNodeFlags_Leaf)) {
            const auto& material = registry.get<TComponentMaterial>(entity);

            ImGui::Indent();
            ImGui::PushItemWidth(-1.0f);
            ImGui::TextUnformatted(material.Material.data());
            ImGui::PopItemWidth();
            ImGui::Unindent();
        }
    }    
}

auto UpdateAllTransforms(entt::registry& registry) -> void{
    const auto view = registry.view<TComponentPosition, TComponentOrientationEuler, TComponentScale, TComponentTransform, TComponentHierarchy>();
    for (auto root : view) {
        const auto& hierarchy = view.get<TComponentHierarchy>(root);
        if (hierarchy.Parent != entt::null) {
            continue;
        }

        std::stack<std::pair<entt::entity, const glm::mat4*>> stack;
        stack.emplace(root, nullptr);

        while (!stack.empty()) {
            auto [entity, parentGlobal] = stack.top();
            stack.pop();

            auto& localPosition = registry.get<TComponentPosition>(entity);
            auto& localOrientation = registry.get<TComponentOrientationEuler>(entity);
            auto& localScale = registry.get<TComponentScale>(entity);
            auto& globalTransform = registry.get<TComponentTransform>(entity);
            auto& renderTransform = registry.get<TComponentRenderTransform>(entity);

            glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), localPosition)
                                  * glm::eulerAngleYXZ(localOrientation.Yaw, localOrientation.Pitch, localOrientation.Roll)
                                  * glm::scale(glm::mat4(1.0f), localScale);

            if (parentGlobal) {
                globalTransform = *parentGlobal * localMatrix;
            } else {
                globalTransform = localMatrix;
            }

            renderTransform = globalTransform;

            const auto& entityHierarchy = registry.get<TComponentHierarchy>(entity);
            for (auto child : entityHierarchy.Children) {
                stack.emplace(child, &globalTransform);
            }
        }
    }
}

bool g_uiIsMagnifierEnabled = true;
float g_uiMagnifierZoom = 1.0f;
glm::vec2 g_uiMagnifierLastCursorPos = {};
glm::vec2 g_uiViewportContentOffset = {};

auto Renderer::Render(
    TRenderContext& renderContext,
    entt::registry& registry) -> void {

    if (g_playerEntity == entt::null) {
        g_playerEntity = registry.view<TComponentCamera>().front();
    }

    {
        PROFILER_ZONESCOPEDN("Create Gpu Resources if necessary");

        /*
         * ECS - Create Gpu Resources if necessary
         */
        const auto createGpuResourcesNecessaryView = registry.view<TComponentCreateGpuResourcesNecessary>();
        for (auto& entity : createGpuResourcesNecessaryView) {

            PROFILER_ZONESCOPEDN("Create Gpu Resource");
            auto& meshComponent = registry.get<TComponentMesh>(entity);
            auto& materialComponent = registry.get<TComponentMaterial>(entity);

            auto& assetPrimitive = Assets::GetAssetPrimitive(meshComponent.Mesh);
            RendererCreateGpuMesh(assetPrimitive, assetPrimitive.Name);
            RendererCreateCpuMaterial(materialComponent.Material);

            registry.emplace<TComponentGpuMesh>(entity, meshComponent.Mesh);
            registry.emplace<TComponentGpuMaterial>(entity, materialComponent.Material);

            registry.remove<TComponentCreateGpuResourcesNecessary>(entity);
        }
    }

    /*
     * ECS - Update Transforms
     */
    UpdateAllTransforms(registry);

    /*
     * Update Global Uniforms
     */
    {
        PROFILER_ZONESCOPEDN("Update Global Uniforms");

        registry.view<TComponentCamera>().each([&](
            const auto& entity,
            const auto& cameraComponent) {

            auto cameraMatrix = registry.get<TComponentRenderTransform>(entity);
            auto viewMatrix = glm::inverse(cameraMatrix);
            const glm::vec3 cameraPosition = cameraMatrix[3];
            const glm::vec3 cameraDirection = cameraMatrix[1];

            auto aspectRatio = g_scaledFramebufferSize.x / g_scaledFramebufferSize.y;
            g_globalUniforms.ProjectionMatrix = glm::infinitePerspective(glm::radians(cameraComponent.FieldOfView), aspectRatio, 0.1f);
            g_globalUniforms.ViewMatrix = glm::inverse(cameraMatrix);

            float jitterX = 0;
            float jitterY = 0;
            if (g_isTaaEnabled)
            {
                jitterX = g_jitter[g_jitterIndex].x;
                jitterY = -g_jitter[g_jitterIndex].y;

                ++g_jitterIndex;
                if (g_jitterIndex >= g_jitterCount) {
                    g_jitterIndex -= g_jitterCount;
                }
            }

            constexpr float jitterScale = 2.0f;
            g_currentJitteredProjectionMatrix = g_globalUniforms.ProjectionMatrix;
            g_currentJitteredProjectionMatrix[2][0] += jitterX * jitterScale / g_scaledFramebufferSize.x;
            g_currentJitteredProjectionMatrix[2][1] += jitterY * jitterScale / g_scaledFramebufferSize.y;

            g_globalUniforms.CurrentJitteredViewProjectionMatrix = g_currentJitteredProjectionMatrix * g_globalUniforms.ViewMatrix;
            g_globalUniforms.PreviousJitteredViewProjectionMatrix = g_previousJitteredProjectionMatrix * g_previousViewMatrix;

            g_globalUniforms.CameraPosition = glm::vec4(cameraPosition, glm::radians(cameraComponent.FieldOfView));
            g_globalUniforms.CameraDirection = glm::vec4(cameraDirection, aspectRatio);
            UpdateBuffer(g_globalUniformsBuffer, 0, sizeof(TGpuGlobalUniforms), &g_globalUniforms);

            g_previousViewMatrix = g_globalUniforms.ViewMatrix;
            g_previousJitteredProjectionMatrix = g_currentJitteredProjectionMatrix;

        });
    }

    /*
     * Resize if necessary
     */
    if (g_windowFramebufferResized || g_sceneViewerResized) {

        PROFILER_ZONESCOPEDN("Resize If Necessary");

        g_windowFramebufferScaledSize = glm::ivec2{g_windowFramebufferSize.x * g_windowSettings.ResolutionScale, g_windowFramebufferSize.y * g_windowSettings.ResolutionScale};
        g_sceneViewerScaledSize = glm::ivec2{g_sceneViewerSize.x * g_windowSettings.ResolutionScale, g_sceneViewerSize.y * g_windowSettings.ResolutionScale};

        if (g_isEditor) {
            g_scaledFramebufferSize = g_sceneViewerScaledSize;
        } else {
            g_scaledFramebufferSize = g_windowFramebufferScaledSize;
        }

        if ((g_scaledFramebufferSize.x * g_scaledFramebufferSize.y) > 0.0f) {
            DeleteRendererFramebuffers();
            CreateRendererFramebuffers(g_scaledFramebufferSize);

            g_windowFramebufferResized = false;
            g_sceneViewerResized = false;
        }
    }

    /*
     * CLEAR DEBUG LINES
     */
    if (g_drawDebugLines) {
        g_debugLines.clear();

        g_debugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{0, 0, 0},
            .StartColor = glm::vec4{0.7f, 0.0f, 0.0f, 1.0f},
            .EndPosition = glm::vec3{128, 0, 0},
            .EndColor = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}
        });
        g_debugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{0, 0, 0},
            .StartColor = glm::vec4{0.0f, 0.7f, 0.0f, 1.0f},
            .EndPosition = glm::vec3{0, 128, 0},
            .EndColor = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}
        });
        g_debugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{0, 0, 0},
            .StartColor = glm::vec4{0.0f, 0.0f, 0.7f, 1.0f},
            .EndPosition = glm::vec3{0, 0, 128},
            .EndColor = glm::vec4{0.0f, 0.0f, 1.0f, 1.0f}
        });
    }

    {
        PROFILER_ZONESCOPEDN("All Depth PrePass Geometry");
        PushDebugGroup("Depth PrePass");
        BindFramebuffer(g_depthPrePassFramebuffer);
        {
            g_depthPrePassGraphicsPipeline.Bind();
            g_depthPrePassGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);

            const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentTransform>();
            renderablesView.each([&](
                const entt::entity& entity,
                const auto& meshComponent,
                const auto& transformComponent) {

                PROFILER_ZONESCOPEDN("Draw PrePass Geometry");

                const auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);
                const auto& t = transformComponent;//EntityGetGlobalTransform(registry, entity);
                g_depthPrePassGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                g_depthPrePassGraphicsPipeline.SetUniform(0, t);

                g_depthPrePassGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
            });
        }
        PopDebugGroup();
    }

    static glm::mat4 selectedObjectMatrix;
    static int32_t counter = 0;

    {
        PROFILER_ZONESCOPEDN("Draw Geometry All");
        PushDebugGroup("Geometry Pass");
        BindFramebuffer(g_geometryFramebuffer);
        {
            g_geometryGraphicsPipeline.Bind();
            g_geometryGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);

            const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentGpuMaterial, TComponentRenderTransform>();
            renderablesView.each([&](
                const entt::entity& entity,
                const auto& meshComponent,
                const auto& materialComponent,
                const auto& transformComponent) {

                PROFILER_ZONESCOPEDN("Draw Geometry");

                auto& cpuMaterial = GetCpuMaterial(materialComponent.GpuMaterial);
                auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                auto worldMatrix = transformComponent;
                if (counter == 1) {
                    selectedObjectMatrix = worldMatrix;
                }
                g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexNormalUvTangentBuffer, 2);
                g_geometryGraphicsPipeline.SetUniform(0, worldMatrix);

                g_geometryGraphicsPipeline.BindTextureAndSampler(8, cpuMaterial.BaseColorTextureId, cpuMaterial.BaseColorTextureSamplerId);
                g_geometryGraphicsPipeline.BindTextureAndSampler(9, cpuMaterial.NormalTextureId, cpuMaterial.NormalTextureSamplerId);

                g_geometryGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
            });
        }
        PopDebugGroup();
    }
    {
        PROFILER_ZONESCOPEDN("Draw ResolveGeometry");
        PushDebugGroup("ResolveGeometry");
        BindFramebuffer(g_resolveGeometryFramebuffer);
        {
            const auto& sampler = GetSampler(g_fstSamplerNearestClampToEdge);

            g_resolveGeometryGraphicsPipeline.Bind();
            //g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);
            //g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(g_globalLightsBuffer, 2);
            g_resolveGeometryGraphicsPipeline.BindTexture(0, g_geometryFramebuffer.ColorAttachments[0]->Texture.Id);
            g_resolveGeometryGraphicsPipeline.BindTexture(1, g_geometryFramebuffer.ColorAttachments[1]->Texture.Id);
            g_resolveGeometryGraphicsPipeline.BindTexture(2, g_depthPrePassFramebuffer.DepthStencilAttachment->Texture.Id);

            g_resolveGeometryGraphicsPipeline.BindTextureAndSampler(8, g_skyBoxTexture.Id, sampler.Id);
            g_resolveGeometryGraphicsPipeline.BindTextureAndSampler(9, g_skyBoxConvolvedTexture.Id, sampler.Id);
            g_resolveGeometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(10, g_skyBoxSHCoefficients);
            g_resolveGeometryGraphicsPipeline.SetUniform(0, g_sunPosition);
            g_resolveGeometryGraphicsPipeline.SetUniform(1, g_globalUniforms.CameraPosition);
            g_resolveGeometryGraphicsPipeline.SetUniform(2, glm::inverse(g_globalUniforms.ProjectionMatrix * g_globalUniforms.ViewMatrix));
            g_resolveGeometryGraphicsPipeline.SetUniform(3, g_scaledFramebufferSize);

            g_resolveGeometryGraphicsPipeline.DrawArrays(0, 3);
        }
        PopDebugGroup();
    }

    /////////////// Debug Lines // move out to some LineRenderer

    if (g_drawDebugLines && !g_debugLines.empty()) {

        PROFILER_ZONESCOPEDN("Draw DebugLines");

        PushDebugGroup("Debug Lines");
        glDisable(GL_CULL_FACE);

        UpdateBuffer(g_debugLinesVertexBuffer, 0, sizeof(TGpuDebugLine) * g_debugLines.size(), g_debugLines.data());

        g_debugLinesGraphicsPipeline.Bind();
        g_debugLinesGraphicsPipeline.BindBufferAsVertexBuffer(g_debugLinesVertexBuffer, 0, 0, sizeof(TGpuDebugLine) / 2);
        g_debugLinesGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);
        g_debugLinesGraphicsPipeline.DrawArrays(0, g_debugLines.size() * 2);

        glEnable(GL_CULL_FACE);
        PopDebugGroup();
    }

    if (g_isFxaaEnabled) {
        PROFILER_ZONESCOPEDN("PostFX FXAA");
        PushDebugGroup("PostFX FXAA");
        BindFramebuffer(g_fxaaFramebuffer);
        {
            g_fxaaGraphicsPipeline.Bind();
            g_fxaaGraphicsPipeline.BindTexture(0, g_resolveGeometryFramebuffer.ColorAttachments[0]->Texture.Id);
            g_fxaaGraphicsPipeline.SetUniform(1, 1);
            g_fxaaGraphicsPipeline.DrawArrays(0, 3);
        }
        PopDebugGroup();
    }

    if (g_isTaaEnabled) {
        PROFILER_ZONESCOPEDN("PostFX TAA");
        PushDebugGroup("PostFX TAA");
        if (g_taaHistoryIndex == 0) {
            BindFramebuffer(g_taaFramebuffer1);
        } else {
            BindFramebuffer(g_taaFramebuffer2);
        }

        const auto& taaFramebuffer = g_taaHistoryIndex == 0
            ? g_taaFramebuffer1
            : g_taaFramebuffer2;

        const auto& taaSampler = GetSampler(g_taaSampler);

        g_taaGraphicsPipeline.Bind();
        g_taaGraphicsPipeline.BindTextureAndSampler(0, g_resolveGeometryFramebuffer.ColorAttachments[0]->Texture.Id, taaSampler.Id); // last color buffer
        g_taaGraphicsPipeline.BindTextureAndSampler(1, taaFramebuffer.ColorAttachments[0]->Texture.Id, taaSampler.Id); // taa history buffer
        g_taaGraphicsPipeline.BindTextureAndSampler(2, g_geometryFramebuffer.ColorAttachments[2]->Texture.Id, taaSampler.Id); // velocity buffer
        g_taaGraphicsPipeline.BindTexture(3, g_geometryFramebuffer.DepthStencilAttachment->Texture.Id); // depth buffer
        g_taaGraphicsPipeline.SetUniform(0, g_taaBlendFactor);
        g_taaGraphicsPipeline.SetUniform(1, 0.1f);
        g_taaGraphicsPipeline.SetUniform(2, 256.0f);

        g_taaGraphicsPipeline.DrawArrays(0, 3);

        PopDebugGroup();

        g_taaHistoryIndex ^= 1;
    }

    /////////////// UI

    PushDebugGroup("UI");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (g_isEditor) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glBlitNamedFramebuffer(g_isFxaaEnabled
                                   ? g_fxaaFramebuffer.Id
                                   : g_isTaaEnabled
                                       ? g_taaHistoryIndex == 0 ? g_taaFramebuffer1.Id : g_taaFramebuffer2.Id
                                       : g_resolveGeometryFramebuffer.Id,
                               0,
                               0, 0, static_cast<int32_t>(g_scaledFramebufferSize.x), static_cast<int32_t>(g_scaledFramebufferSize.y),
                               0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y,
                               GL_COLOR_BUFFER_BIT,
                               GL_NEAREST);
    }

    UiRender(renderContext, registry);
    PopDebugGroup();
}

auto Renderer::ResizeWindowFramebuffer(
    const int32_t width,
    const int32_t height) -> void {

    g_windowFramebufferSize = glm::ivec2{width, height};
    g_windowFramebufferResized = true;
}

auto Renderer::ToggleEditorMode() -> void {
    g_isEditor = !g_isEditor;
    g_windowFramebufferResized = !g_isEditor;
    g_sceneViewerResized = g_isEditor;
}

auto UiUnload() -> void {

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(g_uiContext);
    g_uiContext = nullptr;
}

auto UiInitialize(GLFWwindow* window) -> bool {
    g_uiContext = ImGui::CreateContext();
    constexpr auto fontSize = 18.0f;
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // this little shit doesn't do anything
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->TexGlyphPadding = 1;

    ImFontConfig fontConfig;
    fontConfig.PixelSnapH = true;
    fontConfig.SizePixels = 12.0f;
    fontConfig.OversampleH = 1;
    fontConfig.OversampleV = 1;
    fontConfig.GlyphMinAdvanceX = 4.0f;
    fontConfig.MergeMode = false;

    static const ImWchar fontRanges[] = {
        0x0020,
        0x00FF,
        0x0400,
        0x044F,
        0,
    };

    g_uiFontRobotoRegular = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, fontSize, &fontConfig, fontRanges);
    {
        static constexpr ImWchar iconRange[] = {
            ICON_MIN_MDI,
            ICON_MAX_MDI,
            0
        };
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.GlyphOffset.y = 1.0f;
        iconsConfig.OversampleH = 1;
        iconsConfig.OversampleV = 1;
        iconsConfig.GlyphMinAdvanceX = 4.0f;
        iconsConfig.SizePixels = 12.0f;

        io.Fonts->AddFontFromMemoryCompressedTTF(MaterialDesign_compressed_data, MaterialDesign_compressed_size, fontSize, &iconsConfig, iconRange);
    }
    g_uiFontRobotoBold = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoBold_compressed_data, RobotoBold_compressed_size, fontSize + 2.0f, &fontConfig, fontRanges);
    g_uiFontRobotoRegularSmaller = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, fontSize * 0.8f, &fontConfig, fontRanges);
    g_uiFontJetbrainsMonoRegular = io.Fonts->AddFontFromMemoryCompressedTTF(JetBrainsMono_Regular_compressed_data, JetBrainsMono_Regular_compressed_size, fontSize * 0.8f, &fontConfig, fontRanges);

    io.Fonts->TexGlyphPadding = 1;
    for (int32_t fontConfigIndex = 0; fontConfigIndex < io.Fonts->Sources.Size; fontConfigIndex++) {
        auto* fontSource = &io.Fonts->Sources[fontConfigIndex];
        fontSource->RasterizerMultiply = 1.0f;
    }

    //UiSetDarkStyle();
    //UiSetValveStyle();
    UiSetDarkWithFuchsiaStyle();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        spdlog::error("ImGui: Unable to initialize the GLFW backend");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        spdlog::error("ImGui: Unable to initialize the OpenGL backend");
        return false;
    }

    return true;
}

auto UiRender(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry) -> void {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::BeginFrame();

    if (g_isEditor) {
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    }

    if (!g_isEditor) {
        ImGui::SetNextWindowPos({32, 32});
        ImGui::SetNextWindowSize({168, 212});
        auto windowBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        windowBackgroundColor.w = 0.4f;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
        if (ImGui::Begin("#InGameStatistics", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoDecoration)) {

            ImGui::PushFont(g_uiFontJetbrainsMonoRegular);
            ImGui::TextColored(ImVec4{0.9f, 0.7f, 0.0f, 1.0f}, "F1 to toggle editor");
            ImGui::SeparatorText("Frame Statistics");

            ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * renderContext.FramesPerSecond);
            ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * renderContext.FramesPerSecond));
            ImGui::Text("mfps: %.0f Hz", renderContext.AverageFramesPerSecond);
            ImGui::Text("rfps: %.0f Hz", renderContext.FramesPerSecond);
            ImGui::Text("rpms: %.0f Hz", renderContext.FramesPerSecond * 60.0f);
            ImGui::Text("  ft: %.2f ms", renderContext.DeltaTimeInSeconds * 1000.0f);
            ImGui::Text("   f: %lu", renderContext.FrameCounter);
            ImGui::PopFont();
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    if (g_isEditor) {
        if (ImGui::BeginMainMenuBar()) {
            //ImGui::Image(reinterpret_cast<ImTextureID>(g_iconPackageGreen), ImVec2(16, 16), g_imvec2UnitY, g_imvec2UnitX);
            ImGui::SetCursorPosX(20.0f);
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Quit")) {

                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::RadioButton("Final Output", &g_sceneViewerTextureIndex, 0);
                ImGui::RadioButton("Depth", &g_sceneViewerTextureIndex, 1);
                ImGui::RadioButton("GBuffer-Albedo", &g_sceneViewerTextureIndex, 2);
                ImGui::RadioButton("GBuffer-Normals", &g_sceneViewerTextureIndex, 3);
                ImGui::RadioButton("GBuffer-Velocity", &g_sceneViewerTextureIndex, 4);
                if (g_isFxaaEnabled) {
                    ImGui::RadioButton("FXAA", &g_sceneViewerTextureIndex, 5);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        /*
         * UI - Scene Viewer
         */
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin((char*)ICON_MDI_GRID " Scene")) {
            const auto currentSceneWindowSize = ImGui::GetContentRegionAvail();
            if ((currentSceneWindowSize.x != g_sceneViewerSize.x || currentSceneWindowSize.y != g_sceneViewerSize.y)) {
                g_sceneViewerSize = glm::ivec2(currentSceneWindowSize.x, currentSceneWindowSize.y);
                g_sceneViewerResized = true;
            }

            g_uiViewportContentOffset = []() -> glm::vec2
            {
                auto vMin = ImGui::GetWindowContentRegionMin();
                return {
                    vMin.x + ImGui::GetWindowPos().x,
                    vMin.y + ImGui::GetWindowPos().y,
                  };
            }();
            UiMagnifier(renderContext, g_sceneViewerScaledSize, g_uiViewportContentOffset, ImGui::IsItemHovered());

            if (ImGui::BeginChild("Render Output", currentSceneWindowSize, ImGuiChildFlags_Border)) {
                const auto currentWindowPosition = ImGui::GetWindowPos();

                auto GetCurrentSceneViewerTexture = [&](const int32_t sceneViewerTextureIndex) -> uint32_t {
                    switch (sceneViewerTextureIndex) {
                        case 0: return g_isTaaEnabled
                            ? (g_taaHistoryIndex == 0 ? g_taaFramebuffer1 : g_taaFramebuffer2).ColorAttachments[0]->Texture.Id
                            : g_resolveGeometryFramebuffer.ColorAttachments[0]->Texture.Id;
                        case 1: return g_depthPrePassFramebuffer.DepthStencilAttachment->Texture.Id;
                        case 2: return g_geometryFramebuffer.ColorAttachments[0]->Texture.Id;
                        case 3: return g_geometryFramebuffer.ColorAttachments[1]->Texture.Id;
                        case 4: return g_geometryFramebuffer.ColorAttachments[2]->Texture.Id;
                        case 5: return g_fxaaFramebuffer.ColorAttachments[0]->Texture.Id;
                        case 6: return (g_taaHistoryIndex == 0 ? g_taaFramebuffer1 : g_taaFramebuffer2).ColorAttachments[0]->Texture.Id;
                        default: std::unreachable();
                    }

                    std::unreachable();
                };

                auto texture = GetCurrentSceneViewerTexture(g_sceneViewerTextureIndex);
                ImGui::Image(texture, currentSceneWindowSize, g_uiUnitY, g_uiUnitX);

                if (g_selectedEntity != entt::null) {

                    ImGuizmo::SetDrawlist();
                    ImGuizmo::SetRect(currentWindowPosition.x, currentWindowPosition.y, currentSceneWindowSize.x, currentSceneWindowSize.y);

                    const auto& transformComponent = registry.get<TComponentTransform>(g_selectedEntity);
                    glm::mat4 temp = transformComponent;

                    static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
                    static ImGuizmo::MODE currentGizmoMode(ImGuizmo::MODE::WORLD);
                    static bool useSnap(false);

                    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_1)) {
                        currentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
                    }
                    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_2)) {
                        currentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
                    }
                    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_3)) {
                        currentGizmoOperation = ImGuizmo::OPERATION::SCALE;
                    }
                    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_4)) {
                        currentGizmoMode = ImGuizmo::MODE::LOCAL;
                    }
                    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_5)) {
                        currentGizmoMode = ImGuizmo::MODE::WORLD;
                    }
                    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_0)) {
                        useSnap = !useSnap;
                    }
                /*
                    float matrixTranslation[3];
                    float matrixRotation[3];
                    float matrixScale[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), matrixTranslation, matrixRotation, matrixScale);
                    ImGui::InputFloat3("Tr", matrixTranslation);
                    ImGui::InputFloat3("Rt", matrixRotation);
                    ImGui::InputFloat3("Sc", matrixScale);
                    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(matrix));
                */

                    if (ImGuizmo::Manipulate(
                        glm::value_ptr(g_globalUniforms.ViewMatrix),
                        glm::value_ptr(g_globalUniforms.ProjectionMatrix),
                        currentGizmoOperation,
                        currentGizmoMode,
                        glm::value_ptr(temp))) {

                        registry.replace<TComponentTransform>(g_selectedEntity, temp);
                    }
                }
            }

            ImGui::EndChild();

            //auto collapsingHeaderHeight = ImGui::GetItemRectSize().y;
            //ImGuizmo::SetRect(0, collapsingHeaderHeight, g_sceneViewerScaledSize.x, g_sceneViewerScaledSize.y); //TODO(deccer) get cursor position from where i draw the scene view
        }
        ImGui::PopStyleVar();
        ImGui::End();

        if (ImGui::Begin((char*)ICON_MDI_SETTINGS " Settings")) {
            if (ImGui::SliderFloat("Resolution Scale", &g_windowSettings.ResolutionScale, 0.05f, 4.0f, "%.2f")) {

                g_sceneViewerResized = g_isEditor;
                g_windowFramebufferResized = !g_isEditor;
            }
            ImGui::Checkbox("Enable FXAA", &g_isFxaaEnabled);
            ImGui::Checkbox("Enable TAA", &g_isTaaEnabled);
            if (g_isTaaEnabled) {
                ImGui::DragFloat("TAA Blend Factor", &g_taaBlendFactor, 0.01f, 0.01f, 1.0f, "%.2f");
            }

        }
        ImGui::End();

        /*
         * UI - Assets Viewer
         */
        if (ImGui::Begin((char*)ICON_MDI_LIBRARY_SHELVES " Assets")) {
            auto& assets = Assets::GetAssetModels();
            ImGui::BeginTable("##Assets", 2, ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("Asset");
            ImGui::TableSetupColumn("X");
            ImGui::TableNextRow();
            for (auto& [assetName, asset] : assets) {
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(assetName.data());
                ImGui::TableSetColumnIndex(1);
                ImGui::PushID(assetName.data());
                if (ImGui::Button((char*)ICON_MDI_PLUS_BOX " Create")) {

                }
                ImGui::PopID();
                ImGui::TableNextRow();
            }
            ImGui::EndTable();
        }
        ImGui::End();

        /*
         * UI - Scene Hierarchy
         */
        if (ImGui::Begin((char*)ICON_MDI_TREE " Hierarchy")) {

            RenderEntityHierarchy(registry, static_cast<entt::entity>(0));
        }
        ImGui::End();

        /*
         * UI - Properties
         */
        if (ImGui::Begin((char*)ICON_MDI_CERTIFICATE " Properties")) {

            if (g_selectedEntity != entt::null) {
                ImGui::PushID(static_cast<int32_t>(g_selectedEntity));
                RenderEntityProperties(registry, g_selectedEntity);
                ImGui::PopID();
            }
        }
        ImGui::End();

        /*
         * UI - Atmosphere Settings
         */
        if (ImGui::Begin((char*)ICON_MDI_CLOUD " Atmosphere")) {

            auto atmosphereModified = false;
            if (ImGui::SliderFloat3("Sun Position", glm::value_ptr(g_sunPosition), -10000.0f, 10000.0f)) {
                atmosphereModified = true;
            }
        }
        ImGui::End();
    }
    {
        PROFILER_ZONESCOPEDN("Draw ImGUI");
        ImGui::Render();
        if (auto* imGuiDrawData = ImGui::GetDrawData(); imGuiDrawData != nullptr) {
            PushDebugGroup("UI-ImGui");
            glViewport(0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y);
            ImGui_ImplOpenGL3_RenderDrawData(imGuiDrawData);
            PopDebugGroup();
        }
    }
}

auto UiMagnifier(
    Renderer::TRenderContext& renderContext,
    const glm::vec2 viewportContentSize,
    const glm::vec2 viewportContentOffset,
    const bool isViewportHovered) -> void {

    if (!g_uiIsMagnifierEnabled) {
        return;
    }

    bool magnifierLock = true;

    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)/* && isViewportHovered*/) {
        magnifierLock = false;
    }

    if (ImGui::Begin((char*)ICON_MDI_MAGNIFY" Magnifier",
        &g_uiIsMagnifierEnabled,
        ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::TextUnformatted(std::string(magnifierLock ? "Locked" : "Unlocked").c_str());
        ImGui::SliderFloat("Zoom (+, -)", &g_uiMagnifierZoom, 1.0f, 50.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
        if (ImGui::GetKeyPressedAmount(ImGuiKey_KeypadSubtract, 10000, 1)) {
            g_uiMagnifierZoom = std::max(g_uiMagnifierZoom / 1.5f, 1.0f);
        }
        if (ImGui::GetKeyPressedAmount(ImGuiKey_KeypadAdd, 10000, 1)) {
            g_uiMagnifierZoom = std::min(g_uiMagnifierZoom * 1.5f, 50.0f);
        }

        const auto magnifierSize = ImGui::GetContentRegionAvail();

        // The dimensions of the region viewed by the magnifier, equal to the magnifier's size (1x zoom) or less
        const auto magnifierExtent = ImVec2(
            std::min(viewportContentSize.x, magnifierSize.x / g_uiMagnifierZoom),
            std::min(viewportContentSize.y, magnifierSize.y / g_uiMagnifierZoom));

        // Get window coords
        double x{}, y{};
        glfwGetCursorPos(renderContext.Window, &x, &y);

        // Window to viewport
        x += viewportContentOffset.x;
        y += viewportContentOffset.y;

        // Clamp to smaller region within the viewport so magnifier doesn't view OOB pixels
        x = std::clamp(static_cast<float>(x), magnifierExtent.x / 2.f, viewportContentSize.x - magnifierExtent.x / 2.f);
        y = std::clamp(static_cast<float>(y), magnifierExtent.y / 2.f, viewportContentSize.y - magnifierExtent.y / 2.f);

        // Use stored cursor pos if magnifier is locked
        glm::vec2 mp = magnifierLock ? g_uiMagnifierLastCursorPos : glm::vec2{x, y};
        g_uiMagnifierLastCursorPos = mp;

        // Y flip (+Y is up for textures)
        mp.y = viewportContentSize.y - mp.y;

        // Mouse position in UV space
        mp /= glm::vec2(viewportContentSize.x, viewportContentSize.y);
        const glm::vec2 magnifierHalfExtentUv = {
            magnifierExtent.x / 2.f / viewportContentSize.x,
            -magnifierExtent.y / 2.f / viewportContentSize.y,
        };

        // Calculate the min and max UV of the magnifier window
        const auto uv0{mp - magnifierHalfExtentUv};
        const auto uv1{mp + magnifierHalfExtentUv};

        const auto& magnifierOutput = g_isTaaEnabled
          ? g_taaHistoryIndex == 0
            ? g_taaFramebuffer1.ColorAttachments[0]->Texture.Id
            : g_taaFramebuffer2.ColorAttachments[0]->Texture.Id
          : g_resolveGeometryFramebuffer.ColorAttachments[0]->Texture.Id;

        ImGui::Image(magnifierOutput,
                     magnifierSize,
                     ImVec2(uv0.x, uv0.y),
                     ImVec2(uv1.x, uv1.y));
        }
    ImGui::End();
}

auto UiSetDarkStyle() -> void {

    auto& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.DisabledAlpha = 0.3f;
    style.PopupRounding = 0;
    style.WindowPadding = ImVec2(4, 4);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 2);
    style.ScrollbarSize = 18;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.WindowRounding = 0;
    style.ChildRounding = 0;
    style.FrameRounding = 0;
    style.ScrollbarRounding = 0;
    style.GrabRounding = 0;
    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;

    auto* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.90f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.42f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.32f, 0.32f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.12f, 0.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.67f, 0.60f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.46f, 0.24f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.69f, 0.30f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.00f, 0.96f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

auto UiSetValveStyle() -> void {

    auto& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.WindowPadding = ImVec2(20, 6);
    style.WindowTitleAlign = ImVec2(0.30f, 0.50f);
    style.ScrollbarSize = 17;
    style.FramePadding = ImVec2(5, 6);

    style.FrameRounding = 0;
    style.WindowRounding = 0;
    style.ScrollbarRounding = 0;
    style.ChildRounding = 0;
    style.PopupRounding = 0;
    style.GrabRounding = 0;
    style.TabRounding = 0;

    style.WindowBorderSize = 1;
    style.FrameBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.TabBorderSize = 1;

    style.Colors[ImGuiCol_Text]                   = ImVec4(0.85f, 0.87f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]           = ImVec4(0.63f, 0.67f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]               = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]                = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Border]                 = ImVec4(0.53f, 0.57f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]           = ImVec4(0.16f, 0.18f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]                = ImVec4(0.24f, 0.27f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.24f, 0.27f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.27f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]                = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.35f, 0.42f, 0.31f, 0.52f);
    style.Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]             = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]           = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Header]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]           = ImVec4(1.00f, 0.49f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_Separator]              = ImVec4(0.16f, 0.18f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.16f, 0.18f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.16f, 0.18f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Tab]                    = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]             = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TabActive]              = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]           = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.30f, 0.35f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_DockingPreview]         = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_DockingEmptyBg]         = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]              = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.26f, 0.59f, 0.98f, 0.34f);
    style.Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 1.00f, 0.00f, 0.78f);
    style.Colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong]      = ImVec4(1.00f, 1.00f, 1.00f, 0.67f);
    style.Colors[ImGuiCol_TableBorderLight]       = ImVec4(0.80f, 0.80f, 0.80f, 0.31f);
    style.Colors[ImGuiCol_TableRowBg]             = ImVec4(0.80f, 0.80f, 0.80f, 0.38f);
    style.Colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.58f, 0.53f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_DragDropTarget]         = ImVec4(0.58f, 0.53f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.94f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.12f, 0.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
}

auto Lerp(
    const ImVec4 color1,
    const ImVec4 color2,
    const float interpolator) -> ImVec4 {

    return ImVec4(
        std::lerp(color1.x, color2.x, interpolator),
        std::lerp(color1.y, color2.y, interpolator),
        std::lerp(color1.z, color2.z, interpolator),
        std::lerp(color1.w, color2.w, interpolator));
}

auto UiSetDarkWithFuchsiaStyle() -> void {

    auto& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 4.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.FrameRounding = 2.5f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 11.0f;
    style.ScrollbarRounding = 2.5f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 3.5f;
    style.TabBorderSize = 0.0f;
    style.TabCloseButtonMinWidthSelected = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.060085229575634f, 0.060085229575634f, 0.06008583307266235f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1802574992179871f, 0.1802556961774826f, 0.1802556961774826f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.270386278629303f, 0.2703835666179657f, 0.2703848779201508f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.6266094446182251f, 0.6266031861305237f, 0.6266063451766968f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.9999899864196777f, 0.9999899864196777f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.9999899864196777f, 0.9999899864196777f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.184547483921051f, 0.184547483921051f, 0.1845493316650391f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1803921610116959f, 0.1803921610116959f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1803921610116959f, 0.1803921610116959f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1803921610116959f, 0.1803921610116959f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2489270567893982f, 0.2489245682954788f, 0.2489245682954788f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 0.9999899864196777f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 0.9999899864196777f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.8755365014076233f, 0.00751531170681119f, 0.3875076174736023f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 0.7f);

    auto& dockingPreview = style.Colors[ImGuiCol_HeaderActive];
    dockingPreview.z *= 0.7f;
    style.Colors[ImGuiCol_DockingPreview] = dockingPreview;
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = style.Colors[ImGuiCol_HeaderHovered];
    style.Colors[ImGuiCol_Tab] = Lerp(style.Colors[ImGuiCol_Header], style.Colors[ImGuiCol_TitleBgActive], 0.80f);
    style.Colors[ImGuiCol_TabSelected] = Lerp(style.Colors[ImGuiCol_HeaderActive], style.Colors[ImGuiCol_TitleBgActive], 0.60f);
    style.Colors[ImGuiCol_TabSelectedOverline] = style.Colors[ImGuiCol_HeaderActive];
    style.Colors[ImGuiCol_TabDimmed] = Lerp(style.Colors[ImGuiCol_Tab], style.Colors[ImGuiCol_TitleBg], 0.60f);
    style.Colors[ImGuiCol_TabDimmedSelected] = Lerp(style.Colors[ImGuiCol_TabSelected], style.Colors[ImGuiCol_TitleBg], 0.40f);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = Lerp(style.Colors[ImGuiCol_TabSelected], style.Colors[ImGuiCol_TitleBg], 0.20f);
}
