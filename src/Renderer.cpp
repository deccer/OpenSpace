#include "Renderer.hpp"
#include "RHI.hpp"
#include "Components.hpp"
#include "Assets.hpp"
#include "Images.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <entt/entity/fwd.hpp>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

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

enum class TImGuizmoOperation {
    Translate,
    Rotate,
    Scale
};

enum class TImGuizmoMode {
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
    glm::vec4 CameraPosition;
    glm::vec4 CameraDirection;
};

struct TGpuObject {
    glm::mat4x4 WorldMatrix;
    glm::ivec4 InstanceParameter;
};

constexpr auto MAX_GLOBAL_LIGHTS = 8;
constexpr auto MAX_SHADOW_CASCADES = 4;
constexpr auto SHADOW_MAP_SIZE = 1024;
constexpr auto MAX_SHADOW_FARPLANE = 2048.0f;

struct TGpuGlobalLight {
    alignas(16) glm::mat4 LightViewProjectionMatrix[MAX_SHADOW_CASCADES] = {}; // 16 * 4 = 64
    alignas(16) float CascadeSplitDistances[8] = {}; // 8 * 4 = 32
    alignas(16) glm::vec4 Direction = {};
    alignas(16) glm::vec4 ColorAndIntensity = {};
    alignas(16) glm::ivec4 LightProperties = {1, 0, 0, 0}; // IsEnabled, CanCastShadows, padding, padding
    alignas(16) glm::ivec4 _padding1 = {};
};

entt::entity g_playerEntity = entt::null;

TWindowSettings g_windowSettings = {};

int32_t g_sceneViewerTextureIndex = {};
ImVec2 g_sceneViewerImagePosition = {};

struct TEnvironmentMaps {
    uint32_t EnvironmentMap;
    uint32_t IrradianceMap;
    uint32_t PrefilteredRadianceMap;
    uint32_t BrdfIntegrationMap;
};

struct TDebugLinesPass {
    bool IsEnabled = true;
    std::vector<TGpuDebugLine> DebugLines;
    uint32_t InputLayout = 0;
    uint32_t VertexBuffer = 0;
    TGraphicsPipeline Pipeline = {};
} g_debugLinesPass;

struct TDepthPrePass {
    TFramebuffer Framebuffer = {};
    TGraphicsPipeline Pipeline = {};
} g_depthPrePass;

struct TShadowPass {

    TShadowPass() {
        spdlog::info("Initializing shadow pass");
    }

    TShadowPass(const TShadowPass&) = delete;
    TShadowPass(TShadowPass&&) = delete;
    TShadowPass& operator=(const TShadowPass&) = delete;

    std::array<TFramebuffer, MAX_GLOBAL_LIGHTS * MAX_SHADOW_CASCADES> ShadowMapFramebuffers;
    TTextureId ShadowMapTexture = TTextureId::Invalid;
    uint32_t ShadowMapResolution = SHADOW_MAP_SIZE;
    float CascadeSplitLambda = 0.5f;
    TGraphicsPipeline Pipeline = {};
} g_shadowPass;

struct TGlobalLightPass {
    TFramebuffer Framebuffer = {};
    TGraphicsPipeline Pipeline = {};
} g_globalLightPass;

struct TGeometryPass {
    TFramebuffer Framebuffer = {};
    TGraphicsPipeline Pipeline = {};
} g_geometryPass;

struct TComposePass {
    TFramebuffer Framebuffer = {};
    TGraphicsPipeline Pipeline = {};
} g_composePass;

struct TFxaaPass {
    TFramebuffer Framebuffer = {};
    TGraphicsPipeline Pipeline = {};
    bool IsEnabled = false;
} g_fxaaPass;

std::array<TGpuGlobalLight, MAX_GLOBAL_LIGHTS> g_gpuGlobalLights;
uint32_t g_gpuGlobalLightsBuffer = 0;

TGpuGlobalUniforms g_globalUniforms = {};

uint32_t g_globalUniformsBuffer = 0;
uint32_t g_objectsBuffer = 0;

TGraphicsPipeline g_fstGraphicsPipeline = {};
TSamplerId g_fstSamplerNearestClampToEdge = {};
TSamplerId g_shadowSampler = {};

glm::vec2 g_scaledFramebufferSize = {};
glm::ivec2 g_windowFramebufferSize = {};
glm::ivec2 g_windowFramebufferScaledSize = {};
bool g_windowFramebufferResized = false;

glm::ivec2 g_sceneViewerSize = {};
glm::ivec2 g_sceneViewerScaledSize = {};
bool g_sceneViewerResized = false;
bool g_isEditor = false;

TEnvironmentMaps g_environmentMaps = {};

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
    const Renderer::TRenderContext& renderContext,
    glm::vec2 viewportContentSize,
    glm::vec2 viewportContentOffset,
    bool isViewportHovered) -> void;
auto UiSetStyleDark() -> void;
auto UiSetStyleValve() -> void;
auto UiSetStyleDarkWithFuchsia() -> void;
auto UiSetStylePurple() -> void;
auto UiSetStyleNeon() -> void;

auto inline AddDebugLine(
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec4& startColor = glm::vec4(1.0f),
    const glm::vec4& endColor = glm::vec4(1.0f)
    ) -> void;

struct TGpuVertexPosition {
    glm::vec4 Position;
};

struct TGpuVertexNormalTangentUvTangentSign {
    glm::vec4 Normal;
    glm::vec4 Tangent;
    glm::vec4 UvAndTangentSign;
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

struct TCpuTexture {
    uint32_t Id;
    std::optional<uint32_t> SamplerId;
    std::optional<uint64_t> BindlessHandle;
};

enum TCpuTextureFlag {
    HasBaseColor = 1 << 0,
    HasNormal = 1 << 1,
    HasArm = 1 << 2,
    HasEmissive = 1 << 3,
};

struct TCpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 NormalStrengthRoughnessMetalnessEmissiveStrength; // use .x = normal strength, .y = roughness, .z = metalness, .w = emissive strength
    glm::vec4 EmissiveColor;

    TCpuTexture BaseColorTexture;
    TCpuTexture NormalTexture;
    TCpuTexture ArmTexture;
    TCpuTexture MetallicRoughnessTexture;
    TCpuTexture EmissiveTexture;

    uint32_t TextureFlags = 0;
};

struct TGpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = normal strength .y = metalness, .z = roughness, .w = emission strength

    uint64_t BaseColorTexture;
    uint64_t NormalTexture;

    uint64_t ArmTexture;
    uint64_t EmissiveTexture;

    uint64_t MetallicRoughnessTexture;
    uint64_t _padding1;
};

/*
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
*/

bool g_globalLightsWasModified = true;

std::unordered_map<std::string, TGpuMesh> g_gpuMeshes = {};
std::unordered_map<std::string, TSampler> g_gpuSamplers = {};
std::unordered_map<std::string, TCpuMaterial> g_cpuMaterials = {};
std::unordered_map<std::string, TGpuMaterial> g_gpuMaterials = {};

inline auto ComputeShadowCascades(
    TGpuGlobalLight& globalLight,
    const glm::mat4& cameraViewMatrix,
    const glm::vec3& cameraRight,
    const float fov,
    const float aspectRatio,
    const float cameraNearPlane,
    const float cameraFarPlane) -> void {

    for (auto cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex) {
        const auto ratio = cascadeIndex / static_cast<float>(MAX_SHADOW_CASCADES);
        const auto logarithmic = cameraNearPlane * glm::pow(cameraFarPlane / cameraNearPlane, ratio);
        const auto linear = cameraNearPlane + (cameraFarPlane - cameraNearPlane) * ratio;
        globalLight.CascadeSplitDistances[cascadeIndex] = glm::mix(logarithmic, linear, 0.2f);
    }
    globalLight.CascadeSplitDistances[MAX_SHADOW_CASCADES] = cameraFarPlane;

    for (auto cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex) {
        auto subFrustumProjection = glm::perspective(
            fov,
            aspectRatio,
            globalLight.CascadeSplitDistances[cascadeIndex],
            globalLight.CascadeSplitDistances[cascadeIndex + 1]
        );

        const auto invProjectionView = glm::inverse(subFrustumProjection * cameraViewMatrix);
        glm::vec4 frustumCorners[8];
        auto frustumCenter = glm::vec3(0);

        for (auto x = 0u; x < 2u; ++x) {
            for (auto y = 0u; y < 2u; ++y) {
                for (auto z = 0u; z < 2u; ++z) {
                    const auto corner = invProjectionView * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                    const auto cornerWDivided = corner / corner.w;
                    frustumCorners[x * 4 + y * 2 + z] = cornerWDivided;
                    frustumCenter += glm::vec3(cornerWDivided);
                }
            }
        }
        frustumCenter /= 8;

        const auto lightDirection = glm::normalize(globalLight.Direction.xyz());
        auto lightView = glm::lookAt(
            frustumCenter + lightDirection,
            frustumCenter,
            glm::vec3(0.0f, 1.0f, 0.0f)//glm::cross(-lightDirection, cameraRight)
        );

        constexpr glm::vec4 colors[] = {
            glm::vec4(1, 0, 0, 1),
            glm::vec4(0, 1, 0, 1),
            glm::vec4(0, 0, 1, 1),
            glm::vec4(1, 1, 0, 1)
        };
        const glm::vec4& cascadeColor = colors[cascadeIndex % MAX_SHADOW_CASCADES];

        // near plane
        AddDebugLine(frustumCorners[0], frustumCorners[4], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[4], frustumCorners[6], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[6], frustumCorners[2], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[2], frustumCorners[0], cascadeColor, cascadeColor);

        // far plane
        AddDebugLine(frustumCorners[1], frustumCorners[5], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[5], frustumCorners[7], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[7], frustumCorners[3], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[3], frustumCorners[1], cascadeColor, cascadeColor);

        // near to far
        AddDebugLine(frustumCorners[0], frustumCorners[1], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[4], frustumCorners[5], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[6], frustumCorners[7], cascadeColor, cascadeColor);
        AddDebugLine(frustumCorners[2], frustumCorners[3], cascadeColor, cascadeColor);

        auto minX = std::numeric_limits<float>::max();
        auto maxX = std::numeric_limits<float>::lowest();
        auto minY = std::numeric_limits<float>::max();
        auto maxY = std::numeric_limits<float>::lowest();
        auto minZ = std::numeric_limits<float>::max();
        auto maxZ = std::numeric_limits<float>::lowest();
        for (const auto& frustumCorner : frustumCorners) {
            const auto trf = lightView * frustumCorner;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        /*
        constexpr float zMult = 2.0f;
        if (minZ < 0) {
            minZ *= zMult;
        } else {
            minZ /= zMult;
        }
        if (maxZ < 0) {
            maxZ /= zMult;
        } else {
            maxZ *= zMult;
        }
        */

        auto lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ - 32, maxZ + 32);

        globalLight.LightViewProjectionMatrix[cascadeIndex] = lightProjection * lightView;
    }
}

