#include "Renderer.hpp"
#include "RHI.hpp"
#include "Components.hpp"
#include "Profiler.hpp"
#include "Assets.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <debugbreak.h>
#include <spdlog/spdlog.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "IconsMaterialDesign.h"
#include "IconsFontAwesome6.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <ranges>
#include <utility>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

struct TGpuDebugLine {
    glm::vec3 StartPosition;
    glm::vec4 StartColor;
    glm::vec3 EndPosition;
    glm::vec4 EndColor;
};

struct TGpuGlobalUniforms {

    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
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

struct TAtmosphereSettings {
    glm::vec4 SunPositionAndPlanetRadius;
    glm::vec4 RayOriginAndSunIntensity;
    glm::vec4 RayleighScatteringCoefficientAndAtmosphereRadius;
    float MieScatteringCoefficient;
    float MieScaleHeight;
    float MiePreferredScatteringDirection;
    float RayleighScaleHeight;
};

TAtmosphereSettings g_atmosphereSettings = {};

TWindowSettings g_windowSettings = {};

glm::vec3 g_atmosphereSunPosition = {10000.0f, 10000.0f, 10000.0f};
float g_atmospherePlanetRadius = 6232.558f;
glm::vec3 g_atmosphereRayOrigin = {0.0f, 6227.072f, 0.0f};
float g_atmosphereSunIntensity = 15.0f;
glm::vec3 g_atmosphereRayleighScatteringCoefficient = {0.00037f, 0.00255f, 0.00765};
float g_atmosphereAtmosphereRadius = g_atmospherePlanetRadius + 70.0f;
float g_atmosphereMieScatteringCoefficient = 0.0001;
float g_atmosphereMieScaleHeight = 13.0f;
float g_atmosphereMiePreferredScatteringDirection = 0.9f;
float g_atmosphereRayleighScaleHeight = 6.6f;

constexpr ImVec2 g_imvec2UnitX = ImVec2(1, 0);
constexpr ImVec2 g_imvec2UnitY = ImVec2(0, 1);
ImGuiContext* g_imguiContext = nullptr;

TFramebuffer g_depthPrePassFramebuffer = {};
TGraphicsPipeline g_depthPrePassGraphicsPipeline = {};

TFramebuffer g_geometryFramebuffer = {};
TGraphicsPipeline g_geometryGraphicsPipeline = {};

TFramebuffer g_resolveGeometryFramebuffer = {};
TGraphicsPipeline g_resolveGeometryGraphicsPipeline = {};

std::vector<TGpuGlobalLight> g_gpuGlobalLights;
uint32_t g_globalLightsBuffer = {};

TGpuGlobalUniforms g_globalUniforms = {};

uint32_t g_globalUniformsBuffer = {};
uint32_t g_objectsBuffer = {};

bool g_drawDebugLines = true;
std::vector<TGpuDebugLine> g_debugLines;
uint32_t g_debugInputLayout = 0;
uint32_t g_debugLinesVertexBuffer = 0;
TGraphicsPipeline g_debugLinesGraphicsPipeline = {};

TFramebuffer g_fxaaFramebuffer = {};
TGraphicsPipeline g_fxaaGraphicsPipeline = {};

TGraphicsPipeline g_fstGraphicsPipeline = {};
TSamplerId g_fstSamplerNearestClampToEdge = {};

uint32_t g_atmosphereSettingsBuffer = 0;
bool g_isFxaaEnabled = false;

glm::vec2 g_scaledFramebufferSize = {};

glm::ivec2 g_windowFramebufferSize = {};
glm::ivec2 g_windowFramebufferScaledSize = {};
bool g_windowFramebufferResized = false;

glm::ivec2 g_sceneViewerSize = {};
glm::ivec2 g_sceneViewerScaledSize = {};
bool g_sceneViewerResized = false;
bool g_isEditor = false;
bool g_isSrgbDisabled = false;

auto OnOpenGLDebugMessage(
    [[maybe_unused]] uint32_t source,
    uint32_t type,
    [[maybe_unused]] uint32_t id,
    [[maybe_unused]] uint32_t severity,
    [[maybe_unused]] int32_t length,
    const char* message,
    [[maybe_unused]] const void* userParam) -> void {

    if (type == GL_DEBUG_TYPE_ERROR) {
        spdlog::error(message);
        debug_break();
    }
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
    glm::vec4 Factors; // use .x = basecolor factor .y = normal strength, .z = metalness, .w = roughness

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
    glm::vec4 Factors; // use .x = basecolor factor .y = normal strength, .z = metalness, .w = roughness

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

auto SignNotZero(glm::vec2 v) -> glm::vec2 {

    return glm::vec2((v.x >= 0.0f) ? +1.0f : -1.0f, (v.y >= 0.0f) ? +1.0f : -1.0f);
}

auto EncodeNormal(glm::vec3 normal) -> glm::vec2 {

    glm::vec2 encodedNormal = glm::vec2{normal.x, normal.y} * (1.0f / (abs(normal.x) + abs(normal.y) + abs(normal.z)));
    return (normal.z <= 0.0f)
           ? ((1.0f - glm::abs(glm::vec2{encodedNormal.x, encodedNormal.y})) * SignNotZero(encodedNormal)) //TODO(deccer) check if its encNor.y, encNor.x or not
           : encodedNormal;
}

auto RendererCreateGpuMesh(const TAssetMeshData& assetMeshData) -> void {

    std::vector<TGpuVertexPosition> vertexPositions;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> vertexNormalUvTangents;
    vertexPositions.resize(assetMeshData.Positions.size());
    vertexNormalUvTangents.resize(assetMeshData.Positions.size());

    for (size_t i = 0; i < assetMeshData.Positions.size(); i++) {
        vertexPositions[i] = {
            assetMeshData.Positions[i],
        };

        vertexNormalUvTangents[i] = {
            glm::packSnorm2x16(EncodeNormal(assetMeshData.Normals[i])),
            glm::packSnorm2x16(EncodeNormal(assetMeshData.Tangents[i].xyz())),
            glm::vec4(assetMeshData.Uvs[i], assetMeshData.Tangents[i].w, 0.0f),
        };
    }

    uint32_t buffers[3] = {};
    {
        PROFILER_ZONESCOPEDN("Create GL Buffers + Upload Data");

        glCreateBuffers(3, buffers);
        SetDebugLabel(buffers[0], GL_BUFFER, std::format("{}_position", assetMeshData.Name));
        SetDebugLabel(buffers[1], GL_BUFFER, std::format("{}_normal_uv_tangent", assetMeshData.Name));
        SetDebugLabel(buffers[2], GL_BUFFER, std::format("{}_indices", assetMeshData.Name));
        glNamedBufferStorage(buffers[0], sizeof(TGpuVertexPosition) * vertexPositions.size(),
                             vertexPositions.data(), 0);
        glNamedBufferStorage(buffers[1], sizeof(TGpuPackedVertexNormalTangentUvTangentSign) * vertexNormalUvTangents.size(),
                             vertexNormalUvTangents.data(), 0);
        glNamedBufferStorage(buffers[2], sizeof(uint32_t) * assetMeshData.Indices.size(), assetMeshData.Indices.data(), 0);
    }

    auto gpuMesh = TGpuMesh{
        .Name = assetMeshData.Name,
        .VertexPositionBuffer = buffers[0],
        .VertexNormalUvTangentBuffer = buffers[1],
        .IndexBuffer = buffers[2],

        .VertexCount = vertexPositions.size(),
        .IndexCount = assetMeshData.Indices.size(),

        .InitialTransform = assetMeshData.InitialTransform,
    };

    {
        PROFILER_ZONESCOPEDN("Add Gpu Mesh");
        g_gpuMeshes[assetMeshData.Name] = gpuMesh;
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

auto CreateResidentTextureForMaterialChannel(const std::string& materialDataName) -> int64_t {

    PROFILER_ZONESCOPEDN("CreateResidentTextureForMaterialChannel");

    auto& imageData = GetAssetImageData(materialDataName);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height))))),
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