auto ComputeIrradianceMap(const TTextureId textureId) -> std::expected<TTextureId, std::string> {

    auto computeIrradianceMapComputePipelineResult = CreateComputePipeline(TComputePipelineDescriptor{
        .Label = "Compute Irradiance Map",
        .ComputeShaderFilePath = "data/shaders/ComputeIrradianceMap.cs.glsl",
    });

    if (!computeIrradianceMapComputePipelineResult) {
        return std::unexpected(computeIrradianceMapComputePipelineResult.error());
    }

    const auto& environmentTexture = GetTexture(textureId);

    auto computeIrradianceMapComputePipeline = *computeIrradianceMapComputePipelineResult;

    constexpr auto width = 64;
    constexpr auto height = 64;
    constexpr auto format = TFormat::R16G16B16A16_FLOAT;

    auto convolvedTextureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::TextureCube,
        .Format = format,
        .Extent = TExtent3D(width, height, 1),
        .MipMapLevels = 1,
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = std::format("TextureCube-{}x{}-Irradiance", width, height),
    });

    const auto& convolvedTexture = GetTexture(convolvedTextureId);

    constexpr auto groupX = static_cast<uint32_t>(width / 32);
    constexpr auto groupY = static_cast<uint32_t>(height / 32);
    constexpr auto groupZ = 6u;

    computeIrradianceMapComputePipeline.Bind();
    computeIrradianceMapComputePipeline.BindTexture(0, environmentTexture.Id);
    computeIrradianceMapComputePipeline.BindImage(0, convolvedTexture.Id, 0, 0, TMemoryAccess::WriteOnly, format);
    computeIrradianceMapComputePipeline.Dispatch(groupX, groupY, groupZ);

    computeIrradianceMapComputePipeline.InsertMemoryBarrier(TMemoryBarrierMaskBits::ShaderImageAccess);
    GenerateMipmaps(convolvedTextureId);

    return convolvedTextureId;
}

auto ComputePrefilteredRadianceMap(
    const TTextureId environmentMapTextureId,
    const int32_t prefilteredMapBaseSize = 512) -> std::expected<TTextureId, std::string> {

    auto computePrefilteredRadianceMapComputePipelineResult = CreateComputePipeline(TComputePipelineDescriptor{
        .Label = "Compute Prefiltered Radiance Map",
        .ComputeShaderFilePath = "data/shaders/ComputePrefilteredRadianceMap.cs.glsl",
    });

    if (!computePrefilteredRadianceMapComputePipelineResult) {
        return std::unexpected(computePrefilteredRadianceMapComputePipelineResult.error());
    }

    auto computePrefilteredRadianceMapComputePipeline = *computePrefilteredRadianceMapComputePipelineResult;

    const auto maxMipLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(static_cast<float>(prefilteredMapBaseSize))));
    constexpr auto format = TFormat::R16G16B16A16_FLOAT;

    auto prefilteredEnvironmentMapId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::TextureCube,
        .Format = format,
        .Extent = TExtent3D(prefilteredMapBaseSize, prefilteredMapBaseSize, 1),
        .MipMapLevels = maxMipLevels,
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = std::format("TextureCube-{}x{}-PrefilteredRadiance", prefilteredMapBaseSize, prefilteredMapBaseSize),
    });

    const auto& environmentTexture = GetTexture(environmentMapTextureId);
    const auto& prefilteredEnvironmentMap = GetTexture(prefilteredEnvironmentMapId);

    computePrefilteredRadianceMapComputePipeline.Bind();
    computePrefilteredRadianceMapComputePipeline.BindTexture(0, environmentTexture.Id);

    for (auto mipLevel = 0; mipLevel < maxMipLevels; mipLevel++) {
        const auto mipSize = prefilteredMapBaseSize >> mipLevel;
        const auto roughness = static_cast<float>(mipLevel) / static_cast<float>(maxMipLevels - 1);

        computePrefilteredRadianceMapComputePipeline.BindImage(0, prefilteredEnvironmentMap.Id, mipLevel, 0, TMemoryAccess::WriteOnly, format);
        computePrefilteredRadianceMapComputePipeline.SetUniform(0, roughness);

        const auto numGroups = (mipSize + 31) / 32;
        computePrefilteredRadianceMapComputePipeline.Dispatch(numGroups, numGroups, 6u);

        computePrefilteredRadianceMapComputePipeline.InsertMemoryBarrier(TMemoryBarrierMaskBits::ShaderImageAccess);
    }

    return prefilteredEnvironmentMapId;
}

auto ComputeSHCoefficients(const TTextureId irradianceMapTextureId) -> std::expected<uint32_t, std::string> {

    auto computeSHCoefficientsComputePipelineResult = CreateComputePipeline(TComputePipelineDescriptor{
        .Label = "Compute SH Coefficients",
        .ComputeShaderFilePath = "data/shaders/ComputeSHCoefficients.cs.glsl",
    });

    if (!computeSHCoefficientsComputePipelineResult) {
        return std::unexpected(computeSHCoefficientsComputePipelineResult.error());
    }

    auto computeSHCoefficientsComputePipeline = *computeSHCoefficientsComputePipelineResult;
    auto shCoefficientBuffer = CreateBuffer("CoefficientBuffer", sizeof(float) * 3 * 9, nullptr, GL_DYNAMIC_STORAGE_BIT);
    constexpr auto zeroValue = 0.0f;
    UpdateBuffer(shCoefficientBuffer, 0, sizeof(float) * 3 * 9, &zeroValue);

    const auto& convolvedTexture = GetTexture(irradianceMapTextureId);
    const auto faceSize = convolvedTexture.Extent.Width;

    const auto groupX = faceSize / 8;
    const auto groupY = faceSize / 8;
    constexpr auto groupZ = 6u;

    computeSHCoefficientsComputePipeline.Bind();
    computeSHCoefficientsComputePipeline.BindTexture(0, convolvedTexture.Id);
    computeSHCoefficientsComputePipeline.BindBufferAsShaderStorageBuffer(shCoefficientBuffer, 1);
    computeSHCoefficientsComputePipeline.SetUniform(0, faceSize);
    computeSHCoefficientsComputePipeline.Dispatch(groupX, groupY, groupZ);

    computeSHCoefficientsComputePipeline.InsertMemoryBarrier(TMemoryBarrierMaskBits::ShaderImageAccess);

    return shCoefficientBuffer;
}

auto LoadEnvironmentMaps(const std::string& skyBoxName) -> std::expected<TEnvironmentMaps, std::string> {

    TEnvironmentMaps environmentMaps = {};

    const std::array<std::string, 6> skyBoxNames = {
        std::format("data/sky/TC_{}_Xp.png", skyBoxName),
        std::format("data/sky/TC_{}_Xn.png", skyBoxName),
        std::format("data/sky/TC_{}_Yp.png", skyBoxName),
        std::format("data/sky/TC_{}_Yn.png", skyBoxName),
        std::format("data/sky/TC_{}_Zp.png", skyBoxName),
        std::format("data/sky/TC_{}_Zn.png", skyBoxName),
    };

    TTextureId environmentMapId = {};

    for (auto imageIndex = 0; imageIndex < skyBoxNames.size(); imageIndex++) {

        auto imageName = skyBoxNames[imageIndex];
        int32_t imageWidth = 0;
        int32_t imageHeight = 0;
        int32_t imageComponents = 0;
        const auto imageData = Image::LoadImageFromFile(imageName, &imageWidth, &imageHeight, &imageComponents);

        if (imageIndex == 0) {
            environmentMapId = CreateTexture(TCreateTextureDescriptor{
                .TextureType = TTextureType::TextureCube,
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent3D{ static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1u},
                .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageWidth), static_cast<float>(imageHeight))))),
                .Layers = 6,
                .SampleCount = TSampleCount::One,
                .Label = std::format("TextureCube-{}x{}-{}", imageWidth, imageHeight, skyBoxName),
            });
        }

        UploadTexture(environmentMapId, TUploadTextureDescriptor{
            .Level = 0,
            .Offset = TOffset3D{0, 0, static_cast<uint32_t>(imageIndex)},
            .Extent = TExtent3D{static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1u},
            .UploadFormat = TUploadFormat::Auto,
            .UploadType = TUploadType::Auto,
            .PixelData = imageData
        });

        Image::FreeImage(imageData);
    }

    GenerateMipmaps(environmentMapId);

    environmentMaps.EnvironmentMap = GetTexture(environmentMapId).Id;

    const auto computeIrradianceMapResult = ComputeIrradianceMap(environmentMapId);
    if (!computeIrradianceMapResult) {
        return std::unexpected(std::format("Unable to compute irradiance map from environment map {}", computeIrradianceMapResult.error()));
    }
    environmentMaps.IrradianceMap = GetTexture(*computeIrradianceMapResult).Id;

    const auto computePrefilteredRadianceMapResult = ComputePrefilteredRadianceMap(environmentMapId);
    if (!computePrefilteredRadianceMapResult) {
        return std::unexpected(std::format("Unable to compute prefiltered radiance map from environment map {}", computePrefilteredRadianceMapResult.error()));
    }

    environmentMaps.PrefilteredRadianceMap = GetTexture(*computePrefilteredRadianceMapResult).Id;

    const auto brdfLutMapId = CreateTexture2DFromFile(
        "data/default/T_Lut_Brdf.png",
        TFormat::R8G8B8A8_SRGB,
        false);

    environmentMaps.BrdfIntegrationMap = GetTexture(brdfLutMapId).Id;

    return environmentMaps;
}