auto CreateTextureForMaterialChannel(const std::string& imageDataName) -> uint32_t {

    PROFILER_ZONESCOPEDN("CreateTextureForMaterialChannel");

    auto& imageData = GetAssetImageData(imageDataName);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height))))),
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

constexpr auto ToAddressMode(TAssetSamplerWrapMode wrapMode) -> TTextureAddressMode {
    switch (wrapMode) {
        case TAssetSamplerWrapMode::ClampToEdge: return TTextureAddressMode::ClampToEdge;
        case TAssetSamplerWrapMode::MirroredRepeat: return TTextureAddressMode::RepeatMirrored;
        case TAssetSamplerWrapMode::Repeat: return TTextureAddressMode::Repeat;
        default: std::unreachable();
    }
}

constexpr auto ToMagFilter(std::optional<TAssetSamplerMagFilter> magFilter) -> TTextureMagFilter {
    if (!magFilter.has_value()) {
        return TTextureMagFilter::Linear;
    }

    switch (*magFilter) {
        case TAssetSamplerMagFilter::Linear: return TTextureMagFilter::Linear;
        case TAssetSamplerMagFilter::Nearest: return TTextureMagFilter::Nearest;
        default: std::unreachable();
    }
}

constexpr auto ToMinFilter(std::optional<TAssetSamplerMinFilter> minFilter) -> TTextureMinFilter {
    if (!minFilter.has_value()) {
        return TTextureMinFilter::Linear;
    }
    
    switch (*minFilter) {
        case TAssetSamplerMinFilter::Linear: return TTextureMinFilter::Linear;
        case TAssetSamplerMinFilter::Nearest: return TTextureMinFilter::Nearest;
        case TAssetSamplerMinFilter::NearestMipMapLinear: return TTextureMinFilter::NearestMipmapLinear;
        case TAssetSamplerMinFilter::NearestMipMapNearest: return TTextureMinFilter::NearestMipmapNearest;
        case TAssetSamplerMinFilter::LinearMipMapNearest: return TTextureMinFilter::LinearMipmapNearest;
        case TAssetSamplerMinFilter::LinearMipMapLinear: return TTextureMinFilter::LinearMipmapLinear;        
        default: std::unreachable();
    }
}

auto CreateSamplerDescriptor(const TAssetSamplerData& assetSampler) -> TSamplerDescriptor {

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

    auto& assetMaterialData = GetAssetMaterialData(assetMaterialName);

    auto cpuMaterial = TCpuMaterial{
        .BaseColor = assetMaterialData.BaseColor,
    };

    auto& baseColorChannel = assetMaterialData.BaseColorTextureChannel;
    if (baseColorChannel.has_value()) {
        auto& baseColor = *baseColorChannel;
        auto& baseColorSampler = GetAssetSamplerData(baseColor.SamplerName);

        cpuMaterial.BaseColorTextureId = CreateTextureForMaterialChannel(baseColor.TextureName);
        auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(baseColorSampler));
        auto& sampler = GetSampler(samplerId);
        cpuMaterial.BaseColorTextureSamplerId = sampler.Id;
    }

    auto& normalTextureChannel = assetMaterialData.NormalTextureChannel;
    if (normalTextureChannel.has_value()) {
        auto& normalTexture = *normalTextureChannel;
        auto& normalTextureSampler = GetAssetSamplerData(normalTexture.SamplerName);

        cpuMaterial.NormalTextureId = CreateTextureForMaterialChannel(normalTexture.TextureName);
        auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(normalTextureSampler));
        auto& sampler = GetSampler(samplerId);
        cpuMaterial.NormalTextureSamplerId = sampler.Id;
    }

    g_cpuMaterials[assetMaterialName] = cpuMaterial;
}

auto DeleteRendererFramebuffers() -> void {

    DeleteFramebuffer(g_depthPrePassFramebuffer);
    DeleteFramebuffer(g_geometryFramebuffer);
    DeleteFramebuffer(g_resolveGeometryFramebuffer);
    DeleteFramebuffer(g_fxaaFramebuffer);
}