/*
inline auto SignNotZero(const glm::vec2 v) -> glm::vec2 {
    return glm::vec2(v.x >= 0.0f ? +1.0f : -1.0f, v.y >= 0.0f ? +1.0f : -1.0f);
}

inline auto EncodeOctahedral(const glm::vec3 input) -> glm::vec2 {
    const auto encoded = glm::vec2{input.x, input.y} * (1.0f / (abs(input.x) + abs(input.y) + abs(input.z)));
    return input.z <= 0.0f
        ? (1.0f - glm::abs(glm::vec2{encoded.y, encoded.x})) * SignNotZero(encoded)
        : encoded;
}
*/

glm::vec2 oct_wrap(glm::vec2 v)
{
    return glm::vec2{
        (1.0f - abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f),
        (1.0f - abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f)
    };
}

glm::vec2 EncodeOctahedral(glm::vec3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    return n.z >= 0.0f ? n.xy() : oct_wrap(n.xy());
}

inline auto DecodeOctahedral(const glm::vec2 encoded) -> glm::vec3 {
    const auto decoded = glm::vec3(glm::vec2(encoded.x, encoded.y), 1.0f - abs(encoded.x) - abs(encoded.y));
    const auto signNotZero = glm::vec2(decoded.x >= 0.0f ? +1.0f : -1.0f, decoded.y >= 0.0f ? +1.0f : -1.0f);
    if (decoded.z < 0.0f) {
        glm::vec2(decoded.x, decoded.y) = (1.0f - abs(glm::vec2(decoded.y, decoded.x))) * signNotZero;
    }
    return normalize(decoded);
}

inline bool contains_nan(const glm::vec3 a)
{
    return isnan(a.x) || isnan(a.y) || isnan(a.z);
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
            glm::vec4(assetPrimitive.Positions[i], 0.0f),
        };

        vertexNormalUvTangents[i] = {
            glm::packSnorm2x16(EncodeOctahedral(glm::normalize(assetPrimitive.Normals[i]))),
            glm::packSnorm2x16(EncodeOctahedral(glm::normalize(assetPrimitive.Tangents[i].xyz()))),
            glm::vec4(assetPrimitive.Uvs[i], assetPrimitive.Tangents[i].w, 0.0f),
        };
    }

    uint32_t buffers[3] = {};
    {
        PROFILER_ZONESCOPEDN("Create GL Buffers + Upload Data");

        glCreateBuffers(3, buffers);
        SetDebugLabel(buffers[0], GL_BUFFER, std::format("Geometry Pass-{}-Position", label));
        SetDebugLabel(buffers[1], GL_BUFFER, std::format("Geometry Pass-{}-Normal-Tangent-UvTangentSign", label));
        SetDebugLabel(buffers[2], GL_BUFFER, std::format("Geometry Pass-{}-Indices", label));
        glNamedBufferStorage(buffers[0], sizeof(TGpuVertexPosition) * vertexPositions.size(),
                                vertexPositions.data(), 0);
        glNamedBufferStorage(buffers[1], sizeof(TGpuPackedVertexNormalTangentUvTangentSign) * vertexNormalUvTangents.size(),
                                vertexNormalUvTangents.data(), 0);
        glNamedBufferStorage(buffers[2], sizeof(uint32_t) * assetPrimitive.Indices.size(), assetPrimitive.Indices.data(), 0);
    }

    {
        PROFILER_ZONESCOPEDN("Add Gpu Mesh");
        g_gpuMeshes[label] = TGpuMesh{
            .Name = label,
            .VertexPositionBuffer = buffers[0],
            .VertexNormalUvTangentBuffer = buffers[1],
            .IndexBuffer = buffers[2],

            .VertexCount = vertexPositions.size(),
            .IndexCount = assetPrimitive.Indices.size(),
        };
    }
}

auto GetGpuMesh(const std::string_view assetMeshName) -> TGpuMesh& {
    assert(!assetMeshName.empty());

    return g_gpuMeshes[assetMeshName.data()];
}

auto GetCpuMaterial(const std::string_view assetMaterialName) -> TCpuMaterial& {
    assert(!assetMaterialName.empty());

    return g_cpuMaterials[assetMaterialName.data()];
}

auto GetGpuMaterial(const std::string_view assetMaterialName) -> TGpuMaterial& {
    assert(!assetMaterialName.empty());

    return g_gpuMaterials[assetMaterialName.data()];
}

auto CreateResidentTextureForMaterialChannel(const std::string_view materialDataName) -> int64_t {

    PROFILER_ZONESCOPEDN("CreateResidentTextureForMaterialChannel");

    const auto& imageData = Assets::GetAssetImage(materialDataName.data());

    const auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u },
        .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = std::format("Texture-{}x{}-{}-Resident", imageData.Width, imageData.Height, imageData.Name),
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0 },
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u },
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
        .Format = channel == Assets::TAssetMaterialChannel::Normals
            ? TFormat::R8G8B8A8_UNORM
            : TFormat::R8G8B8A8_SRGB,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u },
        .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = std::format("Texture-{}x{}-{}", imageData.Width, imageData.Height, imageData.Name),
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0 },
        .Extent = TExtent3D{ static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u },
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
        .NormalStrengthRoughnessMetalnessEmissiveStrength = {
            assetMaterialData.NormalStrength,
            assetMaterialData.Roughness,
            assetMaterialData.Metalness,
            assetMaterialData.EmissiveFactor
        },
        .EmissiveColor = glm::vec4(assetMaterialData.EmissiveColor, 1.0f),
    };

    const auto& baseColorTextureChannel = assetMaterialData.BaseColorTextureChannel;
    if (baseColorTextureChannel) {
        const auto& [baseColorChannel, baseColorSamplerName, baseColorTextureName] = *baseColorTextureChannel;
        const auto& baseColorTextureSampler = Assets::GetAssetSampler(baseColorSamplerName);
        const auto baseColorSampler = GetOrCreateSampler(CreateSamplerDescriptor(baseColorTextureSampler));
        const auto& [baseColorSamplerId] = GetSampler(baseColorSampler);

        cpuMaterial.BaseColorTexture = {
            .Id = CreateTextureForMaterialChannel(baseColorTextureName, baseColorChannel),
            .SamplerId = baseColorSamplerId,
            .BindlessHandle = std::nullopt,
        };
        cpuMaterial.TextureFlags |= TCpuTextureFlag::HasBaseColor;
    }

    const auto& normalTextureChannel = assetMaterialData.NormalTextureChannel;
    if (normalTextureChannel) {
        const auto& [normalChannel, normalSamplerName, normalTextureName] = *normalTextureChannel;
        const auto& normalTextureSampler = Assets::GetAssetSampler(normalSamplerName);
        const auto normalSampler = GetOrCreateSampler(CreateSamplerDescriptor(normalTextureSampler));
        const auto& [normalSamplerId] = GetSampler(normalSampler);

        cpuMaterial.NormalTexture = {
            .Id = CreateTextureForMaterialChannel(normalTextureName, normalChannel),
            .SamplerId = normalSamplerId,
            .BindlessHandle = std::nullopt,
        };
        cpuMaterial.TextureFlags |= TCpuTextureFlag::HasNormal;
    }

    const auto& armTextureChannel = assetMaterialData.ArmTextureChannel;
    if (armTextureChannel) {
        const auto& [armChannel, armSamplerName, armTextureName] = *armTextureChannel;
        const auto& armTextureSampler = Assets::GetAssetSampler(armSamplerName);
        const auto armSampler = GetOrCreateSampler(CreateSamplerDescriptor(armTextureSampler));
        const auto& [armSamplerId] = GetSampler(armSampler);

        cpuMaterial.ArmTexture = {
            .Id = CreateTextureForMaterialChannel(armTextureName, armChannel),
            .SamplerId = armSamplerId,
            .BindlessHandle = std::nullopt,
        };
        cpuMaterial.TextureFlags |= TCpuTextureFlag::HasArm;
    }

    const auto& metallicRoughnessTextureChannel = assetMaterialData.MetallicRoughnessTextureChannel;
    if (metallicRoughnessTextureChannel) {
        const auto& [metallicRoughnessChannel, metallicRoughnessSamplerName, metallicRoughnessTextureName] = *metallicRoughnessTextureChannel;
        const auto& metallicRoughnessTextureSampler = Assets::GetAssetSampler(metallicRoughnessSamplerName);
        const auto metallicRoughnessSampler = GetOrCreateSampler(CreateSamplerDescriptor(metallicRoughnessTextureSampler));
        const auto& [metallicRoughnessSamplerId] = GetSampler(metallicRoughnessSampler);

        cpuMaterial.MetallicRoughnessTexture = {
            .Id = CreateTextureForMaterialChannel(metallicRoughnessTextureName, metallicRoughnessChannel),
            .SamplerId = metallicRoughnessSamplerId,
            .BindlessHandle = std::nullopt,
        };
        cpuMaterial.TextureFlags |= TCpuTextureFlag::HasArm;
    }

    const auto& emissiveTextureChannel = assetMaterialData.EmissiveTextureChannel;
    if (emissiveTextureChannel) {
        const auto& [emissiveChannel, emissiveSamplerName, emissiveTextureName] = *emissiveTextureChannel;
        const auto& emissiveTextureSampler = Assets::GetAssetSampler(emissiveSamplerName);
        const auto emissiveSampler = GetOrCreateSampler(CreateSamplerDescriptor(emissiveTextureSampler));
        const auto& [emissiveSamplerId] = GetSampler(emissiveSampler);

        cpuMaterial.EmissiveTexture = {
            .Id = CreateTextureForMaterialChannel(emissiveTextureName, emissiveChannel),
            .SamplerId = emissiveSamplerId,
            .BindlessHandle = std::nullopt,
        };
        cpuMaterial.TextureFlags |= TCpuTextureFlag::HasEmissive;
    }

    g_cpuMaterials[assetMaterialName] = cpuMaterial;
}

auto DeleteRendererFramebuffers() -> void {

    DeleteFramebuffer(g_depthPrePass.Framebuffer);
    DeleteFramebuffer(g_geometryPass.Framebuffer);
    DeleteFramebuffer(g_composePass.Framebuffer);
    DeleteFramebuffer(g_fxaaPass.Framebuffer);
}