auto CreateRendererFramebuffers(const glm::vec2& scaledFramebufferSize) -> void {

    PROFILER_ZONESCOPEDN("CreateRendererFramebuffers");

    g_depthPrePassFramebuffer = CreateFramebuffer({
                                                      .Label = "Depth PrePass",
                                                      .DepthStencilAttachment = TFramebufferDepthStencilAttachmentDescriptor{
                                                          .Label = "Depth",
                                                          .Format = TFormat::D24_UNORM_S8_UINT,
                                                          .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                                                          .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                                                          .ClearDepthStencil = { 1.0f, 0 },
                                                      }
                                                  });

    g_geometryFramebuffer = CreateFramebuffer({
                                                  .Label = "Geometry",
                                                  .ColorAttachments = {
                                                      TFramebufferColorAttachmentDescriptor{
                                                          .Label = "GeometryAlbedo",
                                                          .Format = TFormat::R8G8B8A8_SRGB,
                                                          .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                                                          .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                                                          .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
                                                      },
                                                      TFramebufferColorAttachmentDescriptor{
                                                          .Label = "GeometryNormals",
                                                          .Format = TFormat::R32G32B32A32_FLOAT,
                                                          .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                                                          .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                                                          .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
                                                      },
                                                  },
                                                  .DepthStencilAttachment = TFramebufferExistingDepthStencilAttachmentDescriptor{
                                                      .ExistingDepthTexture = g_depthPrePassFramebuffer.DepthStencilAttachment.value().Texture,
                                                  }
                                              });

    g_resolveGeometryFramebuffer = CreateFramebuffer({
                                                         .Label = "ResolveGeometry",
                                                         .ColorAttachments = {
                                                             TFramebufferColorAttachmentDescriptor{
                                                                 .Label = "ResolvedGeometry",
                                                                 .Format = TFormat::R8G8B8A8_SRGB,
                                                                 .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                                                                 .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                                                                 .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
                                                             },
                                                         },
                                                     });

    g_fxaaFramebuffer = CreateFramebuffer({
                                              .Label = "PostFX-FXAA",
                                              .ColorAttachments = {
                                                  TFramebufferColorAttachmentDescriptor{
                                                      .Label = "PostFX-FXAA-Buffer",
                                                      .Format = TFormat::R8G8B8A8_SRGB,
                                                      .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                                                      .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
                                                      .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f }
                                                  }
                                              }
                                          });
}

auto RendererInitialize(
    const TWindowSettings& windowSettings,
    GLFWwindow* window,
    const glm::vec2& initialFramebufferSize) -> bool {

    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        return false;
    }

    RhiInitialize();

    g_windowSettings = windowSettings;

    if (g_windowSettings.IsDebug) {
        glDebugMessageCallback(OnOpenGLDebugMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

#ifdef USE_PROFILER
    TracyGpuContext
#endif

    g_imguiContext = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // this little shit doesn't do anything
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    constexpr auto fontSize = 18.0f;

    io.Fonts->AddFontFromFileTTF("data/fonts/HurmitNerdFont-Regular.otf", fontSize);
    {
        constexpr float iconFontSize = fontSize * 4.0f / 5.0f;
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        icons_config.GlyphOffset.y = 0; // Hack to realign the icons
        io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_FAR, iconFontSize, &icons_config, icons_ranges);
    }
    {
        constexpr float iconFontSize = fontSize;
        static const ImWchar icons_ranges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        icons_config.GlyphOffset.y = 4; // Hack to realign the icons
        io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_MD, iconFontSize, &icons_config, icons_ranges);
    }

    auto& imguiStyle = ImGui::GetStyle();
    imguiStyle.WindowMenuButtonPosition = ImGuiDir_None;
    imguiStyle.DisabledAlpha = 0.3f;
    imguiStyle.PopupRounding = 0;
    imguiStyle.WindowPadding = ImVec2(4, 4);
    imguiStyle.FramePadding = ImVec2(6, 4);
    imguiStyle.ItemSpacing = ImVec2(6, 2);
    imguiStyle.ScrollbarSize = 18;
    imguiStyle.WindowBorderSize = 1;
    imguiStyle.ChildBorderSize = 1;
    imguiStyle.PopupBorderSize = 1;
    imguiStyle.FrameBorderSize = 1;
    imguiStyle.WindowRounding = 0;
    imguiStyle.ChildRounding = 0;
    imguiStyle.FrameRounding = 0;
    imguiStyle.ScrollbarRounding = 0;
    imguiStyle.GrabRounding = 0;
    imguiStyle.AntiAliasedFill = true;
    imguiStyle.AntiAliasedLines = true;

    auto* colors = imguiStyle.Colors;
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

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        spdlog::error("ImGui: Unable to initialize the GLFW backend");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        spdlog::error("ImGui: Unable to initialize the OpenGL backend");
        return false;
    }

    glfwSwapInterval(g_windowSettings.IsVSyncEnabled ? 1 : 0);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.05f, 0.07f, 1.0f);

    AddDefaultAssets();

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
        return -1;
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
        return -1;
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
        return 0;
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
        return 0;
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
        return 0;
    }

    g_fxaaGraphicsPipeline = *fxaaGraphicsPipelineResult;

    g_debugLinesVertexBuffer = CreateBuffer("VertexBuffer-DebugLines", sizeof(TGpuDebugLine) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    auto atmosphereSettings = TAtmosphereSettings{
        .SunPositionAndPlanetRadius = glm::vec4(g_atmosphereSunPosition, g_atmospherePlanetRadius),
        .RayOriginAndSunIntensity = glm::vec4(g_atmosphereRayOrigin, g_atmosphereSunIntensity),
        .RayleighScatteringCoefficientAndAtmosphereRadius = glm::vec4(g_atmosphereRayleighScatteringCoefficient, g_atmosphereAtmosphereRadius),
        .MieScatteringCoefficient = g_atmosphereMieScatteringCoefficient,
        .MieScaleHeight = g_atmosphereMieScaleHeight,
        .MiePreferredScatteringDirection = g_atmosphereMiePreferredScatteringDirection,
        .RayleighScaleHeight = g_atmosphereRayleighScaleHeight
    };
    g_atmosphereSettingsBuffer = CreateBuffer("AtmosphereSettings", sizeof(TAtmosphereSettings), &atmosphereSettings, GL_DYNAMIC_STORAGE_BIT);

    auto cameraPosition = glm::vec3{0.0f, 0.0f, 20.0f};
    auto cameraDirection = glm::vec3{0.0f, 0.0f, -1.0f};
    auto cameraUp = glm::vec3{0.0f, 1.0f, 0.0f};
    auto fieldOfView = glm::radians(70.0f);
    auto aspectRatio = g_scaledFramebufferSize.x / static_cast<float>(g_scaledFramebufferSize.y);
    g_globalUniforms = {
        .ProjectionMatrix = glm::infinitePerspective(fieldOfView, aspectRatio, 0.1f),
        .ViewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp),
        .CameraPosition = glm::vec4{cameraPosition, fieldOfView},
        .CameraDirection = glm::vec4{cameraDirection, aspectRatio}
    };
    g_globalUniformsBuffer = CreateBuffer("TGpuGlobalUniforms", sizeof(TGpuGlobalUniforms), &g_globalUniforms, GL_DYNAMIC_STORAGE_BIT);
    g_objectsBuffer = CreateBuffer("TGpuObjects", sizeof(TGpuObject) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    g_gpuGlobalLights.push_back(TGpuGlobalLight{
        .ProjectionMatrix = glm::mat4(1.0f),
        .ViewMatrix = glm::mat4(1.0f),
        .Direction = glm::vec4(10.0f, 20.0f, 10.0f, 0.0f),
        .Strength = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
    });

    g_globalLightsBuffer = CreateBuffer("SGpuGlobalLights", g_gpuGlobalLights.size() * sizeof(TGpuGlobalLight), g_gpuGlobalLights.data(), GL_DYNAMIC_STORAGE_BIT);

    g_fstSamplerNearestClampToEdge = GetOrCreateSampler({
                                                            .AddressModeU = TTextureAddressMode::ClampToEdge,
                                                            .AddressModeV = TTextureAddressMode::ClampToEdge,
                                                            .MagFilter = TTextureMagFilter::Nearest,
                                                            .MinFilter = TTextureMinFilter::NearestMipmapNearest
                                                        });

    return true;
}

auto RendererUnload() -> void {

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

    RhiShutdown();
}