auto CreateRendererFramebuffers(const glm::vec2& scaledFramebufferSize) -> void {

    PROFILER_ZONESCOPEDN("CreateRendererFramebuffers");

    g_depthPrePass.Framebuffer = CreateFramebuffer({
        .Label = "Depth PrePass-FBO",
        .DepthStencilAttachment = TFramebufferDepthStencilAttachmentDescriptor{
            .Label = std::format("Depth PrePass-FBO-Depth-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
            .Format = TFormat::D24_UNORM_S8_UINT,
            .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
            .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
            .ClearDepthStencil = { 1.0f, 0 },
        }
    });

    g_geometryPass.Framebuffer = CreateFramebuffer({
        .Label = "Geometry Pass-FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Geometry Pass-Albedo-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Geometry Pass-Normals-Ao-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R32G32B32A32_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Geometry Pass-Velocity-Roughness-Metalness-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R16G16B16A16_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Geometry Pass-Emissive-ViewZ-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R16G16B16A16_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f },
            }
        },
        .DepthStencilAttachment = TFramebufferExistingDepthStencilAttachmentDescriptor{
            .ExistingDepthTextureId = g_depthPrePass.Framebuffer.DepthStencilAttachment.value().TextureId,
        }
    });

    g_globalLightPass.Framebuffer = CreateFramebuffer({
        .Label = "Global Light Pass-FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Global Light Pass-FBO-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R16G16B16A16_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Load,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f },
            }
        }
    });

    g_composePass.Framebuffer = CreateFramebuffer({
        .Label = "Compose Pass-FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Compose Pass-FBO-Output-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        },
    });

    g_fxaaPass.Framebuffer = CreateFramebuffer({
        .Label = "Fxaa Pass-FBO",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = std::format("Fxaa Pass-FBO-Output-{}x{}", scaledFramebufferSize.x, scaledFramebufferSize.y),
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f }
            }
        }
    });
}

auto Renderer::Initialize(
    const TWindowSettings& windowSettings,
    GLFWwindow* window,
    const glm::vec2& initialFramebufferSize) -> bool {

    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        spdlog::error("Failed to initialize GLAD");
        return false;
    }
    spdlog::info("GLAD loaded");

    RhiInitialize(windowSettings.IsDebug);

    g_windowSettings = windowSettings;

#ifdef USE_PROFILER
    TracyGpuContext
#endif

    if (!UiInitialize(window)) {
        spdlog::error("Failed to initialize UI");
        return false;
    }
    spdlog::info("UI initialized");

    glfwSwapInterval(g_windowSettings.IsVSyncEnabled ? 1 : 0);

    glDisable(GL_MULTISAMPLE);
    glDisable(GL_DITHER);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glClearColor(0.03f, 0.05f, 0.07f, 1.0f);

    Assets::AddDefaultAssets();

    const auto loadSkyTextureResult = LoadEnvironmentMaps("Kloofendal_MistyMorning");
    if (!loadSkyTextureResult.has_value()) {
        spdlog::error("Failed to load sky texture: {}", loadSkyTextureResult.error());
        return false;
    }
    g_environmentMaps = *loadSkyTextureResult;
    spdlog::info("Sky textures loaded");

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
            .BlendStates = {
                TBlendStateDescriptor {
                    .IsBlendEnabled = false,
                    .ColorWriteMask = TColorMaskBits::None
                }
            },
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = true,
                .DepthCompareFunction = TCompareFunction::Less
            },
        }
    });
    if (!depthPrePassGraphicsPipelineResult) {
        spdlog::error(depthPrePassGraphicsPipelineResult.error());
        return false;
    }
    g_depthPrePass.Pipeline = *depthPrePassGraphicsPipelineResult;
    spdlog::info("Depth PrePass pipeline initialized");

    auto shadowGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Shadow Pass",
        .VertexShaderFilePath = "data/shaders/Shadow.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/Shadow.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Front,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
        .OutputMergerState = {
            .BlendStates = {
                TBlendStateDescriptor {
                    .ColorWriteMask = TColorMaskBits::None
                },
            },
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = true,
                .DepthCompareFunction = TCompareFunction::Less
            },
        }
    });
    if (!shadowGraphicsPipelineResult) {
        spdlog::error(shadowGraphicsPipelineResult.error());
        return false;
    }
    g_shadowPass.Pipeline = *shadowGraphicsPipelineResult;
    g_shadowPass.ShadowMapResolution = 2048;
    g_shadowPass.CascadeSplitLambda = 0.95f;
    g_shadowPass.ShadowMapTexture = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2DArray,
        .Format = TFormat::D32_FLOAT,
        .Extent = TExtent3D(g_shadowPass.ShadowMapResolution, g_shadowPass.ShadowMapResolution, 1),
        .MipMapLevels = 1,
        .Layers = MAX_GLOBAL_LIGHTS * MAX_SHADOW_CASCADES,
        .SampleCount = TSampleCount::One,
        .Label = std::format("Shadow Pass-ShadowMap-{}x{}", g_shadowPass.ShadowMapResolution, g_shadowPass.ShadowMapResolution),
    });

    for (auto globalLightIndex = 0u; globalLightIndex < MAX_GLOBAL_LIGHTS; ++globalLightIndex) {
        for (auto cascadeIndex = 0u; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex) {
            const auto framebufferIndex = globalLightIndex * MAX_SHADOW_CASCADES + cascadeIndex;
            g_shadowPass.ShadowMapFramebuffers[framebufferIndex] = CreateFramebuffer({
                .Label = std::format("Shadow Pass-FBO-GlobalLight{}-Cascade{}", globalLightIndex, cascadeIndex),
                .DepthStencilAttachment = TFramebufferExistingDepthStencilAttachmentForSpecificLayerDescriptor{
                    .ExistingDepthTextureId = g_shadowPass.ShadowMapTexture,
                    .Layer = framebufferIndex,
                },
            });
        }
    }
    spdlog::info("Shadow Pass initialized");

    auto globalLightPassGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Global Light Pass",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/GlobalLight.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
        .OutputMergerState = {
            .BlendStates {
                TBlendStateDescriptor {
                    .IsBlendEnabled = true,
                    .ColorSourceBlendFactor = TBlendFactor::One,
                    .ColorDestinationBlendFactor = TBlendFactor::One,
                    .ColorBlendOperation = TBlendOperation::Add,
                    .AlphaSourceBlendFactor = TBlendFactor::One,
                    .AlphaDestinationBlendFactor = TBlendFactor::One,
                    .AlphaBlendOperation = TBlendOperation::Add,
                }
            }
        },
    });
    if (!globalLightPassGraphicsPipelineResult) {
        spdlog::error(globalLightPassGraphicsPipelineResult.error());
        return false;
    }
    g_globalLightPass.Pipeline = *globalLightPassGraphicsPipelineResult;

    auto geometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Geometry Pass",
        .VertexShaderFilePath = "data/shaders/Geometry.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/Geometry.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
        .OutputMergerState = {
            .BlendStates = {
                TBlendStateDescriptor {
                    .IsBlendEnabled = false,
                },
                TBlendStateDescriptor {
                    .IsBlendEnabled = false,
                },
                TBlendStateDescriptor {
                    .IsBlendEnabled = false,
                },
                TBlendStateDescriptor {
                    .IsBlendEnabled = false,
                },
            },
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = false,
                .DepthCompareFunction = TCompareFunction::Equal,
            }
        }
    });
    if (!geometryGraphicsPipelineResult) {
        spdlog::error(geometryGraphicsPipelineResult.error());
        return false;
    }
    g_geometryPass.Pipeline = *geometryGraphicsPipelineResult;

    auto composeDeferredGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Compose Pass",
        .VertexShaderFilePath = "data/shaders/ComposeDeferred.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/ComposeDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
            }
        }
    });
    if (!composeDeferredGraphicsPipelineResult) {
        spdlog::error(composeDeferredGraphicsPipelineResult.error());
        return false;
    }
    g_composePass.Pipeline = *composeDeferredGraphicsPipelineResult;

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
        .Label = "Debug Lines Pass",
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
    g_debugLinesPass.Pipeline = *debugLinesGraphicsPipelineResult;

    auto fxaaGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FXAA Pass",
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

    g_fxaaPass.Pipeline = *fxaaGraphicsPipelineResult;

    g_debugLinesPass.VertexBuffer = CreateBuffer("Debug Lines Pass-VBO", sizeof(TGpuDebugLine) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    auto cameraPosition = glm::vec3{-60.0f, -3.0f, 0.0f};
    auto cameraDirection = glm::vec3{0.0f, 0.0f, -1.0f};
    auto cameraUp = glm::vec3{0.0f, 1.0f, 0.0f};
    auto fieldOfView = glm::radians(160.0f);
    auto aspectRatio = g_scaledFramebufferSize.x / g_scaledFramebufferSize.y;

    g_globalUniforms.ProjectionMatrix = glm::infinitePerspective(fieldOfView, aspectRatio, 0.1f);
    g_globalUniforms.ViewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp);
    g_globalUniforms.CameraPosition = glm::vec4{cameraPosition, fieldOfView};
    g_globalUniforms.CameraDirection = glm::vec4{cameraDirection, aspectRatio};

    g_globalUniformsBuffer = CreateBuffer("TGpuGlobalUniforms", sizeof(TGpuGlobalUniforms), &g_globalUniforms, GL_DYNAMIC_STORAGE_BIT);
    g_objectsBuffer = CreateBuffer("TGpuObjects", sizeof(TGpuObject) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    g_gpuGlobalLights.fill(TGpuGlobalLight {
        .LightViewProjectionMatrix = {
            glm::mat4{1.0f},
            glm::mat4{1.0f},
            glm::mat4{1.0f},
            glm::mat4{1.0f},
        },
        .CascadeSplitDistances = {
            0.1f,
            0.2f,
            0.3f,
            0.4f,
            0.5f,
            0.6f,
            0.7f,
            0.8f,
        },
        .Direction = glm::vec4{0.0f, -1.0f, 0.0f, 1.0f},
        .ColorAndIntensity = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
        .LightProperties = glm::ivec4{0, 0, 0, 0}
    });
    g_gpuGlobalLightsBuffer = CreateBuffer("TGpuGlobalLights", g_gpuGlobalLights.size() * sizeof(TGpuGlobalLight), g_gpuGlobalLights.data(), GL_DYNAMIC_STORAGE_BIT);

    g_fstSamplerNearestClampToEdge = GetOrCreateSampler({
        .Label = "FST Sampler Nearest/ClampToEdge",
        .AddressModeU = TTextureAddressMode::ClampToEdge,
        .AddressModeV = TTextureAddressMode::ClampToEdge,
        .MagFilter = TTextureMagFilter::Nearest,
        .MinFilter = TTextureMinFilter::Nearest
    });

    g_shadowSampler = GetOrCreateSampler({
        .Label = "Shadow Sampler",
        .AddressModeU = TTextureAddressMode::ClampToEdge,
        .AddressModeV = TTextureAddressMode::ClampToEdge,
        .MagFilter = TTextureMagFilter::Linear,
        .MinFilter = TTextureMinFilter::Linear,
        .IsComparisonEnabled = true,
        .CompareFunction = TCompareFunction::Greater,
    });

    spdlog::info("Pipelines initialized");

    return true;
}