auto RendererRender(
    TRenderContext& renderContext,
    entt::registry& registry,
    TCamera& camera) -> void {

    ///////////////////////
    // Create Gpu Resources if necessary
    ///////////////////////
    auto createGpuResourcesNecessaryView = registry.view<TComponentCreateGpuResourcesNecessary>();
    for (auto& entity : createGpuResourcesNecessaryView) {

        PROFILER_ZONESCOPEDN("Create Gpu Resources");
        auto& meshComponent = registry.get<TComponentMesh>(entity);
        auto& materialComponent = registry.get<TComponentMaterial>(entity);

        auto& assetMesh = GetAssetMeshData(meshComponent.Mesh);
        RendererCreateGpuMesh(assetMesh);
        RendererCreateCpuMaterial(materialComponent.Material);

        registry.emplace<TComponentGpuMesh>(entity, meshComponent.Mesh);
        registry.emplace<TComponentGpuMaterial>(entity, materialComponent.Material);

        registry.remove<TComponentCreateGpuResourcesNecessary>(entity);
    }

    // Resize if necessary
    if (g_windowFramebufferResized || g_sceneViewerResized) {

        PROFILER_ZONESCOPEDN("Resized");

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

    // Update Per Frame Uniforms
    auto fieldOfView = glm::radians(70.0f); // TODO(deccer) move this into the camera?
    auto aspectRatio = g_scaledFramebufferSize.x / static_cast<float>(g_scaledFramebufferSize.y);
    g_globalUniforms.ProjectionMatrix = glm::infinitePerspective(fieldOfView, aspectRatio, 0.1f);
    g_globalUniforms.ViewMatrix = camera.GetViewMatrix();
    g_globalUniforms.CameraPosition = glm::vec4(camera.Position, fieldOfView);
    g_globalUniforms.CameraDirection = glm::vec4(camera.GetForwardDirection(), aspectRatio);
    UpdateBuffer(g_globalUniformsBuffer, 0, sizeof(TGpuGlobalUniforms), &g_globalUniforms);

    //
    // CLEAR DEBUG LINES
    //
    if (g_drawDebugLines) {
        g_debugLines.clear();

        g_debugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{-150, 30, 4},
            .StartColor = glm::vec4{0.3f, 0.95f, 0.1f, 1.0f},
            .EndPosition = glm::vec3{150, -40, -4},
            .EndColor = glm::vec4{0.6f, 0.1f, 0.0f, 1.0f}
        });
    }

    if (g_isSrgbDisabled) {
        glEnable(GL_FRAMEBUFFER_SRGB);
        g_isSrgbDisabled = false;
    }

    {
        PROFILER_ZONESCOPEDN("All Depth PrePass Geometry");
        PushDebugGroup("Depth PrePass");
        BindFramebuffer(g_depthPrePassFramebuffer);
        {
            g_depthPrePassGraphicsPipeline.Bind();
            g_depthPrePassGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);

            const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentTransform>();
            renderablesView.each([](
                const auto& meshComponent,
                const auto& transformComponent) {

                PROFILER_ZONESCOPEDN("Draw PrePass Geometry");

                auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                g_depthPrePassGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                g_depthPrePassGraphicsPipeline.SetUniform(0, transformComponent * gpuMesh.InitialTransform);

                g_depthPrePassGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
            });
        }
        PopDebugGroup();
    }
    {
        PROFILER_ZONESCOPEDN("Draw Geometry All");
        PushDebugGroup("Geometry");
        BindFramebuffer(g_geometryFramebuffer);
        {
            g_geometryGraphicsPipeline.Bind();
            g_geometryGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);

            const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentGpuMaterial, TComponentTransform>();
            renderablesView.each([](
                const auto& meshComponent,
                const auto& materialComponent,
                const auto& transformComponent) {

                PROFILER_ZONESCOPEDN("Draw Geometry");

                auto& cpuMaterial = GetCpuMaterial(materialComponent.GpuMaterial);
                auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexNormalUvTangentBuffer, 2);
                g_geometryGraphicsPipeline.SetUniform(0, transformComponent * gpuMesh.InitialTransform);

                g_geometryGraphicsPipeline.BindTextureAndSampler(8, cpuMaterial.BaseColorTextureId,
                                                                 cpuMaterial.BaseColorTextureSamplerId);
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
            g_resolveGeometryGraphicsPipeline.Bind();
            //auto& samplerId = g_samplers[size_t(g_fullscreenSamplerNearestClampToEdge.Id)]
            g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);
            g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(g_globalLightsBuffer, 2);
            g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(g_atmosphereSettingsBuffer, 3);
            g_resolveGeometryGraphicsPipeline.BindTexture(0, g_geometryFramebuffer.ColorAttachments[0]->Texture.Id);
            g_resolveGeometryGraphicsPipeline.BindTexture(1, g_geometryFramebuffer.ColorAttachments[1]->Texture.Id);
            g_resolveGeometryGraphicsPipeline.BindTexture(2, g_depthPrePassFramebuffer.DepthStencilAttachment->Texture.Id);

            g_resolveGeometryGraphicsPipeline.SetUniform(0, camera.GetForwardDirection());

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

    /////////////// UI

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (g_isEditor) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glBlitNamedFramebuffer(g_isFxaaEnabled
                               ? g_fxaaFramebuffer.Id
                               : g_resolveGeometryFramebuffer.Id,
                               0,
                               0, 0, static_cast<int32_t>(g_scaledFramebufferSize.x), static_cast<int32_t>(g_scaledFramebufferSize.y),
                               0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y,
                               GL_COLOR_BUFFER_BIT,
                               GL_NEAREST);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (g_isEditor) {
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    }

    if (!g_isEditor) {
        ImGui::SetNextWindowPos({32, 32});
        ImGui::SetNextWindowSize({168, 192});
        auto windowBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        windowBackgroundColor.w = 0.4f;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
        if (ImGui::Begin("#InGameStatistics", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoDecoration)) {

            ImGui::TextColored(ImVec4{0.9f, 0.7f, 0.0f, 1.0f}, "F1 to toggle editor");
            ImGui::SeparatorText("Frame Statistics");

            ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * renderContext.FramesPerSecond);
            ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * renderContext.FramesPerSecond));
            ImGui::Text("mfps: %.0f", renderContext.AverageFramesPerSecond);
            ImGui::Text("rfps: %.0f", renderContext.FramesPerSecond);
            ImGui::Text("rpms: %.0f", renderContext.FramesPerSecond * 60.0f);
            ImGui::Text("  ft: %.2f ms", renderContext.DeltaTimeInSeconds * 1000.0f);
            ImGui::Text("   f: %lu", renderContext.FrameCounter);
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
            ImGui::EndMainMenuBar();
        }

        /*
         * UI - Scene Viewer
         */
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin(ICON_MD_GRID_VIEW "Scene")) {
            auto currentSceneWindowSize = ImGui::GetContentRegionAvail();
            if ((currentSceneWindowSize.x != g_sceneViewerSize.x || currentSceneWindowSize.y != g_sceneViewerSize.y)) {
                g_sceneViewerSize = glm::ivec2(currentSceneWindowSize.x, currentSceneWindowSize.y);
                g_sceneViewerResized = true;
            }

            auto texture = g_geometryFramebuffer.ColorAttachments[1].value().Texture.Id;
            auto imagePosition = ImGui::GetCursorPos();
            ImGui::Image(static_cast<intptr_t>(texture), currentSceneWindowSize, g_imvec2UnitY, g_imvec2UnitX);
            ImGui::SetCursorPos(imagePosition);
            //if (ImGui::BeginChild(1, ImVec2{192, -1})) {
            //    if (ImGui::CollapsingHeader("Statistics")) {
            //    }
            //}
            //ImGui::EndChild();
        }
        ImGui::PopStyleVar();
        ImGui::End();

        if (ImGui::Begin(ICON_MD_SETTINGS " Settings")) {
            if (ImGui::SliderFloat("Resolution Scale", &g_windowSettings.ResolutionScale, 0.05f, 4.0f, "%.2f")) {

                g_sceneViewerResized = g_isEditor;
                g_windowFramebufferResized = !g_isEditor;
            }
            ImGui::Checkbox("Enable FXAA", &g_isFxaaEnabled);
        }
        ImGui::End();

        /*
         * UI - Assets Viewer
         */
        if (ImGui::Begin(ICON_MD_DATA_OBJECT " Assets")) {
            auto& assets = GetAssets();
            ImGui::BeginTable("##Assets", 2, ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("Asset");
            ImGui::TableSetupColumn("X");
            ImGui::TableNextRow();
            for (auto& [assetName, asset] : assets) {
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(assetName.data());
                ImGui::TableSetColumnIndex(1);
                ImGui::PushID(assetName.data());
                if (ImGui::Button(ICON_MD_ADD_BOX " Create")) {

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
        if (ImGui::Begin(ICON_MD_APP_REGISTRATION "Hierarchy")) {

        }
        ImGui::End();

        /*
         * UI - Properties
         */
        if (ImGui::Begin(ICON_MD_ALIGN_HORIZONTAL_LEFT "Properties")) {

        }
        ImGui::End();

        /*
        * UI - Atmosphere Settings
        */
        if (ImGui::Begin(ICON_MD_SUNNY " Atmosphere")) {

            auto atmosphereModified = false;
            if (ImGui::SliderFloat3("Sun Position", glm::value_ptr(g_atmosphereSunPosition), -10000.0f, 10000.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Planet Radius", &g_atmospherePlanetRadius, 1.0f, 7000.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat3("Ray Origin", glm::value_ptr(g_atmosphereRayOrigin), -10000.0f, 10000.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Sun Intensity", &g_atmosphereSunIntensity, 0.1f, 40.0f)) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat3("Coefficients", glm::value_ptr(g_atmosphereRayleighScatteringCoefficient), 0.0f, 0.01f, "%.5f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Mie Scattering Coeff", &g_atmosphereMieScatteringCoefficient, 0.001f, 0.1f, "%.5f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Mie Scale Height", &g_atmosphereMieScaleHeight, 0.01f, 6000.0f, "%.2f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Mie Scattering Direction", &g_atmosphereMiePreferredScatteringDirection, -1.0f, 1.0f, "%.001f")) {
                atmosphereModified = true;
            }
            if (ImGui::SliderFloat("Rayleigh Scale Height", &g_atmosphereRayleighScaleHeight, -1000, 1000, "%0.01f")) {
                atmosphereModified = true;
            }

            if (atmosphereModified) {

                g_atmosphereSettings = TAtmosphereSettings{
                    .SunPositionAndPlanetRadius = glm::vec4(g_atmosphereSunPosition, g_atmospherePlanetRadius),
                    .RayOriginAndSunIntensity = glm::vec4(g_atmosphereRayOrigin, g_atmosphereSunIntensity),
                    .RayleighScatteringCoefficientAndAtmosphereRadius = glm::vec4(g_atmosphereRayleighScatteringCoefficient, g_atmosphereAtmosphereRadius),
                    .MieScatteringCoefficient = g_atmosphereMieScatteringCoefficient,
                    .MieScaleHeight = g_atmosphereMieScaleHeight,
                    .MiePreferredScatteringDirection = g_atmosphereMiePreferredScatteringDirection,
                    .RayleighScaleHeight = g_atmosphereRayleighScaleHeight
                };
                UpdateBuffer(g_atmosphereSettingsBuffer, 0, sizeof(TAtmosphereSettings), &g_atmosphereSettings);

                atmosphereModified = false;
            }
        }
        ImGui::End();
    }
    {
        PROFILER_ZONESCOPEDN("Draw ImGUI");
        ImGui::Render();
        auto* imGuiDrawData = ImGui::GetDrawData();
        if (imGuiDrawData != nullptr) {
            PushDebugGroup("UI");
            glDisable(GL_FRAMEBUFFER_SRGB);
            g_isSrgbDisabled = true;
            glViewport(0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y);
            ImGui_ImplOpenGL3_RenderDrawData(imGuiDrawData);
            PopDebugGroup();
        }
    }
}

auto RendererResizeWindowFramebuffer(int32_t width, int32_t height) -> void {

    g_windowFramebufferSize = glm::ivec2{width, height};
    g_windowFramebufferResized = true;
}

auto RendererToggleEditorMode() -> void {
    g_isEditor = !g_isEditor;
    g_windowFramebufferResized = !g_isEditor;
    g_sceneViewerResized = g_isEditor;
}