auto Renderer::Unload() -> void {

    DeleteBuffer(g_gpuGlobalLightsBuffer);
    DeleteBuffer(g_globalUniformsBuffer);
    DeleteBuffer(g_objectsBuffer);

    DeleteRendererFramebuffers();
    for (auto framebufferIndex = 0; framebufferIndex < MAX_GLOBAL_LIGHTS * MAX_SHADOW_CASCADES; ++framebufferIndex) {
        DeleteFramebuffer(g_shadowPass.ShadowMapFramebuffers[framebufferIndex]);
    }

    DeletePipeline(g_debugLinesPass.Pipeline);
    DeletePipeline(g_fstGraphicsPipeline);

    DeletePipeline(g_depthPrePass.Pipeline);
    DeletePipeline(g_shadowPass.Pipeline);
    DeletePipeline(g_globalLightPass.Pipeline);
    DeletePipeline(g_geometryPass.Pipeline);
    DeletePipeline(g_composePass.Pipeline);
    DeletePipeline(g_fxaaPass.Pipeline);

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
            const auto& children = registry.get<TComponentHierarchy>(entity).Children;
            for (const auto& child : children) {
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

    const auto showFloat = [&](const int axis, float* value)
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

        static constexpr ImU32 colorX = IM_COL32(168, 46, 2, 255);
        static constexpr ImU32 colorY = IM_COL32(112, 162, 22, 255);
        static constexpr ImU32 colorZ = IM_COL32(51, 122, 210, 255);

        const auto size = ImVec2(ImGui::GetFontSize() / 4.0f, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f);
        posPostLabel = ImVec2(posPostLabel.x - 1.0f, posPostLabel.y + 0.0f); // ImGui::GetStyle().FramePadding.y / 2.0f);
        const auto axisColorRectangle = ImRect(posPostLabel.x, posPostLabel.y, posPostLabel.x + size.x, posPostLabel.y + size.y);
        ImGui::GetWindowDrawList()->AddRectFilled(axisColorRectangle.Min, axisColorRectangle.Max, axis == 0 ? colorX : axis == 1 ? colorY : colorZ);
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

    if (registry.all_of<TComponentName>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Name", ImGuiTreeNodeFlags_Leaf)) {
        ImGui::Indent();
        auto& name = registry.get<TComponentName>(entity);

        ImGui::PushItemWidth(-1.0f);
        ImGui::InputText("##Name", name.Name.data(), name.Name.size());
        ImGui::PopItemWidth();

        ImGui::Unindent();
    }

    if (registry.all_of<TComponentHierarchy>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Parent", ImGuiTreeNodeFlags_Leaf)) {
        ImGui::Indent();
        const auto& hierarchy = registry.get<TComponentHierarchy>(entity);
        const auto& parentName = registry.get<TComponentName>(hierarchy.Parent);

        ImGui::TextUnformatted(parentName.Name.data());
        ImGui::Unindent();
    }

    if (registry.all_of<TComponentTransform>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Transform", ImGuiTreeNodeFlags_Leaf)) {
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

    if (registry.all_of<TComponentGlobalLight>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_WEATHER_SUNNY " Global Light", ImGuiTreeNodeFlags_Leaf)) {
        ImGui::AlignTextToFramePadding();
        float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;

        ImGui::Indent();

        auto& globalLight = registry.get<TComponentGlobalLight>(entity);

        auto& tempAzimuth = globalLight.Azimuth;
        if (ImGui::DragFloat("Azimuth", &tempAzimuth, 0.01f, 0.0f, 2.0f * glm::pi<float>(), "%.2f")) {
            globalLight.Azimuth = tempAzimuth;
        }

        auto& tempElevation = globalLight.Elevation;
        if (ImGui::DragFloat("Elevation", &tempElevation, 0.01f, -(glm::pi<float>() - 0.2f), glm::pi<float>() - 0.2f, "%.2f")) {
            globalLight.Elevation = tempElevation;
        }

        auto& tempColor = globalLight.Color;
        if (ImGui::DragFloat3("Color", &tempColor.x, 0.01f, 0.0f, 1.0f, "%.2f")) {
            globalLight.Color = tempColor;
        }

        auto& tempIntensity = globalLight.Intensity;
        if (ImGui::DragFloat("Intensity", &tempIntensity, 0.01, 0.01f, 10.0f, "%.2f")) {
            globalLight.Intensity = tempIntensity;
        }

        auto& tempIsEnabled = globalLight.IsEnabled;
        if (ImGui::Checkbox("Is Enabled", &tempIsEnabled)) {
            globalLight.IsEnabled = tempIsEnabled;
        }

        auto& tempCanCastShadow = globalLight.CanCastShadows;
        if (ImGui::Checkbox("Can Cast Shadows", &tempCanCastShadow)) {
            globalLight.CanCastShadows = tempCanCastShadow;
        }

        auto& tempIsDebugEnabled = globalLight.IsDebugEnabled;
        if (ImGui::Checkbox("Draw Debug Lines", &tempIsDebugEnabled)) {
            globalLight.IsDebugEnabled = tempIsDebugEnabled;
        }

        ImGui::Unindent();
    }

    if (registry.all_of<TComponentCamera>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Camera", ImGuiTreeNodeFlags_Leaf)) {
        auto& camera = registry.get<TComponentCamera>(entity);

        ImGui::Indent();
        ImGui::SliderFloat("Field Of View", &camera.FieldOfView, 1.0f, 179.0f);
        ImGui::Unindent();
    }

    if (registry.all_of<TComponentMesh>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Mesh", ImGuiTreeNodeFlags_Leaf)) {
        const auto& mesh = registry.get<TComponentMesh>(entity);

        ImGui::Indent();
        ImGui::PushItemWidth(-1.0f);
        ImGui::TextUnformatted(mesh.Mesh.data());
        ImGui::PopItemWidth();
        ImGui::Unindent();
    }

    if (registry.all_of<TComponentMaterial>(entity) && ImGui::CollapsingHeader((char*)ICON_MDI_CIRCLE " Material", ImGuiTreeNodeFlags_Leaf)) {
        const auto& material = registry.get<TComponentMaterial>(entity);

        ImGui::Indent();
        ImGui::PushItemWidth(-1.0f);
        ImGui::TextUnformatted(material.Material.data());
        ImGui::PopItemWidth();
        ImGui::Unindent();
    }    
}

auto PolarToCartesian(
    const float azimuth,
    const float elevation) -> glm::vec3 {

    const float x = sin(elevation) * cos(azimuth);
    const float y = sin(elevation) * sin(azimuth);
    const float z = cos(elevation);

    auto axis = glm::normalize(glm::vec3(x, y, z));
    axis = glm::rotateX(axis, -glm::radians(90.0f));
    return glm::rotateY(axis, glm::radians(90.0f));
}

auto DrawDirectionalLightMarker(
    const glm::vec3 origin,
    const glm::vec3 lightDir,
    const float arrowLength,
    const float discRadius,
    const glm::vec4 color) -> void {

    const auto dir = glm::normalize(lightDir);
    const auto tip = origin + dir * arrowLength;

    AddDebugLine(tip, origin, color, color);

    // Orthonormal basis for disc/arrowhead
    constexpr auto up = glm::vec3(0, 1, 0);
    auto right = glm::normalize(glm::cross(dir, up));
    if (glm::length(right) < 0.001f) {
        right = glm::vec3(1, 0, 0);
    }
    const auto forward = glm::normalize(glm::cross(right, dir));

    // Arrowhead lines
    const auto headOffset1 = -dir * 0.5f + right * 0.25f;
    const auto headOffset2 = -dir * 0.5f - right * 0.25f;
    AddDebugLine(tip + headOffset1, tip, color, color);
    AddDebugLine(tip + headOffset2, tip, color, glm::vec4(color.xyz() * 0.8f, 0.4f));

    // Disc at tip
    constexpr auto segments = 32;
    for (int i = 0; i < segments; ++i) {
        const auto lineSegmentStart = glm::two_pi<float>() * i / segments;
        const auto lineSegmentEnd = glm::two_pi<float>() * (i + 1) / segments;

        const auto start = tip + (cos(lineSegmentStart) * right + sin(lineSegmentStart) * forward) * discRadius;
        const auto end = tip + (cos(lineSegmentEnd) * right + sin(lineSegmentEnd) * forward) * discRadius;

        AddDebugLine(start, end, color * 0.8f, color * 0.8f);
    }
}

auto UpdateGlobalLights(entt::registry& registry) -> void {
    const auto globalLightsEntities = registry.view<TComponentGlobalLight>();

    auto globalLightIndex = 0;
    for (const auto globalLightEntity : globalLightsEntities) {
        const auto& globalLight = registry.get<TComponentGlobalLight>(globalLightEntity);
        const auto direction = PolarToCartesian(globalLight.Azimuth, globalLight.Elevation);

        g_gpuGlobalLights[globalLightIndex++] = {
            .Direction = glm::vec4{direction, 0.0f},
            .ColorAndIntensity = glm::vec4{globalLight.Color, globalLight.Intensity},
            .LightProperties = glm::ivec4{globalLight.IsEnabled, globalLight.CanCastShadows, 0, 0},
        };
        if (globalLight.IsEnabled && globalLight.IsDebugEnabled) {
            DrawDirectionalLightMarker(glm::vec3(0), direction, 2048.0f, 64.0f, glm::vec4(globalLight.Color, 1));
        }
        if (globalLightIndex >= MAX_GLOBAL_LIGHTS) {
            break;
        }
    }

    const auto cameraView = registry.view<TComponentCamera>();
    const auto camera = registry.get<TComponentCamera>(cameraView.front());
    // make sure the framebuffer is actually large enough
    if (g_scaledFramebufferSize.x * g_scaledFramebufferSize.y <= 0.0f) {
        return;
    }

    const auto cameraAspectRatio = g_scaledFramebufferSize.x / g_scaledFramebufferSize.y;

    PROFILER_ZONESCOPEDN("Update Shadow Cascades");
    {
        for (globalLightIndex = 0; globalLightIndex < MAX_GLOBAL_LIGHTS; ++globalLightIndex) {

            auto& globalLight = g_gpuGlobalLights[globalLightIndex];
            //if (globalLight.LightProperties.x == 0) {
            //    continue;
            //}

            const auto direction = globalLight.Direction.xyz();
            const auto cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), direction));
            ComputeShadowCascades(
                globalLight,
                g_globalUniforms.ViewMatrix,
                cameraRight,
                camera.FieldOfView,
                cameraAspectRatio,
                camera.NearPlane,
                MAX_SHADOW_FARPLANE);
        }

        UpdateBuffer(g_gpuGlobalLightsBuffer, 0, sizeof(TGpuGlobalLight) * MAX_GLOBAL_LIGHTS, g_gpuGlobalLights.data());
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

auto inline AddDebugLine(
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec4& startColor,
    const glm::vec4& endColor) -> void {

    g_debugLinesPass.DebugLines.push_back(TGpuDebugLine{
        .StartPosition = start,
        .StartColor = startColor,
        .EndPosition = end,
        .EndColor = endColor
    });
}

auto inline ResetDebugLines() -> void {

    if (g_debugLinesPass.IsEnabled) {
        g_debugLinesPass.DebugLines.clear();

        g_debugLinesPass.DebugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{0, 0, 0},
            .StartColor = glm::vec4{0.7f, 0.0f, 0.0f, 1.0f},
            .EndPosition = glm::vec3{128, 0, 0},
            .EndColor = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}
        });
        g_debugLinesPass.DebugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{0, 0, 0},
            .StartColor = glm::vec4{0.0f, 0.7f, 0.0f, 1.0f},
            .EndPosition = glm::vec3{0, 128, 0},
            .EndColor = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}
        });
        g_debugLinesPass.DebugLines.push_back(TGpuDebugLine{
            .StartPosition = glm::vec3{0, 0, 0},
            .StartColor = glm::vec4{0.0f, 0.0f, 0.7f, 1.0f},
            .EndPosition = glm::vec3{0, 0, 128},
            .EndColor = glm::vec4{0.0f, 0.0f, 1.0f, 1.0f}
        });
    }
}

auto inline CreateGpuResourcesIfNecessary(entt::registry& registry) -> void {
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

auto inline UpdateGlobalTransforms(entt::registry& registry) -> void {

    PROFILER_ZONESCOPEDN("Update Global Uniforms");

    registry.view<TComponentCamera>().each([&](
        const auto& entity,
        const auto& cameraComponent) {

        auto cameraMatrix = registry.get<TComponentRenderTransform>(entity);
        auto viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPosition = cameraMatrix[3];
        const glm::vec3 cameraDirection = cameraMatrix[1];

        auto aspectRatio = g_scaledFramebufferSize.x / g_scaledFramebufferSize.y;
        g_globalUniforms.ProjectionMatrix = glm::infinitePerspective(glm::radians(cameraComponent.FieldOfView), aspectRatio, cameraComponent.NearPlane);
        g_globalUniforms.ViewMatrix = glm::inverse(cameraMatrix);
        g_globalUniforms.CameraPosition = glm::vec4(cameraPosition, glm::radians(cameraComponent.FieldOfView));
        g_globalUniforms.CameraDirection = glm::vec4(cameraDirection, aspectRatio);
        UpdateBuffer(g_globalUniformsBuffer, 0, sizeof(TGpuGlobalUniforms), &g_globalUniforms);
    });
}

auto inline ResizeFramebuffersIfNecessary() -> void {
    if (g_windowFramebufferResized || g_sceneViewerResized) {

        PROFILER_ZONESCOPEDN("Resize If Necessary");

        g_windowFramebufferScaledSize = glm::ivec2{
            g_windowFramebufferSize.x * g_windowSettings.ResolutionScale,
            g_windowFramebufferSize.y * g_windowSettings.ResolutionScale
        };
        g_sceneViewerScaledSize = glm::ivec2{
            g_sceneViewerSize.x * g_windowSettings.ResolutionScale,
            g_sceneViewerSize.y * g_windowSettings.ResolutionScale
        };

        if (g_isEditor) {
            g_scaledFramebufferSize = g_sceneViewerScaledSize;
        } else {
            g_scaledFramebufferSize = g_windowFramebufferScaledSize;
        }

        if (g_scaledFramebufferSize.x * g_scaledFramebufferSize.y > 0.0f) {

            DeleteRendererFramebuffers();
            CreateRendererFramebuffers(g_scaledFramebufferSize);

            g_windowFramebufferResized = false;
            g_sceneViewerResized = false;
        }
    }
}

auto inline RenderDepthPrePass(entt::registry& registry) -> void {

    PROFILER_ZONESCOPEDN("All Depth PrePass Geometry");
    PushDebugGroup("Depth PrePass");
    BindFramebuffer(g_depthPrePass.Framebuffer);
    {
        g_depthPrePass.Pipeline.Bind();
        g_depthPrePass.Pipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);

        const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentTransform>();
        renderablesView.each([&](
            const entt::entity& entity,
            const auto& meshComponent,
            const auto& transformComponent) {

            PROFILER_ZONESCOPEDN("Draw PrePass Geometry");

            const auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

            g_depthPrePass.Pipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
            g_depthPrePass.Pipeline.SetUniform(0, transformComponent);

            g_depthPrePass.Pipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
        });
    }
    PopDebugGroup();
}

auto inline RenderShadows(const entt::registry& registry) -> void {

    PROFILER_ZONESCOPEDN("Draw Shadows");

    PushDebugGroup("Shadow Pass");

    g_shadowPass.Pipeline.Bind();
    g_shadowPass.Pipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);
    {
        for (auto framebufferIndex = 0; framebufferIndex < MAX_GLOBAL_LIGHTS * MAX_SHADOW_CASCADES; ++framebufferIndex) {
            const auto globalLightIndex = framebufferIndex / MAX_SHADOW_CASCADES;
            const auto cascadeIndex = framebufferIndex % MAX_SHADOW_CASCADES;

            const auto& globalLight = g_gpuGlobalLights[globalLightIndex];
            if (globalLight.LightProperties.x == 0) {
                continue;
            }

            PushDebugGroup(std::format("Update Shadow Cascades - Global Light {} - Cascade {}", globalLightIndex, cascadeIndex).c_str());

            BindFramebuffer(g_shadowPass.ShadowMapFramebuffers[framebufferIndex]);

            const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentRenderTransform>();
            renderablesView.each([&](
                const entt::entity& entity,
                const auto& meshComponent,
                const auto& transformComponent) {

                PROFILER_ZONESCOPEDN("Draw Shadow Cascade");

                auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                g_shadowPass.Pipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                g_shadowPass.Pipeline.SetUniform(0, globalLight.LightViewProjectionMatrix[cascadeIndex]);
                g_shadowPass.Pipeline.SetUniform(4, transformComponent);

                g_shadowPass.Pipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
            });

            PopDebugGroup();
        }
    }

    PopDebugGroup();
}

auto inline RenderGeometryPass(const entt::registry& registry) -> void {
    PROFILER_ZONESCOPEDN("Draw Geometry All");
    PushDebugGroup("Geometry Pass");
    BindFramebuffer(g_geometryPass.Framebuffer);
    {
        g_geometryPass.Pipeline.Bind();
        g_geometryPass.Pipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);

        const auto& renderablesView = registry.view<TComponentGpuMesh, TComponentGpuMaterial, TComponentRenderTransform>();
        renderablesView.each([&](
            const entt::entity& entity,
            const auto& meshComponent,
            const auto& materialComponent,
            const auto& transformComponent) {

            PROFILER_ZONESCOPEDN("Draw Geometry");

            auto& cpuMaterial = GetCpuMaterial(materialComponent.GpuMaterial);
            auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

            auto objectProperties = glm::ivec4{0, 0, 0, 0};
            g_geometryPass.Pipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
            g_geometryPass.Pipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexNormalUvTangentBuffer, 2);
            g_geometryPass.Pipeline.SetUniform(0, transformComponent);
            g_geometryPass.Pipeline.SetUniform(4, objectProperties);
            g_geometryPass.Pipeline.SetUniform(5, cpuMaterial.NormalStrengthRoughnessMetalnessEmissiveStrength);
            g_geometryPass.Pipeline.SetUniform(6, cpuMaterial.EmissiveColor);
            g_geometryPass.Pipeline.SetUniform(7, cpuMaterial.TextureFlags);

            if (cpuMaterial.BaseColorTexture.Id != 0) {
                g_geometryPass.Pipeline.BindTextureAndSampler(8, cpuMaterial.BaseColorTexture.Id, *cpuMaterial.BaseColorTexture.SamplerId);
            }
            if (cpuMaterial.NormalTexture.Id != 0) {
                g_geometryPass.Pipeline.BindTextureAndSampler(9, cpuMaterial.NormalTexture.Id, *cpuMaterial.NormalTexture.SamplerId);
            }
            if (cpuMaterial.ArmTexture.Id != 0) {
                g_geometryPass.Pipeline.BindTextureAndSampler(10, cpuMaterial.ArmTexture.Id, *cpuMaterial.ArmTexture.SamplerId);
            } else if (cpuMaterial.MetallicRoughnessTexture.Id != 0) {
                g_geometryPass.Pipeline.BindTextureAndSampler(10, cpuMaterial.MetallicRoughnessTexture.Id, *cpuMaterial.MetallicRoughnessTexture.SamplerId);
            } else {
                g_geometryPass.Pipeline.BindTextureAndSampler(10, 0, 0);
            }
            if (cpuMaterial.EmissiveTexture.Id != 0) {
                g_geometryPass.Pipeline.BindTextureAndSampler(11, cpuMaterial.EmissiveTexture.Id, *cpuMaterial.EmissiveTexture.SamplerId);
            } else {
                g_geometryPass.Pipeline.BindTextureAndSampler(11, 0, 0);
            }

            g_geometryPass.Pipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
        });
    }
    PopDebugGroup();
}

auto inline RenderGlobalLightPass() -> void {
    PROFILER_ZONESCOPEDN("Draw Global Lights");
    PushDebugGroup("GlobalLight Pass");
    BindFramebuffer(g_globalLightPass.Framebuffer);
    {
        const auto& sampler = GetSampler(g_fstSamplerNearestClampToEdge);
        const auto& shadowSampler = GetSampler(g_shadowSampler);

        g_globalLightPass.Pipeline.Bind();
        g_globalLightPass.Pipeline.BindBufferAsShaderStorageBuffer(g_gpuGlobalLightsBuffer, 1);
        g_globalLightPass.Pipeline.BindTexture(0, g_depthPrePass.Framebuffer.DepthStencilAttachment->Texture.Id);
        g_globalLightPass.Pipeline.BindTexture(1, g_geometryPass.Framebuffer.ColorAttachments[0]->Texture.Id);
        g_globalLightPass.Pipeline.BindTexture(2, g_geometryPass.Framebuffer.ColorAttachments[1]->Texture.Id);
        g_globalLightPass.Pipeline.BindTexture(3, g_geometryPass.Framebuffer.ColorAttachments[2]->Texture.Id);
        g_globalLightPass.Pipeline.BindTexture(4, g_geometryPass.Framebuffer.ColorAttachments[3]->Texture.Id);

        const auto& shadowMap = GetTexture(g_shadowPass.ShadowMapTexture);

        g_globalLightPass.Pipeline.BindTextureAndSampler(8, g_environmentMaps.EnvironmentMap, sampler.Id);
        g_globalLightPass.Pipeline.BindTextureAndSampler(9, g_environmentMaps.IrradianceMap, sampler.Id);
        g_globalLightPass.Pipeline.BindTextureAndSampler(10, g_environmentMaps.PrefilteredRadianceMap, sampler.Id);
        g_globalLightPass.Pipeline.BindTextureAndSampler(11, g_environmentMaps.BrdfIntegrationMap, sampler.Id);
        g_globalLightPass.Pipeline.BindTextureAndSampler(12, shadowMap.Id, shadowSampler.Id);

        g_globalLightPass.Pipeline.SetUniform(0, glm::inverse(g_globalUniforms.ProjectionMatrix * g_globalUniforms.ViewMatrix));
        g_globalLightPass.Pipeline.SetUniform(4, g_globalUniforms.CameraPosition);
        g_globalLightPass.Pipeline.SetUniform(5, g_scaledFramebufferSize);

        for (auto globalLightIndex = 0; globalLightIndex < MAX_GLOBAL_LIGHTS; ++globalLightIndex) {
            const auto& globalLight = g_gpuGlobalLights[globalLightIndex];
            if (globalLight.LightProperties.x == 0) {
                continue;
            }

            g_globalLightPass.Pipeline.SetUniform(6, globalLightIndex);
            g_globalLightPass.Pipeline.DrawArrays(0, 3);
        }
    }
    PopDebugGroup();
}

auto inline RenderComposePass() -> void {
    PROFILER_ZONESCOPEDN("Draw ComposeGeometry");
    PushDebugGroup("ComposeGeometry");
    BindFramebuffer(g_composePass.Framebuffer);
    {
        const auto& sampler = GetSampler(g_fstSamplerNearestClampToEdge);

        g_composePass.Pipeline.Bind();
        g_composePass.Pipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);
        g_composePass.Pipeline.BindTexture(0, g_depthPrePass.Framebuffer.DepthStencilAttachment->Texture.Id);
        g_composePass.Pipeline.BindTexture(1, g_geometryPass.Framebuffer.ColorAttachments[0]->Texture.Id);
        g_composePass.Pipeline.BindTexture(2, g_geometryPass.Framebuffer.ColorAttachments[1]->Texture.Id);
        g_composePass.Pipeline.BindTexture(3, g_geometryPass.Framebuffer.ColorAttachments[2]->Texture.Id);
        g_composePass.Pipeline.BindTexture(4, g_geometryPass.Framebuffer.ColorAttachments[3]->Texture.Id);
        g_composePass.Pipeline.BindTexture(5, g_globalLightPass.Framebuffer.ColorAttachments[0]->Texture.Id);

        g_composePass.Pipeline.BindTextureAndSampler(8, g_environmentMaps.EnvironmentMap, sampler.Id);
        g_composePass.Pipeline.BindTextureAndSampler(9, g_environmentMaps.IrradianceMap, sampler.Id);
        g_composePass.Pipeline.BindTextureAndSampler(10, g_environmentMaps.PrefilteredRadianceMap, sampler.Id);
        g_composePass.Pipeline.BindTextureAndSampler(11, g_environmentMaps.BrdfIntegrationMap, sampler.Id);

        g_composePass.Pipeline.SetUniform(0, glm::inverse(g_globalUniforms.ProjectionMatrix * g_globalUniforms.ViewMatrix));
        g_composePass.Pipeline.SetUniform(4, g_globalUniforms.CameraPosition);
        g_composePass.Pipeline.SetUniform(5, g_scaledFramebufferSize);

        g_composePass.Pipeline.DrawArrays(0, 3);
    }
    PopDebugGroup();
}

auto inline RenderDebugLines() -> void {

    if (g_debugLinesPass.IsEnabled && !g_debugLinesPass.DebugLines.empty()) {

        PROFILER_ZONESCOPEDN("Draw DebugLines");

        PushDebugGroup("Debug Lines");
        glDisable(GL_CULL_FACE);

        UpdateBuffer(g_debugLinesPass.VertexBuffer, 0, sizeof(TGpuDebugLine) * g_debugLinesPass.DebugLines.size(), g_debugLinesPass.DebugLines.data());

        g_debugLinesPass.Pipeline.Bind();
        g_debugLinesPass.Pipeline.BindBufferAsVertexBuffer(g_debugLinesPass.VertexBuffer, 0, 0, sizeof(TGpuDebugLine) / 2);
        g_debugLinesPass.Pipeline.BindBufferAsUniformBuffer(g_globalUniformsBuffer, 0);
        g_debugLinesPass.Pipeline.DrawArrays(0, g_debugLinesPass.DebugLines.size() * 2);

        glEnable(GL_CULL_FACE);
        PopDebugGroup();
    }
}

auto inline RenderFxaaPass() -> void {
    if (g_fxaaPass.IsEnabled) {
        PROFILER_ZONESCOPEDN("PostFX FXAA");
        PushDebugGroup("PostFX FXAA");
        BindFramebuffer(g_fxaaPass.Framebuffer);
        {
            g_fxaaPass.Pipeline.Bind();
            g_fxaaPass.Pipeline.BindTexture(0, g_composePass.Framebuffer.ColorAttachments[0]->Texture.Id);
            g_fxaaPass.Pipeline.SetUniform(1, 1);
            g_fxaaPass.Pipeline.DrawArrays(0, 3);
        }
        PopDebugGroup();
    }
}

auto inline RenderUi(Renderer::TRenderContext& renderContext, entt::registry& registry) -> void {
    PushDebugGroup("UI");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (g_isEditor) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glBlitNamedFramebuffer(g_fxaaPass.IsEnabled
                                   ? g_fxaaPass.Framebuffer.Id
                                   : g_composePass.Framebuffer.Id,
                               0,
                               0, 0, static_cast<int32_t>(g_scaledFramebufferSize.x), static_cast<int32_t>(g_scaledFramebufferSize.y),
                               0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y,
                               GL_COLOR_BUFFER_BIT,
                               GL_NEAREST);
    }

    UiRender(renderContext, registry);
    PopDebugGroup();
}

auto Renderer::Render(
    TRenderContext& renderContext,
    entt::registry& registry) -> void {

    if (g_playerEntity == entt::null) {
        g_playerEntity = registry.view<TComponentCamera>().front();
    }

    ResetDebugLines();

    CreateGpuResourcesIfNecessary(registry);

    UpdateGlobalLights(registry);

    UpdateAllTransforms(registry);
    UpdateGlobalTransforms(registry);

    ResizeFramebuffersIfNecessary();

    RenderDepthPrePass(registry);
    RenderShadows(registry);
    RenderGeometryPass(registry);
    RenderGlobalLightPass();
    RenderComposePass();
    RenderDebugLines();

    RenderFxaaPass();

    RenderUi(renderContext, registry);
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
    //UiSetStyleDarkWithFuchsia();
    UiSetStylePurple();
    //UiSetStyleNeon();

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

            ImGui::Text("rpms: %.0f Hz", renderContext.FramesPerSecond * 60.0f);
            ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * renderContext.FramesPerSecond);
            ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * renderContext.FramesPerSecond));

            ImGui::TextColored(ImColor::HSV(0.14f, 1.0f, 1.0f), " mft: %.2f ms", renderContext.AverageFramesPerSecond);
            ImGui::Text("  ft: %.2f ms", renderContext.DeltaTimeInSeconds * 1000.0f);
            ImGui::TextColored(ImColor::HSV(0.14f, 1.0f, 1.0f), "rfps: %.0f Hz", renderContext.FramesPerSecond);
            ImGui::TextColored(ImColor::HSV(0.16f, 1.0f, 1.0f), "      %.0f Hz (1%%)", renderContext.FramesPerSecond1P);
            ImGui::TextColored(ImColor::HSV(0.18f, 1.0f, 1.0f), "      %.0f Hz (0.1%%)", renderContext.FramesPerSecond01P);
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
                ImGui::RadioButton("GBuffer-Emissive", &g_sceneViewerTextureIndex, 5);
                if (g_fxaaPass.IsEnabled) {
                    ImGui::RadioButton("FXAA", &g_sceneViewerTextureIndex, 6);
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
            const auto currentSceneWindowPosition = ImGui::GetCursorScreenPos();
            if (currentSceneWindowSize.x != g_sceneViewerSize.x || currentSceneWindowSize.y != g_sceneViewerSize.y) {
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

            auto GetCurrentSceneViewerTexture = [&](const int32_t sceneViewerTextureIndex) -> uint32_t {
                switch (sceneViewerTextureIndex) {
                    case 0: return g_composePass.Framebuffer.ColorAttachments[0]->Texture.Id;
                    case 1: return g_depthPrePass.Framebuffer.DepthStencilAttachment->Texture.Id;
                    case 2: return g_geometryPass.Framebuffer.ColorAttachments[0]->Texture.Id;
                    case 3: return g_geometryPass.Framebuffer.ColorAttachments[1]->Texture.Id;
                    case 4: return g_geometryPass.Framebuffer.ColorAttachments[2]->Texture.Id;
                    case 5: return g_geometryPass.Framebuffer.ColorAttachments[3]->Texture.Id;
                    case 6: return g_fxaaPass.Framebuffer.ColorAttachments[0]->Texture.Id;
                    default: std::unreachable();
                }

                std::unreachable();
            };

            const auto texture = GetCurrentSceneViewerTexture(g_sceneViewerTextureIndex);
            ImGui::Image(texture, currentSceneWindowSize, g_uiUnitY, g_uiUnitX);

            if (g_selectedEntity != entt::null) {

                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(currentSceneWindowPosition.x, currentSceneWindowPosition.y, currentSceneWindowSize.x, currentSceneWindowSize.y);

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
        ImGui::PopStyleVar();
        ImGui::End();

        if (ImGui::Begin((char*)ICON_MDI_SETTINGS " Settings")) {
            if (ImGui::SliderFloat("Resolution Scale", &g_windowSettings.ResolutionScale, 0.05f, 4.0f, "%.2f")) {

                g_sceneViewerResized = g_isEditor;
                g_windowFramebufferResized = !g_isEditor;
            }
            ImGui::Checkbox("Enable FXAA", &g_fxaaPass.IsEnabled);
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
    const Renderer::TRenderContext& renderContext,
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

        const auto& magnifierOutput = g_composePass.Framebuffer.ColorAttachments[0]->Texture.Id;

        ImGui::Image(magnifierOutput,
                     magnifierSize,
                     ImVec2(uv0.x, uv0.y),
                     ImVec2(uv1.x, uv1.y));
        }
    ImGui::End();
}

auto UiSetStyleDark() -> void {

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

auto UiSetStyleValve() -> void {

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

auto UiSetStyleDarkWithFuchsia() -> void {

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

auto UiSetStylePurple() -> void {

    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();

    // Style adjustments
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.ChildRounding = 4.0f;

    style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.IndentSpacing = 22.0f;

    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;

     ImVec4* colors = style.Colors;

     constexpr glm::vec3 kBaseAltColor{ 0.701f, 0.701f, 1.0f };
     constexpr glm::vec3 kMainAltColor = kBaseAltColor - kBaseAltColor * 0.1f;
     constexpr glm::vec3 kAltHover = kMainAltColor + kMainAltColor * 0.1f;
     constexpr glm::vec3 kAltActive = kAltHover + kAltHover * 0.1f;

     constexpr glm::vec3 kBasePurple{ 0.462f, 0.427f, 0.858f };
     constexpr glm::vec3 kMainPurple = kBasePurple;
     constexpr glm::vec3 kDarkerPurple = kMainPurple - kMainPurple * 0.2f;
     constexpr glm::vec3 kDarkestPurple = kMainPurple - kMainPurple * 0.6f;
     constexpr glm::vec3 kPurpleHover = kMainPurple + kMainPurple * 0.1f;
     constexpr glm::vec3 kPurpleActive = kPurpleHover + kPurpleHover * 0.1f;

     constexpr glm::vec3 kBaseWindowBkg = { 0.086f, 0.129f, 0.196f };
     constexpr glm::vec3 kWindowBkg = kBaseWindowBkg - kBaseWindowBkg * 0.1f;
     constexpr glm::vec3 kWindowBarBkg = kBaseWindowBkg - kBaseWindowBkg * 0.2f;
     constexpr glm::vec3 kMenuBarBkg = kBaseWindowBkg - kBaseWindowBkg * 0.3f;
     constexpr glm::vec3 kChildBkg = kWindowBkg - kWindowBkg * 0.1f;

     ImVec4 mainAlt = ImVec4(kMainAltColor.x, kMainAltColor.y, kMainAltColor.z, 1.0f);
     ImVec4 altActive = ImVec4(kAltActive.x, kAltActive.y, kAltActive.z, 1.0f);
     ImVec4 altHover = ImVec4(kAltHover.x, kAltHover.y, kAltHover.z, 1.0f);

     ImVec4 mainPurple = ImVec4(kMainPurple.x, kMainPurple.y, kMainPurple.z, 1.0f);
     ImVec4 darkerPurple = ImVec4(kDarkerPurple.x, kDarkerPurple.y, kDarkerPurple.z, 1.0f);
     ImVec4 darkestPurple = ImVec4(kDarkestPurple.x, kDarkestPurple.y, kDarkestPurple.z, 1.0f);
     ImVec4 purpleActive = ImVec4(kPurpleActive.x, kPurpleActive.y, kPurpleActive.z, 1.0f);
     ImVec4 purpleHover = ImVec4(kPurpleHover.x, kPurpleHover.y, kPurpleHover.z, 1.0f);

     ImVec4 windowBkg = ImVec4(kWindowBkg.x, kWindowBkg.y, kWindowBkg.z, 1.00f);
     ImVec4 childBkg = ImVec4(kChildBkg.x, kChildBkg.y, kChildBkg.z, 1.00f);
     ImVec4 windowBarBkg = ImVec4(kWindowBarBkg.x, kWindowBarBkg.y, kWindowBarBkg.z, 1.00f);
     ImVec4 activeWindowBarBkg = ImVec4(kBaseWindowBkg.x, kBaseWindowBkg.y, kBaseWindowBkg.z, 1.0f);
     ImVec4 menuBarBkg = ImVec4(kMenuBarBkg.x, kMenuBarBkg.y, kMenuBarBkg.z, 1.0f);

     // Base colors inspired by Catppuccin Mocha
     colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
     colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.56f, 0.52f, 1.00f);
     colors[ImGuiCol_WindowBg] = windowBkg;
     colors[ImGuiCol_ChildBg] = childBkg;
     colors[ImGuiCol_PopupBg] = windowBkg;
     colors[ImGuiCol_Border] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);
     colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
     colors[ImGuiCol_FrameBg] = darkestPurple;
     colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.20f, 0.29f, 1.00f);
     colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.22f, 0.31f, 1.00f);
     colors[ImGuiCol_TitleBg] = windowBarBkg;
     colors[ImGuiCol_TitleBgActive] = activeWindowBarBkg;
     colors[ImGuiCol_TitleBgCollapsed] = childBkg;
     colors[ImGuiCol_MenuBarBg] = menuBarBkg;
     colors[ImGuiCol_ScrollbarBg] = darkestPurple;
     colors[ImGuiCol_ScrollbarGrab] = darkerPurple;
     colors[ImGuiCol_ScrollbarGrabHovered] = mainPurple;
     colors[ImGuiCol_ScrollbarGrabActive] = purpleHover;
     colors[ImGuiCol_CheckMark] = purpleActive;
     colors[ImGuiCol_SliderGrab] = purpleHover;
     colors[ImGuiCol_SliderGrabActive] = purpleActive;
     colors[ImGuiCol_Button] = darkerPurple;
     colors[ImGuiCol_ButtonHovered] = mainPurple;
     colors[ImGuiCol_ButtonActive] = purpleActive;
     colors[ImGuiCol_Header] = darkerPurple;
     colors[ImGuiCol_HeaderHovered] = mainPurple;
     colors[ImGuiCol_HeaderActive] = purpleHover;
     colors[ImGuiCol_Separator] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);
     colors[ImGuiCol_SeparatorHovered] = ImVec4(0.95f, 0.66f, 0.47f, 1.00f);
     colors[ImGuiCol_SeparatorActive] = ImVec4(0.95f, 0.66f, 0.47f, 1.00f);
     colors[ImGuiCol_ResizeGrip] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);
     colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f);
     colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.61f, 0.85f, 1.00f);
     colors[ImGuiCol_Tab] = ImVec4(0.21f, 0.18f, 0.25f, 1.00f);
     colors[ImGuiCol_TabHovered] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);
     colors[ImGuiCol_TabActive] = ImVec4(0.76f, 0.46f, 0.58f, 1.00f);
     colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.16f, 0.22f, 1.00f);
     colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.21f, 0.18f, 0.25f, 1.00f);
     colors[ImGuiCol_PlotLines] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);
     colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f);
     colors[ImGuiCol_PlotHistogram] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);
     colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f);
     colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
     colors[ImGuiCol_TableBorderStrong] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);
     colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
     colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
     colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
     colors[ImGuiCol_TextSelectedBg] = ImVec4(0.82f, 0.61f, 0.85f, 0.35f);
     colors[ImGuiCol_DragDropTarget] = ImVec4(0.95f, 0.66f, 0.47f, 0.90f);
     colors[ImGuiCol_NavHighlight] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);
     colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
     colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
     colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

auto UiSetStyleNeon() -> void {

    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.ScrollbarSize = 15.0f;
    style.GrabMinSize = 17.0f;

    // Rounding
    style.WindowRounding = 0.0f;
    style.ChildRounding = 7.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 7.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 7.0f;

    // Widgets
    style.WindowMenuButtonPosition = ImGuiDir::ImGuiDir_None;
    style.SeparatorTextBorderSize = 3.0f;

    // Docking
    style.DockingSeparatorSize = 2.0f;

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.19f, 0.94f); // #171730
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.19f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.22f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(1.00f, 0.00f, 0.29f, 0.50f); // #ff0049
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.00f, 0.69f, 0.54f); // #cc00af
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 0.00f, 0.29f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.93f, 1.00f, 0.67f); // #00eeff
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.80f, 0.00f, 0.69f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.09f, 0.19f, 0.51f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.93f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.93f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.00f, 0.29f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.80f, 0.00f, 0.69f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.00f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.93f, 1.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.80f, 0.00f, 0.69f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.00f, 0.29f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.93f, 1.00f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];

    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);

    colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.93f, 1.00f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.80f, 0.00f, 0.69f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.00f, 0.29f, 0.95f);
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive] = ImVec4(0.80f, 0.00f, 0.69f, 1.00f);

    colors[ImGuiCol_TabSelected] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected] = ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_DockingPreview].w *= 0.7f;
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];

    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.80f, 0.00f, 0.69f, 0.35f);

    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);

    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
