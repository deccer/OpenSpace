#define GLM_FORCE_CXX2A
#define GLM_FORCE_AVX2
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>
#include <stack>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_GLSL
#include <stb_include.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <debugbreak.h>
#include <spdlog/spdlog.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <tracy/TracyOpenGL.hpp>

#include <entt/entt.hpp>

// - Core Types ---------------------------------------------------------------

template <auto TDeleterCallback>
struct SUniqueDeleter {
    template <typename T>
    constexpr void operator()(T* t) const {
        callback(t);
    }
};

template <typename T, auto callback>
using SDeletableUniquePtr = std::unique_ptr<T, SUniqueDeleter<TDeleterCallback>>;

template<class TTag>
struct SIdImpl {
    enum class Id : std::size_t { Invalid = SIZE_MAX };

    bool operator==(const SIdImpl&) const noexcept = default;
};

template<class TTag>
using SId = typename SIdImpl<TTag>::Id;

template<class TTag>
struct SIdGenerator {
public:
    auto Next() -> SId<TTag> {
        _counter += 1;
        return { _counter };
    }
private:
    size_t _counter = 0;
};

constexpr auto HashString(std::string_view str) {
    auto hash = 2166136261u;
    for (auto ch: str) {
        hash ^= ch;
        hash *= 16777619u;
    }
    return hash;
}

constexpr uint32_t operator"" _hash(const char* str, size_t) {
    return HashString(str);
}

struct SExtent2D
{
    uint32_t Width{};
    uint32_t Height{};

    bool operator==(const SExtent2D&) const noexcept = default;
    constexpr SExtent2D operator+(const SExtent2D& other) const { return { Width + other.Width, Height + other.Height }; }
    constexpr SExtent2D operator-(const SExtent2D& other) const { return { Width - other.Width, Height - other.Height }; }
    constexpr SExtent2D operator*(const SExtent2D& other) const { return { Width * other.Width, Height * other.Height }; }
    constexpr SExtent2D operator/(const SExtent2D& other) const { return { Width / other.Width, Height / other.Height }; }
    constexpr SExtent2D operator>>(const SExtent2D& other) const { return { Width >> other.Width, Height >> other.Height }; }
    constexpr SExtent2D operator<<(const SExtent2D& other) const { return { Width << other.Width, Height << other.Height }; }
    constexpr SExtent2D operator+(uint32_t value) const { return *this + SExtent2D{ value, value }; }
    constexpr SExtent2D operator-(uint32_t value) const { return *this - SExtent2D{ value, value }; }
    constexpr SExtent2D operator*(uint32_t value) const { return *this * SExtent2D{ value, value }; }
    constexpr SExtent2D operator/(uint32_t value) const { return *this / SExtent2D{ value, value }; }
    constexpr SExtent2D operator>>(uint32_t value) const { return *this >> SExtent2D{ value, value }; }
    constexpr SExtent2D operator<<(uint32_t value) const { return *this << SExtent2D{ value, value }; }
};

constexpr SExtent2D operator+(uint32_t value, SExtent2D extent) {   
    return extent + value;
}

constexpr SExtent2D operator-(uint32_t value, SExtent2D extent) {
    return extent - value;
}

constexpr SExtent2D operator*(uint32_t value, SExtent2D extent) {
    return extent * value;
}

constexpr SExtent2D operator/(uint32_t value, SExtent2D extent) {
    return extent / value;
}

constexpr SExtent2D operator>>(uint32_t value, SExtent2D extent) {
    return extent >> value;
}

constexpr SExtent2D operator<<(uint32_t value, SExtent2D extent) {
    return extent << value;
}

struct SExtent3D
{
    uint32_t Width{};
    uint32_t Height{};
    uint32_t Depth{};

    operator SExtent2D() const { return { Width, Height }; }
    bool operator==(const SExtent3D&) const noexcept = default;
    constexpr SExtent3D operator+(const SExtent3D& other) const { return { Width + other.Width, Height + other.Height, Depth + other.Depth }; }
    constexpr SExtent3D operator-(const SExtent3D& other) const { return { Width - other.Width, Height - other.Height, Depth - other.Depth }; }
    constexpr SExtent3D operator*(const SExtent3D& other) const { return { Width * other.Width, Height * other.Height, Depth * other.Depth }; }
    constexpr SExtent3D operator/(const SExtent3D& other) const { return { Width / other.Width, Height / other.Height, Depth / other.Depth }; }
    constexpr SExtent3D operator>>(const SExtent3D& other) const { return { Width >> other.Width, Height >> other.Height, Depth >> other.Depth }; }
    constexpr SExtent3D operator<<(const SExtent3D& other) const { return { Width << other.Width, Height << other.Height, Depth << other.Depth }; }
    constexpr SExtent3D operator+(uint32_t value) const { return *this + SExtent3D{ value, value, value }; }
    constexpr SExtent3D operator-(uint32_t value) const { return *this - SExtent3D{ value, value, value }; }
    constexpr SExtent3D operator*(uint32_t value) const { return *this * SExtent3D{ value, value, value }; }
    constexpr SExtent3D operator/(uint32_t value) const { return *this / SExtent3D{ value, value, value }; }
    constexpr SExtent3D operator>>(uint32_t value) const { return *this >> SExtent3D{ value, value, value }; }
    constexpr SExtent3D operator<<(uint32_t value) const { return *this << SExtent3D{ value, value, value }; }
};

constexpr SExtent3D operator+(uint32_t value, SExtent3D extent) {
    return extent + value;
}

constexpr SExtent3D operator-(uint32_t value, SExtent3D extent) {
    return extent - value;
}

constexpr SExtent3D operator*(uint32_t value, SExtent3D extent) {
    return extent * value;
}

constexpr SExtent3D operator/(uint32_t value, SExtent3D extent) {
    return extent / value;
}

constexpr SExtent3D operator>>(uint32_t value, SExtent3D extent) {
    return extent >> value;
}

constexpr SExtent3D operator<<(uint32_t value, SExtent3D extent) {
    return extent << value;
}

struct SOffset2D {

    uint32_t X{};
    uint32_t Y{};

    bool operator==(const SOffset2D&) const noexcept = default;
    constexpr SOffset2D operator+(const SOffset2D & other) const { return { X + other.X, Y + other.Y }; }
    constexpr SOffset2D operator-(const SOffset2D & other) const { return { X - other.X, Y - other.Y }; }
    constexpr SOffset2D operator*(const SOffset2D & other) const { return { X * other.X, Y * other.Y }; }
    constexpr SOffset2D operator/(const SOffset2D & other) const { return { X / other.X, Y / other.Y }; }
    constexpr SOffset2D operator>>(const SOffset2D & other) const { return { X >> other.X, Y >> other.Y }; }
    constexpr SOffset2D operator<<(const SOffset2D & other) const { return { X << other.X, Y << other.Y }; }
    constexpr SOffset2D operator+(uint32_t value) const { return *this + SOffset2D{ value, value }; }
    constexpr SOffset2D operator-(uint32_t value) const { return *this - SOffset2D{ value, value }; }
    constexpr SOffset2D operator*(uint32_t value) const { return *this * SOffset2D{ value, value }; }
    constexpr SOffset2D operator/(uint32_t value) const { return *this / SOffset2D{ value, value }; }
    constexpr SOffset2D operator>>(uint32_t value) const { return *this >> SOffset2D{ value, value }; }
    constexpr SOffset2D operator<<(uint32_t value) const { return *this << SOffset2D{ value, value }; }
};

constexpr SOffset2D operator+(uint32_t value, SOffset2D offset) {
    return offset + value;
}

constexpr SOffset2D operator-(uint32_t value, SOffset2D offset) {
    return offset - value;
}

constexpr SOffset2D operator*(uint32_t value, SOffset2D offset) {
    return offset * value;
}

constexpr SOffset2D operator/(uint32_t value, SOffset2D offset) {
    return offset / value;
}

constexpr SOffset2D operator>>(uint32_t value, SOffset2D offset) {
    return offset >> value;
}

constexpr SOffset2D operator<<(uint32_t value, SOffset2D offset) {
    return offset << value;
}

struct SOffset3D {

    uint32_t X{};
    uint32_t Y{};
    uint32_t Z{};

    operator SOffset2D() const { return { X, Y }; }
    bool operator==(const SOffset3D&) const noexcept = default;
    constexpr SOffset3D operator+(const SOffset3D& other) const { return { X + other.X, Y + other.Y, Z + other.Z }; }
    constexpr SOffset3D operator-(const SOffset3D& other) const { return { X - other.X, Y - other.Y, Z - other.Z }; }
    constexpr SOffset3D operator*(const SOffset3D& other) const { return { X * other.X, Y * other.Y, Z * other.Z }; }
    constexpr SOffset3D operator/(const SOffset3D& other) const { return { X / other.X, Y / other.Y, Z / other.Z }; }
    constexpr SOffset3D operator>>(const SOffset3D& other) const { return { X >> other.X, Y >> other.Y, Z >> other.Z }; }
    constexpr SOffset3D operator<<(const SOffset3D& other) const { return { X << other.X, Y << other.Y, Z << other.Z }; }
    constexpr SOffset3D operator+(uint32_t value) const { return *this + SOffset3D{ value, value, value }; }
    constexpr SOffset3D operator-(uint32_t value) const { return *this - SOffset3D{ value, value, value }; }
    constexpr SOffset3D operator*(uint32_t value) const { return *this * SOffset3D{ value, value, value }; }
    constexpr SOffset3D operator/(uint32_t value) const { return *this / SOffset3D{ value, value, value }; }
    constexpr SOffset3D operator>>(uint32_t value) const { return *this >> SOffset3D{ value, value, value }; }
    constexpr SOffset3D operator<<(uint32_t value) const { return *this << SOffset3D{ value, value, value }; }
};

constexpr SOffset3D operator+(uint32_t value, SOffset3D offset) {
    return offset + value;
}

constexpr SOffset3D operator-(uint32_t value, SOffset3D offset) { 
    return offset - value;
}

constexpr SOffset3D operator*(uint32_t value, SOffset3D offset) { 
    return offset * value;
}

constexpr SOffset3D operator/(uint32_t value, SOffset3D offset) { 
    return offset / value;
}

constexpr SOffset3D operator>>(uint32_t value, SOffset3D offset) {
    return offset >> value;
}

constexpr SOffset3D operator<<(uint32_t value, SOffset3D offset) {
    return offset << value;
}

enum class EWindowStyle {
    Windowed,
    Fullscreen,
    FullscreenExclusive
};

struct SWindowSettings {
    int32_t ResolutionWidth;
    int32_t ResolutionHeight;
    float ResolutionScale;
    EWindowStyle WindowStyle;
    bool IsDebug;
    bool IsVSyncEnabled;
};

// - RHI ----------------------------------------------------------------------

enum class EFormat : uint32_t {

    Undefined,

    // Color formats
    R8_UNORM,
    R8_SNORM,
    R16_UNORM,
    R16_SNORM,
    R8G8_UNORM,
    R8G8_SNORM,
    R16G16_UNORM,
    R16G16_SNORM,
    R3G3B2_UNORM,
    R4G4B4_UNORM,
    R5G5B5_UNORM,
    R8G8B8_UNORM,
    R8G8B8_SNORM,
    R10G10B10_UNORM,
    R12G12B12_UNORM,
    R16G16B16_SNORM,
    R2G2B2A2_UNORM,
    R4G4B4A4_UNORM,
    R5G5B5A1_UNORM,
    R8G8B8A8_UNORM,
    R8G8B8A8_SNORM,
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R12G12B12A12_UNORM,
    R16G16B16A16_UNORM,
    R16G16B16A16_SNORM,
    R8G8B8_SRGB,
    R8G8B8A8_SRGB,
    R16_FLOAT,
    R16G16_FLOAT,
    R16G16B16_FLOAT,
    R16G16B16A16_FLOAT,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32_FLOAT,
    R32G32B32A32_FLOAT,
    R11G11B10_FLOAT,
    R9G9B9_E5,
    R8_SINT,
    R8_UINT,
    R16_SINT,
    R16_UINT,
    R32_SINT,
    R32_UINT,
    R8G8_SINT,
    R8G8_UINT,
    R16G16_SINT,
    R16G16_UINT,
    R32G32_SINT,
    R32G32_UINT,
    R8G8B8_SINT,
    R8G8B8_UINT,
    R16G16B16_SINT,
    R16G16B16_UINT,
    R32G32B32_SINT,
    R32G32B32_UINT,
    R8G8B8A8_SINT,
    R8G8B8A8_UINT,
    R16G16B16A16_SINT,
    R16G16B16A16_UINT,
    R32G32B32A32_SINT,
    R32G32B32A32_UINT,

    // Depth & stencil formats
    D32_FLOAT,
    D32_UNORM,
    D24_UNORM,
    D16_UNORM,
    D32_FLOAT_S8_UINT,
    D24_UNORM_S8_UINT,
    S8_UINT,

    // Compressed formats
    // DXT
    BC1_RGB_UNORM,
    BC1_RGB_SRGB,
    BC1_RGBA_UNORM,
    BC1_RGBA_SRGB,
    BC2_RGBA_UNORM,
    BC2_RGBA_SRGB,
    BC3_RGBA_UNORM,
    BC3_RGBA_SRGB,
    // RGTC
    BC4_R_UNORM,
    BC4_R_SNORM,
    BC5_RG_UNORM,
    BC5_RG_SNORM,
    // BPTC
    BC6H_RGB_UFLOAT,
    BC6H_RGB_SFLOAT,
    BC7_RGBA_UNORM,
    BC7_RGBA_SRGB
};

enum class EFormatClass {
    Float,
    Integer,
    Long
};

enum class EBaseTypeClass {
    Float,
    Integer,
    UnsignedInteger
};

enum class ETextureType : uint32_t {
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMultisample,
    Texture2DMultisampleArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
};

enum class ESampleCount : uint32_t {
    One = 1,
    Two = 2,
    Four = 4,
    Eight = 8,
    SixTeen = 16,
    ThirtyTwo = 32,
};

enum class EUploadFormat : uint32_t
{
    Undefined,
    Auto,
    R,
    Rg,
    Rgb,
    Bgr,
    Rgba,
    Bgra,
    RInteger,
    RgInteger,
    RgbInteger,
    BgrInteger,
    RgbaInteger,
    BgraInteger,
    Depth,
    StencilIndex,
    DepthStencilIndex,
};

enum class EUploadType : uint32_t
{
    Undefined,
    Auto,
    UnsignedByte,
    SignedByte,
    UnsignedShort,
    SignedShort,
    UnsignedInteger,
    SignedInteger,
    Float,
    UnsignedByte332,
    UnsignedByte233Reversed,
    UnsignedShort565,
    UnsignedShort565Reversed,
    UnsignedShort4444,
    UnsignedShort4444Reversed,
    UnsignedShort5551,
    UnsignedShort1555Reversed,
    UnsignedInteger8888,
    UnsignedInteger8888Reversed,
    UnsignedInteger1010102,
    UnsignedInteger2101010Reversed,
};

enum class EAttachmentType : uint32_t {
    ColorAttachment0 = 0u,
    ColorAttachment1,
    ColorAttachment2,
    ColorAttachment3,
    ColorAttachment4,
    ColorAttachment5,
    ColorAttachment6,
    ColorAttachment7,
    DepthAttachment,
    StencilAttachment
};

enum class ETextureAddressMode {
    Repeat,
    RepeatMirrored,
    ClampToEdge,
    ClampToBorder,
    ClampToEdgeMirrored,
};

enum class ETextureMinFilter {
    Nearest,
    NearestMipmapLinear,
    NearestMipmapNearest,
    Linear,
    LinearMipmapLinear,
    LinearMipmapNearest,
};

enum class ETextureMagFilter {
    Nearest,
    Linear
};

enum class EPrimitiveTopology {
    Triangles,
    TriangleStip,
    TriangleFan,
    Lines,
};

enum class EFramebufferAttachmentLoadOperation {
    Load,
    Clear,
    DontCare
};

using STextureId = SId<struct TTextureId>;
using SSamplerId = SId<struct TSamplerId>;
using SFramebufferId = SId<struct TFramebufferId>;
using SBufferId = SId<struct TBufferId>;
using SGraphicsPipelineId = SId<struct TGraphicsPipelineId>;
using SComputePipelineId = SId<struct TComputePipelineId>;

struct SCreateTextureDescriptor {
    ETextureType TextureType = {};
    EFormat Format = {};
    SExtent3D Extent = {};
    uint32_t MipMapLevels = 0;
    uint32_t Layers = 0;
    ESampleCount SampleCount = {};
    std::string Label = {};
};

struct SUploadTextureDescriptor {
    uint32_t Level;
    SOffset3D Offset;
    SExtent3D Extent;
    EUploadFormat UploadFormat = EUploadFormat::Auto;
    EUploadType UploadType = EUploadType::Auto;
    const void* PixelData = nullptr;
};

struct STexture {
    uint32_t Id;
    EFormat Format;
    SExtent3D Extent;
    ETextureType TextureType;
};

struct SFramebufferAttachmentClearColor {
    SFramebufferAttachmentClearColor() = default;

    template<typename... Args>
    requires (sizeof...(Args) <= 4)
    SFramebufferAttachmentClearColor(const Args&... args)
      : Data(std::array<std::common_type_t<std::remove_cvref_t<Args>...>, 4>{ args...})
    {
    }

    std::variant<std::array<float, 4>, std::array<uint32_t, 4>, std::array<int32_t, 4>> Data;
};

struct SFramebufferAttachmentClearDepthStencil {
    float Depth = {};
    int32_t Stencil = {};
};

struct SFramebufferAttachmendDescriptor {
    std::string_view Label;
    EFormat Format;    
    SExtent2D Extent;
    EFramebufferAttachmentLoadOperation LoadOperation;
};

struct SFramebufferColorAttachmentDescriptor {
    std::string_view Label;
    EFormat Format;    
    SExtent2D Extent;
    EFramebufferAttachmentLoadOperation LoadOperation;
    SFramebufferAttachmentClearColor ClearColor;
};

struct SFramebufferDepthStencilAttachmentDescriptor {
    std::string_view Label;
    EFormat Format;    
    SExtent2D Extent;
    EFramebufferAttachmentLoadOperation LoadOperation;
    SFramebufferAttachmentClearDepthStencil ClearDepthStencil;
};

struct SFramebufferDescriptor {
    std::string_view Label;
    std::array<std::optional<SFramebufferColorAttachmentDescriptor>, 8> ColorAttachments;
    std::optional<SFramebufferDepthStencilAttachmentDescriptor> DepthStencilAttachment;
};

struct SFramebufferColorAttachment {
    STexture Texture;
    SFramebufferAttachmentClearColor ClearColor;
    EFramebufferAttachmentLoadOperation LoadOperation;
};

struct SFramebufferDepthStencilAttachment {
    STexture Texture;
    SFramebufferAttachmentClearDepthStencil ClearDepthStencil;
    EFramebufferAttachmentLoadOperation LoadOperation;
};

struct SFramebuffer {
    uint32_t Id;
    std::array<std::optional<SFramebufferColorAttachment>, 8> ColorAttachments;
    std::optional<SFramebufferDepthStencilAttachment> DepthStencilAttachment;
};

struct SSamplerDescriptor {
    std::string_view Label;
    ETextureAddressMode AddressModeU = ETextureAddressMode::ClampToEdge;
    ETextureAddressMode AddressModeV = ETextureAddressMode::ClampToEdge;
    ETextureAddressMode AddressModeW = ETextureAddressMode::ClampToEdge;
    ETextureMagFilter MagFilter = ETextureMagFilter::Linear;
    ETextureMinFilter MinFilter = ETextureMinFilter::Linear;
    float LodBias = 0;
    float LodMin = -1000;
    float LodMax = 1000;

    bool operator==(const SSamplerDescriptor& rhs) const {
        return Label == rhs.Label &&
            AddressModeU == rhs.AddressModeU &&
            AddressModeV == rhs.AddressModeV &&
            AddressModeW == rhs.AddressModeW &&
            MagFilter == rhs.MagFilter &&
            MinFilter == rhs.MinFilter &&
            LodBias == rhs.LodBias &&
            LodMin == rhs.LodMin &&
            LodMax == rhs.LodMax;
    }
};

namespace std {
    template<>
    struct hash<SSamplerDescriptor> {
        size_t operator()(const SSamplerDescriptor& samplerDescriptor) const {
            size_t seed = 0;
            hash_combine(seed, samplerDescriptor.Label);
            hash_combine(seed, samplerDescriptor.AddressModeU);
            hash_combine(seed, samplerDescriptor.AddressModeV);
            hash_combine(seed, samplerDescriptor.AddressModeW);
            hash_combine(seed, samplerDescriptor.MagFilter);
            hash_combine(seed, samplerDescriptor.MinFilter);
            hash_combine(seed, samplerDescriptor.LodBias);
            hash_combine(seed, samplerDescriptor.LodMax);
            hash_combine(seed, samplerDescriptor.LodMin);
            return seed;
        }
    private:
        template<typename T>
        void hash_combine(size_t& seed, const T& v) const {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}

struct SSampler {
    uint32_t Id;
};

struct SInputAssemblyDescriptor {
    EPrimitiveTopology PrimitiveTopology = EPrimitiveTopology::Triangles;
    bool IsPrimitiveRestartEnabled = false;
};

struct SVertexInputAttributeDescriptor {
    uint32_t Location;
    uint32_t Binding;
    EFormat Format;
    uint32_t Offset;
};

struct SVertexInputDescriptor {
    std::array<std::optional<const SVertexInputAttributeDescriptor>, 8> VertexInputAttributes = {};
};

struct SGraphicsPipelineDescriptor {
    std::string_view Label;
    std::string_view VertexShaderFilePath;
    std::string_view FragmentShaderFilePath;

    SInputAssemblyDescriptor InputAssembly;
    std::optional<SVertexInputDescriptor> VertexInput;
};

struct SComputePipelineDescriptor {
    std::string_view Label;
    std::string_view ComputeShaderFilePath;
};

struct SPipeline {
    uint32_t Id;

    inline auto virtual Bind() -> void {
        glUseProgram(Id);
    }

    inline auto BindBufferAsUniformBuffer(uint32_t buffer, int32_t bindingIndex) -> void {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, buffer);
    }

    inline auto BindBufferAsShaderStorageBuffer(uint32_t buffer, int32_t bindingIndex) -> void {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, buffer);
    }

    inline auto BindTexture(int32_t bindingIndex, uint32_t texture) -> void {
        glBindTextureUnit(bindingIndex, texture);
    }

    inline auto BindTextureAndSampler(int32_t bindingIndex, uint32_t texture, uint32_t sampler) -> void {
        glBindTextureUnit(bindingIndex, texture);
        glBindSampler(bindingIndex, sampler);
    }

    inline auto SetUniform(int32_t location, float value) -> void {
        glProgramUniform1f(Id, location, value);
    }

    inline auto SetUniform(int32_t location, int32_t value) -> void {
        glProgramUniform1i(Id, location, value);
    }

    inline auto SetUniform(int32_t location, uint32_t value) -> void {
        glProgramUniform1ui(Id, location, value);
    }

    inline auto SetUniform(int32_t location, uint64_t value) -> void {
        glProgramUniformHandleui64ARB(Id, location, value);
    }

    inline auto SetUniform(int32_t location, const glm::vec2& value) -> void {
        glProgramUniform2fv(Id, location, 1, glm::value_ptr(value));
    }

    inline auto SetUniform(int32_t location, const glm::vec3& value) -> void {
        glProgramUniform3fv(Id, location, 1, glm::value_ptr(value));
    }

    inline auto SetUniform(int32_t location, float value1, float value2, float value3, float value4) -> void {
        glProgramUniform4f(Id, location, value1, value2, value3, value4);
    }

    inline auto SetUniform(int32_t location, int32_t value1, int32_t value2, int32_t value3, int32_t value4) -> void {
        glProgramUniform4i(Id, location, value1, value2, value3, value4);
    }

    inline auto SetUniform(int32_t location, const glm::vec4 value) -> void {
        glProgramUniform4fv(Id, location, 1, glm::value_ptr(value));
    }

    inline auto SetUniform(int32_t location, const glm::mat4& value) -> void {
        glProgramUniformMatrix4fv(Id, location, 1, GL_FALSE, glm::value_ptr(value));
    }
};

struct SGraphicsPipeline : public SPipeline {

    auto Bind() -> void override;
    auto BindBufferAsVertexBuffer(uint32_t buffer, uint32_t bindingIndex, size_t offset, size_t stride) -> void;
    auto DrawArrays(int32_t vertexOffset, size_t vertexCount) -> void;
    auto DrawElements(uint32_t indexBuffer, size_t elementCount) -> void;
    auto DrawElementsInstanced(uint32_t indexBuffer, size_t elementCount, size_t instanceCount) -> void;

    std::optional<uint32_t> InputLayout;
    uint32_t PrimitiveTopology;
    bool IsPrimitiveRestartEnabled;
};

struct SComputePipeline : public SPipeline {

};

inline auto SetDebugLabel(
    const uint32_t object,
    const uint32_t objectType,
    const std::string_view label) -> void {

    glObjectLabel(objectType, object, static_cast<GLsizei>(label.size()), label.data());
}

inline auto PushDebugGroup(const std::string_view label) -> void {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, label.size(), label.data());
}

inline auto PopDebugGroup() -> void {
    glPopDebugGroup();
}

inline auto CreateBuffer(
    std::string_view label,
    size_t sizeInBytes,
    const void* data,
    uint32_t flags) -> uint32_t {

    uint32_t buffer = 0;
    glCreateBuffers(1, &buffer);
    SetDebugLabel(buffer, GL_BUFFER, label);
    glNamedBufferStorage(buffer, sizeInBytes, data, flags);
    return buffer;
}

inline auto UpdateBuffer(
    uint32_t buffer,
    size_t offsetInBytes,
    size_t sizeInBytes,
    const void* data) -> void {

    glNamedBufferSubData(buffer, offsetInBytes, sizeInBytes, data);
}

inline auto DeleteBuffer(uint32_t buffer) -> void {

    glDeleteBuffers(1, &buffer);
}

inline auto DeletePipeline(SPipeline pipeline) -> void {

    glDeleteProgram(pipeline.Id);
}

uint32_t g_defaultInputLayout = 0;
uint32_t g_lastIndexBuffer = 0;

std::vector<SSampler> g_samplers;
SSamplerId g_samplerCounter = SSamplerId::Invalid;

std::vector<STexture> g_textures;
STextureId g_textureCounter = STextureId::Invalid;

std::unordered_map<SSamplerDescriptor, SSamplerId> g_samplerDescriptors;

auto InitializeRhi() -> bool {

    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        return false;
    }

    g_samplers.reserve(128);
    g_textures.reserve(16384);    

    glCreateVertexArrays(1, &g_defaultInputLayout);
    SetDebugLabel(g_defaultInputLayout, GL_VERTEX_ARRAY, "InputLayout-Default");

    return true;
}

auto UnloadRhi() -> void {

    glDeleteVertexArrays(1, &g_defaultInputLayout);
}

auto ReadTextFromFile(const std::filesystem::path& filePath) -> std::expected<std::string, std::string> {

    std::string pathStr = filePath.string();
    std::string parentPathStr = filePath.parent_path().string();
    char error[256]{};
    auto processedSource = std::unique_ptr<char, decltype([](char* ptr) { free(ptr); })>(stb_include_file(pathStr.data(), nullptr, parentPathStr.data(), error));
    if (!processedSource)
    {
        return std::unexpected(std::format("Failed to process includes for {}", filePath.string()));
    }

    return processedSource.get();
}

inline auto GetTexture(STextureId id) -> STexture& {

    assert(id != STextureId::Invalid);
    return g_textures[size_t(id)];
}

///// Conversions

constexpr auto PrimitiveTopologyToGL(EPrimitiveTopology primitiveTopology) -> uint32_t {
    switch (primitiveTopology) {
        case EPrimitiveTopology::Lines: return GL_LINES;
        case EPrimitiveTopology::TriangleFan: return GL_TRIANGLE_FAN;
        case EPrimitiveTopology::Triangles: return GL_TRIANGLES;
        case EPrimitiveTopology::TriangleStip: return GL_TRIANGLE_STRIP;
        default: std::unreachable();
    }
}

constexpr auto TextureAddressModeToGL(ETextureAddressMode textureAddressMode) -> uint32_t {
    switch (textureAddressMode) {
        case ETextureAddressMode::ClampToBorder : return GL_CLAMP_TO_BORDER;
        case ETextureAddressMode::ClampToEdge : return GL_CLAMP_TO_EDGE;
        case ETextureAddressMode::ClampToEdgeMirrored: return GL_MIRROR_CLAMP_TO_EDGE;
        case ETextureAddressMode::Repeat: return GL_REPEAT;
        case ETextureAddressMode::RepeatMirrored: return GL_MIRRORED_REPEAT;
        default: std::unreachable();
    }
}

constexpr auto TextureMagFilterToGL(ETextureMagFilter textureMagFilter) -> uint32_t {
    switch (textureMagFilter) {
        case ETextureMagFilter::Linear: return GL_LINEAR;
        case ETextureMagFilter::Nearest: return GL_NEAREST;
        default: std::unreachable();
    }
}

constexpr auto TextureMinFilterToGL(ETextureMinFilter textureMinFilter) -> uint32_t {
    switch (textureMinFilter) {
        case ETextureMinFilter::Linear: return GL_LINEAR;
        case ETextureMinFilter::Nearest: return GL_NEAREST;
        case ETextureMinFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
        case ETextureMinFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
        case ETextureMinFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
        case ETextureMinFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        default: std::unreachable();
    }
}

constexpr auto TextureTypeToGL(ETextureType textureType) -> uint32_t {

    switch (textureType)
    {
        case ETextureType::Texture1D: return GL_TEXTURE_1D;
        case ETextureType::Texture2D: return GL_TEXTURE_2D;
        case ETextureType::Texture3D: return GL_TEXTURE_3D;
        case ETextureType::Texture1DArray: return GL_TEXTURE_1D_ARRAY;
        case ETextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
        case ETextureType::TextureCube: return GL_TEXTURE_CUBE_MAP;
        case ETextureType::TextureCubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
        case ETextureType::Texture2DMultisample: return GL_TEXTURE_2D_MULTISAMPLE;
        case ETextureType::Texture2DMultisampleArray: return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        default: std::unreachable();
    }
}

constexpr auto UploadFormatToGL(EUploadFormat uploadFormat) -> uint32_t {

    switch (uploadFormat) {
        case EUploadFormat::R: return GL_RED;
        case EUploadFormat::Rg: return GL_RG;
        case EUploadFormat::Rgb: return GL_RGB;
        case EUploadFormat::Bgr: return GL_BGR;
        case EUploadFormat::Rgba: return GL_RGBA;
        case EUploadFormat::Bgra: return GL_BGRA;
        case EUploadFormat::RInteger: return GL_RED_INTEGER;
        case EUploadFormat::RgInteger: return GL_RG_INTEGER;
        case EUploadFormat::RgbInteger: return GL_RGB_INTEGER;
        case EUploadFormat::BgrInteger: return GL_BGR_INTEGER;
        case EUploadFormat::RgbaInteger: return GL_RGBA_INTEGER;
        case EUploadFormat::BgraInteger: return GL_BGRA_INTEGER;
        case EUploadFormat::Depth: return GL_DEPTH_COMPONENT;
        case EUploadFormat::StencilIndex: return GL_STENCIL_INDEX;
        case EUploadFormat::DepthStencilIndex: return GL_DEPTH_STENCIL;
        default: std::unreachable();
    }
}

constexpr auto UploadTypeToGL(EUploadType uploadType) -> uint32_t {

    switch (uploadType) {
        case EUploadType::UnsignedByte: return GL_UNSIGNED_BYTE;
        case EUploadType::SignedByte: return GL_BYTE;
        case EUploadType::UnsignedShort: return GL_UNSIGNED_SHORT;
        case EUploadType::SignedShort: return GL_SHORT;
        case EUploadType::UnsignedInteger: return GL_UNSIGNED_INT;
        case EUploadType::SignedInteger: return GL_INT;
        case EUploadType::Float: return GL_FLOAT;
        case EUploadType::UnsignedByte332: return GL_UNSIGNED_BYTE_3_3_2;
        case EUploadType::UnsignedByte233Reversed: return GL_UNSIGNED_BYTE_2_3_3_REV;
        case EUploadType::UnsignedShort565: return GL_UNSIGNED_SHORT_5_6_5;
        case EUploadType::UnsignedShort565Reversed: return GL_UNSIGNED_SHORT_5_6_5_REV;
        case EUploadType::UnsignedShort4444: return GL_UNSIGNED_SHORT_4_4_4_4;
        case EUploadType::UnsignedShort4444Reversed: return GL_UNSIGNED_SHORT_4_4_4_4_REV;
        case EUploadType::UnsignedShort5551: return GL_UNSIGNED_SHORT_5_5_5_1;
        case EUploadType::UnsignedShort1555Reversed: return GL_UNSIGNED_SHORT_1_5_5_5_REV;
        case EUploadType::UnsignedInteger8888: return GL_UNSIGNED_INT_8_8_8_8;
        case EUploadType::UnsignedInteger8888Reversed: return GL_UNSIGNED_INT_8_8_8_8_REV;
        case EUploadType::UnsignedInteger1010102: return GL_UNSIGNED_INT_10_10_10_2;
        case EUploadType::UnsignedInteger2101010Reversed: return GL_UNSIGNED_INT_2_10_10_10_REV;
        default: std::unreachable();
    }
}

constexpr auto SampleCountToGL(ESampleCount sampleCount) -> uint32_t {

    switch (sampleCount) {
        case ESampleCount::One: return 1;
        case ESampleCount::Two: return 2;
        case ESampleCount::Four: return 4;
        case ESampleCount::Eight: return 8;
        case ESampleCount::SixTeen: return 16;
        case ESampleCount::ThirtyTwo: return 32;
        default: std::unreachable();
    }
}

auto FormatToGL(EFormat format) -> uint32_t {
    switch (format) {
        case EFormat::R8_UNORM: return GL_R8;
        case EFormat::R8_SNORM: return GL_R8_SNORM;
        case EFormat::R16_UNORM: return GL_R16;
        case EFormat::R16_SNORM: return GL_R16_SNORM;
        case EFormat::R8G8_UNORM: return GL_RG8;
        case EFormat::R8G8_SNORM: return GL_RG8_SNORM;
        case EFormat::R16G16_UNORM: return GL_RG16;
        case EFormat::R16G16_SNORM: return GL_RG16_SNORM;
        case EFormat::R3G3B2_UNORM: return GL_R3_G3_B2;
        case EFormat::R4G4B4_UNORM: return GL_RGB4;
        case EFormat::R5G5B5_UNORM: return GL_RGB5;
        case EFormat::R8G8B8_UNORM: return GL_RGB8;
        case EFormat::R8G8B8_SNORM: return GL_RGB8_SNORM;
        case EFormat::R10G10B10_UNORM: return GL_RGB10;
        case EFormat::R12G12B12_UNORM: return GL_RGB12;
        // GL_RG16?
        case EFormat::R16G16B16_SNORM: return GL_RGB16_SNORM;
        case EFormat::R2G2B2A2_UNORM: return GL_RGBA2;
        case EFormat::R4G4B4A4_UNORM: return GL_RGBA4;
        case EFormat::R5G5B5A1_UNORM: return GL_RGB5_A1;
        case EFormat::R8G8B8A8_UNORM: return GL_RGBA8;
        case EFormat::R8G8B8A8_SNORM: return GL_RGBA8_SNORM;
        case EFormat::R10G10B10A2_UNORM: return GL_RGB10_A2;
        case EFormat::R10G10B10A2_UINT: return GL_RGB10_A2UI;
        case EFormat::R12G12B12A12_UNORM: return GL_RGBA12;
        case EFormat::R16G16B16A16_UNORM: return GL_RGBA16;
        case EFormat::R16G16B16A16_SNORM: return GL_RGBA16_SNORM;
        case EFormat::R8G8B8_SRGB: return GL_SRGB8;
        case EFormat::R8G8B8A8_SRGB: return GL_SRGB8_ALPHA8;
        case EFormat::R16_FLOAT: return GL_R16F;
        case EFormat::R16G16_FLOAT: return GL_RG16F;
        case EFormat::R16G16B16_FLOAT: return GL_RGB16F;
        case EFormat::R16G16B16A16_FLOAT: return GL_RGBA16F;
        case EFormat::R32_FLOAT: return GL_R32F;
        case EFormat::R32G32_FLOAT: return GL_RG32F;
        case EFormat::R32G32B32_FLOAT: return GL_RGB32F;
        case EFormat::R32G32B32A32_FLOAT: return GL_RGBA32F;
        case EFormat::R11G11B10_FLOAT: return GL_R11F_G11F_B10F;
        case EFormat::R9G9B9_E5: return GL_RGB9_E5;
        case EFormat::R8_SINT: return GL_R8I;
        case EFormat::R8_UINT: return GL_R8UI;
        case EFormat::R16_SINT: return GL_R16I;
        case EFormat::R16_UINT: return GL_R16UI;
        case EFormat::R32_SINT: return GL_R32I;
        case EFormat::R32_UINT: return GL_R32UI;
        case EFormat::R8G8_SINT: return GL_RG8I;
        case EFormat::R8G8_UINT: return GL_RG8UI;
        case EFormat::R16G16_SINT: return GL_RG16I;
        case EFormat::R16G16_UINT: return GL_RG16UI;
        case EFormat::R32G32_SINT: return GL_RG32I;
        case EFormat::R32G32_UINT: return GL_RG32UI;
        case EFormat::R8G8B8_SINT: return GL_RGB8I;
        case EFormat::R8G8B8_UINT: return GL_RGB8UI;
        case EFormat::R16G16B16_SINT: return GL_RGB16I;
        case EFormat::R16G16B16_UINT: return GL_RGB16UI;
        case EFormat::R32G32B32_SINT: return GL_RGB32I;
        case EFormat::R32G32B32_UINT: return GL_RGB32UI;
        case EFormat::R8G8B8A8_SINT: return GL_RGBA8I;
        case EFormat::R8G8B8A8_UINT: return GL_RGBA8UI;
        case EFormat::R16G16B16A16_SINT: return GL_RGBA16I;
        case EFormat::R16G16B16A16_UINT: return GL_RGBA16UI;
        case EFormat::R32G32B32A32_SINT: return GL_RGBA32I;
        case EFormat::R32G32B32A32_UINT: return GL_RGBA32UI;
        case EFormat::D32_FLOAT: return GL_DEPTH_COMPONENT32F;
        case EFormat::D32_UNORM: return GL_DEPTH_COMPONENT32;
        case EFormat::D24_UNORM: return GL_DEPTH_COMPONENT24;
        case EFormat::D16_UNORM: return GL_DEPTH_COMPONENT16;
        case EFormat::D32_FLOAT_S8_UINT: return GL_DEPTH32F_STENCIL8;
        case EFormat::D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
        case EFormat::S8_UINT: return GL_STENCIL_INDEX8;
        /*
        case EFormat::BC1_RGB_UNORM: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case EFormat::BC1_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case EFormat::BC1_RGB_SRGB: return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
        case EFormat::BC1_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
        case EFormat::BC2_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case EFormat::BC2_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
        case EFormat::BC3_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case EFormat::BC3_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
        */
        case EFormat::BC4_R_UNORM: return GL_COMPRESSED_RED_RGTC1;
        case EFormat::BC4_R_SNORM: return GL_COMPRESSED_SIGNED_RED_RGTC1;
        case EFormat::BC5_RG_UNORM: return GL_COMPRESSED_RG_RGTC2;
        case EFormat::BC5_RG_SNORM: return GL_COMPRESSED_SIGNED_RG_RGTC2;
        case EFormat::BC6H_RGB_UFLOAT: return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case EFormat::BC6H_RGB_SFLOAT: return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
        case EFormat::BC7_RGBA_UNORM: return GL_COMPRESSED_RGBA_BPTC_UNORM;
        case EFormat::BC7_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
        default:
            std::string message = "Format not mappable";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

auto AttachmentTypeToGL(EAttachmentType attachmentType) -> uint32_t {
    switch (attachmentType) {
        case EAttachmentType::ColorAttachment0: return GL_COLOR_ATTACHMENT0;
        case EAttachmentType::ColorAttachment1: return GL_COLOR_ATTACHMENT1;
        case EAttachmentType::ColorAttachment2: return GL_COLOR_ATTACHMENT2;
        case EAttachmentType::ColorAttachment3: return GL_COLOR_ATTACHMENT3;
        case EAttachmentType::ColorAttachment4: return GL_COLOR_ATTACHMENT4;
        case EAttachmentType::ColorAttachment5: return GL_COLOR_ATTACHMENT5;
        case EAttachmentType::ColorAttachment6: return GL_COLOR_ATTACHMENT6;
        case EAttachmentType::ColorAttachment7: return GL_COLOR_ATTACHMENT7;
        case EAttachmentType::DepthAttachment: return GL_DEPTH_ATTACHMENT;
        case EAttachmentType::StencilAttachment: return GL_STENCIL_ATTACHMENT;
        default:
            std::string message = "AttachmentType not mappable";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;        
    }
}

auto FormatToUploadFormat(EFormat format) -> EUploadFormat {

    switch (format)
    {
        case EFormat::R8_UNORM:
        case EFormat::R8_SNORM:
        case EFormat::R16_UNORM:
        case EFormat::R16_SNORM:
        case EFormat::R16_FLOAT:
        case EFormat::R32_FLOAT:
            return EUploadFormat::R;
        case EFormat::R8_SINT:
        case EFormat::R8_UINT:
        case EFormat::R16_SINT:
        case EFormat::R16_UINT:
        case EFormat::R32_SINT:
        case EFormat::R32_UINT:
            return EUploadFormat::RInteger;
        case EFormat::R8G8_UNORM:
        case EFormat::R8G8_SNORM:
        case EFormat::R16G16_UNORM:
        case EFormat::R16G16_SNORM:
        case EFormat::R16G16_FLOAT:
        case EFormat::R32G32_FLOAT:
            return EUploadFormat::Rg;
        case EFormat::R8G8_SINT:
        case EFormat::R8G8_UINT:
        case EFormat::R16G16_SINT:
        case EFormat::R16G16_UINT:
        case EFormat::R32G32_SINT:
        case EFormat::R32G32_UINT:
            return EUploadFormat::RgInteger;
        case EFormat::R3G3B2_UNORM:
        case EFormat::R4G4B4_UNORM:
        case EFormat::R5G5B5_UNORM:
        case EFormat::R8G8B8_UNORM:
        case EFormat::R8G8B8_SNORM:
        case EFormat::R10G10B10_UNORM:
        case EFormat::R12G12B12_UNORM:
        case EFormat::R16G16B16_SNORM:
        case EFormat::R8G8B8_SRGB:
        case EFormat::R16G16B16_FLOAT:
        case EFormat::R9G9B9_E5:
        case EFormat::R32G32B32_FLOAT:
        case EFormat::R11G11B10_FLOAT:
            return EUploadFormat::Rgb;
        case EFormat::R8G8B8_SINT:
        case EFormat::R8G8B8_UINT:
        case EFormat::R16G16B16_SINT:
        case EFormat::R16G16B16_UINT:
        case EFormat::R32G32B32_SINT:
        case EFormat::R32G32B32_UINT:
            return EUploadFormat::RgbInteger;
        case EFormat::R2G2B2A2_UNORM:
        case EFormat::R4G4B4A4_UNORM:
        case EFormat::R5G5B5A1_UNORM:
        case EFormat::R8G8B8A8_UNORM:
        case EFormat::R8G8B8A8_SNORM:
        case EFormat::R10G10B10A2_UNORM:
        case EFormat::R12G12B12A12_UNORM:
        case EFormat::R16G16B16A16_UNORM:
        case EFormat::R16G16B16A16_SNORM:
        case EFormat::R8G8B8A8_SRGB:
        case EFormat::R16G16B16A16_FLOAT:
        case EFormat::R32G32B32A32_FLOAT:
            return EUploadFormat::Rgba;
        case EFormat::R10G10B10A2_UINT:
        case EFormat::R8G8B8A8_SINT:
        case EFormat::R8G8B8A8_UINT:
        case EFormat::R16G16B16A16_SINT:
        case EFormat::R16G16B16A16_UINT:
        case EFormat::R32G32B32A32_SINT:
        case EFormat::R32G32B32A32_UINT:
            return EUploadFormat::RgbaInteger;
        case EFormat::D32_FLOAT:
        case EFormat::D32_UNORM:
        case EFormat::D24_UNORM:
        case EFormat::D16_UNORM:
            return EUploadFormat::Depth;
        case EFormat::D32_FLOAT_S8_UINT:
        case EFormat::D24_UNORM_S8_UINT:
            return EUploadFormat::DepthStencilIndex;
        case EFormat::S8_UINT:
            return EUploadFormat::StencilIndex;
        default: 
            std::unreachable();
	}
}

auto FormatToUnderlyingOpenGLType(EFormat format) -> uint32_t {

    switch (format)
    {
        case EFormat::R8_UNORM:
        case EFormat::R8G8_UNORM:
        case EFormat::R8G8B8_UNORM:
        case EFormat::R8G8B8A8_UNORM:
        case EFormat::R8_UINT:
        case EFormat::R8G8_UINT:
        case EFormat::R8G8B8_UINT:
        case EFormat::R8G8B8A8_UINT:
        case EFormat::R8G8B8A8_SRGB:
        case EFormat::R8G8B8_SRGB:
            return GL_UNSIGNED_BYTE;
        case EFormat::R8_SNORM:
        case EFormat::R8G8_SNORM:
        case EFormat::R8G8B8_SNORM:
        case EFormat::R8G8B8A8_SNORM:
        case EFormat::R8_SINT:
        case EFormat::R8G8_SINT:
        case EFormat::R8G8B8_SINT:
        case EFormat::R8G8B8A8_SINT:
            return GL_BYTE;
        case EFormat::R16_UNORM:
        case EFormat::R16G16_UNORM:
        case EFormat::R16G16B16A16_UNORM:
        case EFormat::R16_UINT:
        case EFormat::R16G16_UINT:
        case EFormat::R16G16B16_UINT:
        case EFormat::R16G16B16A16_UINT:
            return GL_UNSIGNED_SHORT;
        case EFormat::R16_SNORM:
        case EFormat::R16G16_SNORM:
        case EFormat::R16G16B16_SNORM:
        case EFormat::R16G16B16A16_SNORM:
        case EFormat::R16_SINT:
        case EFormat::R16G16_SINT:
        case EFormat::R16G16B16_SINT:
        case EFormat::R16G16B16A16_SINT:
            return GL_SHORT;
        case EFormat::R16_FLOAT:
        case EFormat::R16G16_FLOAT:
        case EFormat::R16G16B16_FLOAT:
        case EFormat::R16G16B16A16_FLOAT:
            return GL_HALF_FLOAT;
        case EFormat::R32_FLOAT:
        case EFormat::R32G32_FLOAT:
        case EFormat::R32G32B32_FLOAT:
        case EFormat::R32G32B32A32_FLOAT:
            return GL_FLOAT;
        case EFormat::R32_SINT:
        case EFormat::R32G32_SINT:
        case EFormat::R32G32B32_SINT:
        case EFormat::R32G32B32A32_SINT:
            return GL_INT;
        case EFormat::R32_UINT:
        case EFormat::R32G32_UINT:
        case EFormat::R32G32B32_UINT:
        case EFormat::R32G32B32A32_UINT:
            return GL_UNSIGNED_INT;
        default:
            std::string message = "Format not mappable to opengl type";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

constexpr auto TextureTypeToDimension(ETextureType textureType) -> uint32_t {

    switch (textureType) {
        case ETextureType::Texture1D:
            return 1;
        case ETextureType::Texture2D: 
        case ETextureType::Texture2DMultisample:
        case ETextureType::Texture1DArray:
            return 2;
        case ETextureType::Texture3D:
        case ETextureType::Texture2DArray:
        case ETextureType::Texture2DMultisampleArray:
        case ETextureType::TextureCube:
        case ETextureType::TextureCubeArray:
            return 3;
        default:
            std::unreachable();
    } 
}

auto FormatToBaseTypeClass(EFormat format) -> EBaseTypeClass
{
    switch (format)
    {
        case EFormat::R8_UNORM:
        case EFormat::R8_SNORM:
        case EFormat::R16_UNORM:
        case EFormat::R16_SNORM:
        case EFormat::R8G8_UNORM:
        case EFormat::R8G8_SNORM:
        case EFormat::R16G16_UNORM:
        case EFormat::R16G16_SNORM:
        case EFormat::R3G3B2_UNORM:
        case EFormat::R4G4B4_UNORM:
        case EFormat::R5G5B5_UNORM:
        case EFormat::R8G8B8_UNORM:
        case EFormat::R8G8B8_SNORM:
        case EFormat::R10G10B10_UNORM:
        case EFormat::R12G12B12_UNORM:
        case EFormat::R16G16B16_SNORM:
        case EFormat::R2G2B2A2_UNORM:
        case EFormat::R4G4B4A4_UNORM:
        case EFormat::R5G5B5A1_UNORM:
        case EFormat::R8G8B8A8_UNORM:
        case EFormat::R8G8B8A8_SNORM:
        case EFormat::R10G10B10A2_UNORM:
        case EFormat::R12G12B12A12_UNORM:
        case EFormat::R16G16B16A16_UNORM:
        case EFormat::R8G8B8_SRGB:
        case EFormat::R8G8B8A8_SRGB:
        case EFormat::R16_FLOAT:
        case EFormat::R16G16_FLOAT:
        case EFormat::R16G16B16_FLOAT:
        case EFormat::R16G16B16A16_FLOAT:
        case EFormat::R32_FLOAT:
        case EFormat::R32G32_FLOAT:
        case EFormat::R32G32B32_FLOAT:
        case EFormat::R32G32B32A32_FLOAT:
        case EFormat::R11G11B10_FLOAT:
        case EFormat::R9G9B9_E5:
            return EBaseTypeClass::Float;
        case EFormat::R8_SINT:
        case EFormat::R16_SINT:
        case EFormat::R32_SINT:
        case EFormat::R8G8_SINT:
        case EFormat::R16G16_SINT:
        case EFormat::R32G32_SINT:
        case EFormat::R8G8B8_SINT:
        case EFormat::R16G16B16_SINT:
        case EFormat::R32G32B32_SINT:
        case EFormat::R8G8B8A8_SINT:
        case EFormat::R16G16B16A16_SINT:
        case EFormat::R32G32B32A32_SINT:
            return EBaseTypeClass::Integer;
        case EFormat::R10G10B10A2_UINT:
        case EFormat::R8_UINT:
        case EFormat::R16_UINT:
        case EFormat::R32_UINT:
        case EFormat::R8G8_UINT:
        case EFormat::R16G16_UINT:
        case EFormat::R32G32_UINT:
        case EFormat::R8G8B8_UINT:
        case EFormat::R16G16B16_UINT:
        case EFormat::R32G32B32_UINT:
        case EFormat::R8G8B8A8_UINT:
        case EFormat::R16G16B16A16_UINT:
        case EFormat::R32G32B32A32_UINT:
            return EBaseTypeClass::UnsignedInteger;
        default:
            std::unreachable();
    }
}

constexpr auto FormatToAttachmentType(EFormat attachmentFormat, size_t colorAttachmentIndex) -> EAttachmentType {

    switch (attachmentFormat) {
        case EFormat::D24_UNORM:
        case EFormat::D24_UNORM_S8_UINT:
        case EFormat::D32_FLOAT:
        case EFormat::D32_FLOAT_S8_UINT:
        case EFormat::D32_UNORM:
            return EAttachmentType::DepthAttachment;
        default:
            return static_cast<EAttachmentType>(static_cast<std::underlying_type<EAttachmentType>::type>(EAttachmentType::ColorAttachment0) + colorAttachmentIndex);
    }
}

auto FormatToComponentCount(EFormat format) -> int32_t {
    switch (format) {
        case EFormat::R8_UNORM:
        case EFormat::R8_SNORM:
        case EFormat::R16_UNORM:
        case EFormat::R16_SNORM:
        case EFormat::R16_FLOAT:
        case EFormat::R32_FLOAT:
        case EFormat::R8_SINT:
        case EFormat::R16_SINT:
        case EFormat::R32_SINT:
        case EFormat::R8_UINT:
        case EFormat::R16_UINT:
        case EFormat::R32_UINT:
            return 1;
        case EFormat::R8G8_UNORM:
        case EFormat::R8G8_SNORM:
        case EFormat::R16G16_FLOAT:
        case EFormat::R16G16_UNORM:
        case EFormat::R16G16_SNORM:
        case EFormat::R32G32_FLOAT:
        case EFormat::R8G8_SINT:
        case EFormat::R16G16_SINT:
        case EFormat::R32G32_SINT:
        case EFormat::R8G8_UINT:
        case EFormat::R16G16_UINT:
        case EFormat::R32G32_UINT:
            return 2;
        case EFormat::R8G8B8_UNORM:
        case EFormat::R8G8B8_SNORM:
        case EFormat::R16G16B16_SNORM:
        case EFormat::R16G16B16_FLOAT:
        case EFormat::R32G32B32_FLOAT:
        case EFormat::R8G8B8_SINT:
        case EFormat::R16G16B16_SINT:
        case EFormat::R32G32B32_SINT:
        case EFormat::R8G8B8_UINT:
        case EFormat::R16G16B16_UINT:
        case EFormat::R32G32B32_UINT:
            return 3;
        case EFormat::R8G8B8A8_UNORM:
        case EFormat::R8G8B8A8_SNORM:
        case EFormat::R16G16B16A16_UNORM:
        case EFormat::R16G16B16A16_FLOAT:
        case EFormat::R32G32B32A32_FLOAT:
        case EFormat::R8G8B8A8_SINT:
        case EFormat::R16G16B16A16_SINT:
        case EFormat::R32G32B32A32_SINT:
        case EFormat::R10G10B10A2_UINT:
        case EFormat::R8G8B8A8_UINT:
        case EFormat::R16G16B16A16_UINT:
        case EFormat::R32G32B32A32_UINT:
            return 4;
        default:
            std::unreachable();
    }
}

auto IsFormatNormalized(EFormat format) -> int32_t {

    switch (format) {
        case EFormat::R8_UNORM:
        case EFormat::R8_SNORM:
        case EFormat::R16_UNORM:
        case EFormat::R16_SNORM:
        case EFormat::R8G8_UNORM:
        case EFormat::R8G8_SNORM:
        case EFormat::R16G16_UNORM:
        case EFormat::R16G16_SNORM:
        case EFormat::R8G8B8_UNORM:
        case EFormat::R8G8B8_SNORM:
        case EFormat::R16G16B16_SNORM:
        case EFormat::R8G8B8A8_UNORM:
        case EFormat::R8G8B8A8_SNORM:
        case EFormat::R16G16B16A16_UNORM:
            return GL_TRUE;
        case EFormat::R16_FLOAT:
        case EFormat::R32_FLOAT:
        case EFormat::R8_SINT:
        case EFormat::R16_SINT:
        case EFormat::R32_SINT:
        case EFormat::R8_UINT:
        case EFormat::R16_UINT:
        case EFormat::R32_UINT:
        case EFormat::R16G16_FLOAT:
        case EFormat::R32G32_FLOAT:
        case EFormat::R8G8_SINT:
        case EFormat::R16G16_SINT:
        case EFormat::R32G32_SINT:
        case EFormat::R8G8_UINT:
        case EFormat::R16G16_UINT:
        case EFormat::R32G32_UINT:
        case EFormat::R16G16B16_FLOAT:
        case EFormat::R32G32B32_FLOAT:
        case EFormat::R8G8B8_SINT:
        case EFormat::R16G16B16_SINT:
        case EFormat::R32G32B32_SINT:
        case EFormat::R8G8B8_UINT:
        case EFormat::R16G16B16_UINT:
        case EFormat::R32G32B32_UINT:
        case EFormat::R16G16B16A16_FLOAT:
        case EFormat::R32G32B32A32_FLOAT:
        case EFormat::R8G8B8A8_SINT:
        case EFormat::R16G16B16A16_SINT:
        case EFormat::R32G32B32A32_SINT:
        case EFormat::R10G10B10A2_UINT:
        case EFormat::R8G8B8A8_UINT:
        case EFormat::R16G16B16A16_UINT:
        case EFormat::R32G32B32A32_UINT:
            return GL_FALSE;
        default:
            std::unreachable();
    }
}

auto FormatToFormatClass(EFormat format) -> EFormatClass
  {
    switch (format)
    {
        case EFormat::R8_UNORM:
        case EFormat::R8_SNORM:
        case EFormat::R16_UNORM:
        case EFormat::R16_SNORM:
        case EFormat::R8G8_UNORM:
        case EFormat::R8G8_SNORM:
        case EFormat::R16G16_UNORM:
        case EFormat::R16G16_SNORM:
        case EFormat::R8G8B8_UNORM:
        case EFormat::R8G8B8_SNORM:
        case EFormat::R16G16B16_SNORM:
        case EFormat::R8G8B8A8_UNORM:
        case EFormat::R8G8B8A8_SNORM:
        case EFormat::R16G16B16A16_UNORM:
        case EFormat::R16_FLOAT:
        case EFormat::R16G16_FLOAT:
        case EFormat::R16G16B16_FLOAT:
        case EFormat::R16G16B16A16_FLOAT:
        case EFormat::R32_FLOAT:
        case EFormat::R32G32_FLOAT:
        case EFormat::R32G32B32_FLOAT:
        case EFormat::R32G32B32A32_FLOAT:
            return EFormatClass::Float;
        case EFormat::R8_SINT:
        case EFormat::R16_SINT:
        case EFormat::R32_SINT:
        case EFormat::R8G8_SINT:
        case EFormat::R16G16_SINT:
        case EFormat::R32G32_SINT:
        case EFormat::R8G8B8_SINT:
        case EFormat::R16G16B16_SINT:
        case EFormat::R32G32B32_SINT:
        case EFormat::R8G8B8A8_SINT:
        case EFormat::R16G16B16A16_SINT:
        case EFormat::R32G32B32A32_SINT:
        case EFormat::R10G10B10A2_UINT:
        case EFormat::R8_UINT:
        case EFormat::R16_UINT:
        case EFormat::R32_UINT:
        case EFormat::R8G8_UINT:
        case EFormat::R16G16_UINT:
        case EFormat::R32G32_UINT:
        case EFormat::R8G8B8_UINT:
        case EFormat::R16G16B16_UINT:
        case EFormat::R32G32B32_UINT:
        case EFormat::R8G8B8A8_UINT:
        case EFormat::R16G16B16A16_UINT:
        case EFormat::R32G32B32A32_UINT:
            return EFormatClass::Integer;
        default: 
            return EFormatClass::Long;
    }
}

/////////////////////////////////////////////////////////////

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

    auto imageFile = fopen(filePath.c_str(), "rb");
    if (imageFile != nullptr) {
        auto* pixels = stbi_load_from_file(imageFile, width, height, components, 4);
        fclose(imageFile);
        return pixels;
    } else {
        return nullptr;
    }
}

auto CreateTexture(const SCreateTextureDescriptor& createTextureDescriptor) -> STextureId {

    ZoneScopedN("CreateTexture");
    STexture texture = {};

    glCreateTextures(TextureTypeToGL(createTextureDescriptor.TextureType), 1, &texture.Id);
    if (!createTextureDescriptor.Label.empty()) {
        SetDebugLabel(texture.Id, GL_TEXTURE, createTextureDescriptor.Label);
    }

    texture.Extent = createTextureDescriptor.Extent;
    texture.Format = createTextureDescriptor.Format;
    texture.TextureType = createTextureDescriptor.TextureType;

    switch (createTextureDescriptor.TextureType)
    {
        case ETextureType::Texture1D:
            glTextureStorage1D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width);
            break;
        case ETextureType::Texture2D:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height);
            break;
        case ETextureType::Texture3D:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Extent.Depth);
            break;
        case ETextureType::Texture1DArray:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Layers);
            break;
        case ETextureType::Texture2DArray:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Layers);
            break;
        case ETextureType::TextureCube:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height);
            break;
        case ETextureType::TextureCubeArray:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Layers);
            break;
        case ETextureType::Texture2DMultisample:
            glTextureStorage2DMultisample(texture.Id,
                                          SampleCountToGL(createTextureDescriptor.SampleCount),
                                          FormatToGL(createTextureDescriptor.Format),
                                          createTextureDescriptor.Extent.Width,
                                          createTextureDescriptor.Extent.Height,
                                          GL_TRUE);
            break;
        case ETextureType::Texture2DMultisampleArray:
            glTextureStorage3DMultisample(texture.Id,
                                          SampleCountToGL(createTextureDescriptor.SampleCount),
                                          FormatToGL(createTextureDescriptor.Format),
                                          createTextureDescriptor.Extent.Width,
                                          createTextureDescriptor.Extent.Height,
                                          createTextureDescriptor.Layers,
                                          GL_TRUE);
            break;
        default:
            std::unreachable();
    }

    const auto textureId = STextureId(g_textures.size());
    g_textures.emplace_back(texture);    

    return textureId;
}

auto UploadTexture(const STextureId& textureId, const SUploadTextureDescriptor& updateTextureDescriptor) -> void {

    ZoneScopedN("UploadTexture");
    auto& texture = GetTexture(textureId);

    uint32_t format = 0;
    if (updateTextureDescriptor.UploadFormat == EUploadFormat::Auto)
    {
        format = UploadFormatToGL(FormatToUploadFormat(texture.Format));
    }
    else
    {
        format = UploadFormatToGL(updateTextureDescriptor.UploadFormat);
    }

    uint32_t type = 0;
    if (updateTextureDescriptor.UploadType == EUploadType::Auto)
    {
        type = FormatToUnderlyingOpenGLType(texture.Format);
    }
    else
    {
        type = UploadTypeToGL(updateTextureDescriptor.UploadType);
    }

    switch (TextureTypeToDimension(texture.TextureType))
    {
        case 1:
            glTextureSubImage1D(texture.Id,
                                updateTextureDescriptor.Level,
                                updateTextureDescriptor.Offset.X,
                                updateTextureDescriptor.Extent.Width,
                                format,
                                type,
                                updateTextureDescriptor.PixelData);
            break;
        case 2:
            glTextureSubImage2D(texture.Id,
                                updateTextureDescriptor.Level,
                                updateTextureDescriptor.Offset.X,
                                updateTextureDescriptor.Offset.Y,
                                updateTextureDescriptor.Extent.Width,
                                updateTextureDescriptor.Extent.Height,
                                format,
                                type,
                                updateTextureDescriptor.PixelData);
            break;
        case 3:
            glTextureSubImage3D(texture.Id,
                                updateTextureDescriptor.Level,
                                updateTextureDescriptor.Offset.X,
                                updateTextureDescriptor.Offset.Y,
                                updateTextureDescriptor.Offset.Z,
                                updateTextureDescriptor.Extent.Width,
                                updateTextureDescriptor.Extent.Height,
                                updateTextureDescriptor.Extent.Depth,
                                format,
                                type,
                                updateTextureDescriptor.PixelData);
            break;
    }
}

auto MakeTextureResident(const STextureId& textureId) -> uint64_t {
    
    auto& texture = GetTexture(textureId);

    auto textureHandle = glGetTextureHandleARB(texture.Id);
    glMakeTextureHandleResidentARB(textureHandle);

    return textureHandle;
}

auto GenerateMipmaps(const STextureId textureId) -> void {

    auto& texture = GetTexture(textureId);
    glGenerateTextureMipmap(texture.Id);
}

auto GetOrCreateSampler(const SSamplerDescriptor& samplerDescriptor) -> SSamplerId {

    if (auto existingSamplerDescriptor = g_samplerDescriptors.find(samplerDescriptor); existingSamplerDescriptor != g_samplerDescriptors.end()) {
        return existingSamplerDescriptor->second;
    }

    SSampler sampler = {};
    glCreateSamplers(1, &sampler.Id);
    SetDebugLabel(sampler.Id, GL_SAMPLER, samplerDescriptor.Label);
    glSamplerParameteri(sampler.Id, GL_TEXTURE_WRAP_S, TextureAddressModeToGL(samplerDescriptor.AddressModeU));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_WRAP_T, TextureAddressModeToGL(samplerDescriptor.AddressModeV));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_WRAP_R, TextureAddressModeToGL(samplerDescriptor.AddressModeW));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MAG_FILTER, TextureMagFilterToGL(samplerDescriptor.MagFilter));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MIN_FILTER, TextureMinFilterToGL(samplerDescriptor.MinFilter));
    glSamplerParameterf(sampler.Id, GL_TEXTURE_LOD_BIAS, samplerDescriptor.LodBias);
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MIN_LOD, samplerDescriptor.LodMin);
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MAX_LOD, samplerDescriptor.LodMax);

    const auto samplerId = SSamplerId(g_samplers.size());
    g_samplerDescriptors.insert({samplerDescriptor, samplerId});
    g_samplers.emplace_back(sampler);

    return samplerId;
};

auto CreateFramebuffer(const SFramebufferDescriptor& framebufferDescriptor) -> SFramebuffer {

    SFramebuffer framebuffer = {};
    glCreateFramebuffers(1, &framebuffer.Id);
    if (!framebufferDescriptor.Label.empty()) {
        SetDebugLabel(framebuffer.Id, GL_FRAMEBUFFER, framebufferDescriptor.Label);
    }

    std::array<uint32_t, 8> drawBuffers = {};

    for (auto colorAttachmentIndex = 0; auto colorAttachmentDescriptorValue : framebufferDescriptor.ColorAttachments) {
        if (colorAttachmentDescriptorValue.has_value()) {
            auto& colorAttachmentDescriptor = *colorAttachmentDescriptorValue;
            auto colorAttachmentTextureId = CreateTexture({
                .TextureType = ETextureType::Texture2D,
                .Format = colorAttachmentDescriptor.Format,
                .Extent = SExtent3D(colorAttachmentDescriptor.Extent.Width, colorAttachmentDescriptor.Extent.Height, 1),
                .MipMapLevels = 1,
                .Layers = 0,
                .SampleCount = ESampleCount::One,
                .Label = std::format("{}_{}x{}", colorAttachmentDescriptor.Label, colorAttachmentDescriptor.Extent.Width, colorAttachmentDescriptor.Extent.Height)
            });
            auto& colorAttachmentTexture = GetTexture(colorAttachmentTextureId);

            framebuffer.ColorAttachments[colorAttachmentIndex] = {
                .Texture = colorAttachmentTexture,
                .ClearColor = colorAttachmentDescriptor.ClearColor,
                .LoadOperation = colorAttachmentDescriptor.LoadOperation,
            };

            auto attachmentType = FormatToAttachmentType(colorAttachmentDescriptor.Format, colorAttachmentIndex);
            glNamedFramebufferTexture(framebuffer.Id, AttachmentTypeToGL(attachmentType), colorAttachmentTexture.Id, 0);

            drawBuffers[colorAttachmentIndex] = AttachmentTypeToGL(attachmentType);

        } else {
            framebuffer.ColorAttachments[colorAttachmentIndex] = std::nullopt;
            drawBuffers[colorAttachmentIndex] = GL_NONE;
        }

        colorAttachmentIndex++;        
    }

    glNamedFramebufferDrawBuffers(framebuffer.Id, 8, drawBuffers.data());

    if (framebufferDescriptor.DepthStencilAttachment.has_value()) {
        auto& depthStencilAttachment = *framebufferDescriptor.DepthStencilAttachment;
        auto depthTextureId = CreateTexture({
            .TextureType = ETextureType::Texture2D,
            .Format = depthStencilAttachment.Format,
            .Extent = SExtent3D(depthStencilAttachment.Extent.Width, depthStencilAttachment.Extent.Height, 1),
            .MipMapLevels = 1,
            .Layers = 0,
            .SampleCount = ESampleCount::One,
            .Label = std::format("{}_{}x{}", depthStencilAttachment.Label, depthStencilAttachment.Extent.Width, depthStencilAttachment.Extent.Height)
        });
        auto& depthTexture = g_textures[size_t(depthTextureId)];

        auto attachmentType = FormatToAttachmentType(depthStencilAttachment.Format, 0);
        glNamedFramebufferTexture(framebuffer.Id, AttachmentTypeToGL(attachmentType), depthTexture.Id, 0);

        framebuffer.DepthStencilAttachment = {
            .Texture = depthTexture,
            .ClearDepthStencil = depthStencilAttachment.ClearDepthStencil,
            .LoadOperation = depthStencilAttachment.LoadOperation,
        };
    } else {
        framebuffer.DepthStencilAttachment = std::nullopt;
    }

    auto framebufferStatus = glCheckNamedFramebufferStatus(framebuffer.Id, GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        auto message = std::format("Framebuffer {} is incomplete", framebufferDescriptor.Label);
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
    }

    return framebuffer;
}

auto BindFramebuffer(const SFramebuffer& framebuffer) -> void {

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.Id);

    for (auto colorAttachmentIndex = 0; auto colorAttachmentValue : framebuffer.ColorAttachments) {
        if (colorAttachmentValue.has_value()) {
            auto& colorAttachment = *colorAttachmentValue;
            if (colorAttachment.LoadOperation == EFramebufferAttachmentLoadOperation::Clear) {
                auto baseTypeClass = FormatToBaseTypeClass(colorAttachment.Texture.Format);
                switch (baseTypeClass) {
                    case EBaseTypeClass::Float:
                        glClearNamedFramebufferfv(0, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<float, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    case EBaseTypeClass::Integer:
                        glClearNamedFramebufferiv(0, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<int32_t, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    case EBaseTypeClass::UnsignedInteger:
                        glClearNamedFramebufferuiv(0, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<uint32_t, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    default:
                        std::unreachable_sentinel;
                }
            }
        }
        colorAttachmentIndex++;
    }

    if (framebuffer.DepthStencilAttachment.has_value()) {
        auto& depthStencilAttachment = *framebuffer.DepthStencilAttachment;
        if (depthStencilAttachment.LoadOperation == EFramebufferAttachmentLoadOperation::Clear) {
            glClearNamedFramebufferfi(framebuffer.Id, GL_DEPTH_STENCIL, 0, depthStencilAttachment.ClearDepthStencil.Depth, depthStencilAttachment.ClearDepthStencil.Stencil);
        }
    }
}

auto DeleteFramebuffer(const SFramebuffer& framebuffer) -> void {

    assert(framebuffer.Id != 0);

    for (auto colorAttachment : framebuffer.ColorAttachments) {
        if (colorAttachment.has_value()) {
            glDeleteTextures(1, &(*colorAttachment).Texture.Id);
        }
    }

    if (framebuffer.DepthStencilAttachment.has_value()) {
        glDeleteTextures(1, &(*framebuffer.DepthStencilAttachment).Texture.Id);
    }

    glDeleteFramebuffers(1, &framebuffer.Id);
}

auto CreateGraphicsProgram(
    std::string_view label,
    std::string_view vertexShaderFilePath,
    std::string_view fragmentShaderFilePath) -> std::expected<uint32_t, std::string> {

    auto vertexShaderSourceResult = ReadTextFromFile(vertexShaderFilePath);
    if (!vertexShaderSourceResult) {
        return std::unexpected(vertexShaderSourceResult.error());
    }

    auto fragmentShaderSourceResult = ReadTextFromFile(fragmentShaderFilePath);
    if (!fragmentShaderSourceResult) {
        return std::unexpected(fragmentShaderSourceResult.error());
    }

    auto vertexShaderSource = *vertexShaderSourceResult;
    auto fragmentShaderSource = *fragmentShaderSourceResult;

    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto vertexShaderSourcePtr = vertexShaderSource.data();
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    glCompileShader(vertexShader);
    int32_t status = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {

        int32_t infoLength = 512;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(vertexShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(vertexShader);

        return std::unexpected(std::format("Vertex shader in program {} has errors\n{}", label, infoLog));
    }

    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragmentShaderSourcePtr = fragmentShaderSource.data();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {

        int32_t infoLength = 512;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(fragmentShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(fragmentShader);

        return std::unexpected(std::format("Fragment shader in program {} has errors\n{}", label, infoLog));
    }

    auto program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {

        int32_t infoLength = 512;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetProgramInfoLog(program, infoLength, nullptr, infoLog.data());

        return std::unexpected(std::format("Graphics program {} has linking errors\n{}", label, infoLog));
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

auto CreateComputeProgram(
    std::string_view label,
    std::string_view computeShaderFilePath) -> std::expected<uint32_t, std::string> {

    auto computeShaderSourceResult = ReadTextFromFile(computeShaderFilePath);
    if (!computeShaderSourceResult) {
        return std::unexpected(computeShaderSourceResult.error());
    }

    int32_t status = 0;

    auto computeShader = glCreateShader(GL_COMPUTE_SHADER);
    auto computeShaderSourcePtr = (*computeShaderSourceResult).data();
    glShaderSource(computeShader, 1, &computeShaderSourcePtr, nullptr);
    glCompileShader(computeShader);
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {

        int32_t infoLength = 512;
        glGetShaderiv(computeShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(computeShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(computeShader);

        return std::unexpected(std::format("Compute shader in program {} has errors\n{}", label, infoLog));
    }

    auto program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {

        int32_t infoLength = 512;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetProgramInfoLog(program, infoLength, nullptr, infoLog.data());

        return std::unexpected(std::format("Compute program {} has linking errors\n{}", label, infoLog));
    }

    glDetachShader(program, computeShader);
    glDeleteShader(computeShader);

    return program;
}

auto CreateGraphicsPipeline(const SGraphicsPipelineDescriptor& graphicsPipelineDescriptor) -> std::expected<SGraphicsPipeline, std::string> {

    SGraphicsPipeline pipeline = {};

    auto graphicsProgramResult = CreateGraphicsProgram(graphicsPipelineDescriptor.Label,
                                                       graphicsPipelineDescriptor.VertexShaderFilePath,
                                                       graphicsPipelineDescriptor.FragmentShaderFilePath);
    if (!graphicsProgramResult) {
        return std::unexpected(std::format("RHI: Unable to build GraphicsPipeline {}\n{}", graphicsPipelineDescriptor.Label, graphicsProgramResult.error()));
    }

    pipeline.Id = *graphicsProgramResult;

    if (graphicsPipelineDescriptor.VertexInput.has_value()) {

        uint32_t inputLayout = 0;
        glCreateVertexArrays(1, &inputLayout);

        auto& vertexInput = *graphicsPipelineDescriptor.VertexInput;
        for(auto inputAttributeIndex = 0; auto& inputAttribute : vertexInput.VertexInputAttributes) {
            if (inputAttribute.has_value()) {
                auto& inputAttributeValue = *inputAttribute;
                
                glEnableVertexArrayAttrib(inputLayout, inputAttributeValue.Location);
                glVertexArrayAttribBinding(inputLayout, inputAttributeValue.Location, inputAttributeValue.Binding);

                auto type = FormatToUnderlyingOpenGLType(inputAttributeValue.Format);
                auto componentCount = FormatToComponentCount(inputAttributeValue.Format);
                auto isFormatNormalized = IsFormatNormalized(inputAttributeValue.Format);
                auto formatClass = FormatToFormatClass(inputAttributeValue.Format);
                switch (formatClass)
                {
                    case EFormatClass::Float:
                        glVertexArrayAttribFormat(inputLayout, inputAttributeValue.Location, componentCount, type, isFormatNormalized, inputAttributeValue.Offset);
                        break;
                    case EFormatClass::Integer:
                        glVertexArrayAttribIFormat(inputLayout, inputAttributeValue.Location, componentCount, type, inputAttributeValue.Offset);
                        break;
                    case EFormatClass::Long:
                        glVertexArrayAttribLFormat(inputLayout, inputAttributeValue.Location, componentCount, type, inputAttributeValue.Offset);
                        break;
                    default: 
                        std::string message = "Unsupported Format Class";
                        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
                        break;
                }
            }

            inputAttributeIndex++;
        }

        SetDebugLabel(inputLayout, GL_VERTEX_ARRAY, std::format("InputLayout-{}", graphicsPipelineDescriptor.Label));

        pipeline.InputLayout = inputLayout;
    } else {
        pipeline.InputLayout = std::nullopt;
    }
    
    pipeline.PrimitiveTopology = PrimitiveTopologyToGL(graphicsPipelineDescriptor.InputAssembly.PrimitiveTopology);
    pipeline.IsPrimitiveRestartEnabled = graphicsPipelineDescriptor.InputAssembly.IsPrimitiveRestartEnabled;

    return pipeline;
}

auto CreateComputePipeline(const SComputePipelineDescriptor& computePipelineDescriptor) -> std::expected<SComputePipeline, std::string> {

    SComputePipeline pipeline = {};

    auto computeProgramResult = CreateComputeProgram(computePipelineDescriptor.Label,
                                                     computePipelineDescriptor.ComputeShaderFilePath);
    if (!computeProgramResult) {
        return std::unexpected(std::format("RHI: Unable to build ComputePipeline {}\n{}", computePipelineDescriptor.Label, computeProgramResult.error()));
    }

    pipeline.Id = *computeProgramResult;

    return pipeline;
}

auto SGraphicsPipeline::Bind() -> void {
    SPipeline::Bind();
    glBindVertexArray(InputLayout.has_value() ? *InputLayout : g_defaultInputLayout);
}

auto SGraphicsPipeline::BindBufferAsVertexBuffer(uint32_t buffer, uint32_t bindingIndex, size_t offset, size_t stride) -> void {

    if (InputLayout.has_value()) {
        glVertexArrayVertexBuffer(*InputLayout, bindingIndex, buffer, offset, stride);
    }
}

auto SGraphicsPipeline::DrawArrays(int32_t vertexOffset, size_t vertexCount) -> void {

    glDrawArrays(PrimitiveTopology, vertexOffset, vertexCount);
}

auto SGraphicsPipeline::DrawElements(uint32_t indexBuffer, size_t elementCount) -> void {

    if (g_lastIndexBuffer != indexBuffer) {
        glVertexArrayElementBuffer(InputLayout.has_value() ? InputLayout.value() : g_defaultInputLayout, indexBuffer);
        g_lastIndexBuffer = indexBuffer;
    }

    glDrawElements(PrimitiveTopology, elementCount, GL_UNSIGNED_INT, nullptr);
}

auto SGraphicsPipeline::DrawElementsInstanced(uint32_t indexBuffer, size_t elementCount, size_t instanceCount) -> void {

    if (g_lastIndexBuffer != indexBuffer) {
        glVertexArrayElementBuffer(InputLayout.has_value() ? InputLayout.value() : g_defaultInputLayout, indexBuffer);
        g_lastIndexBuffer = indexBuffer;
    }

    glDrawElementsInstanced(PrimitiveTopology, elementCount, GL_UNSIGNED_INT, nullptr, instanceCount);
}

// - Engine -------------------------------------------------------------------

struct SGpuVertexPosition {
    glm::vec3 Position;
};

struct SGpuVertexNormalUvTangent {
    uint32_t Normal;
    uint32_t Tangent;
    glm::vec3 UvAndTangentSign;
};

struct SGpuVertexPositionColor {
    glm::vec3 Position;
    glm::vec4 Color;
};

struct SGpuGlobalUniforms {

    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 CameraPosition;
};

struct SGpuGlobalLight {
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 Direction;
    glm::vec4 Strength;
};

struct SDebugLine {
    glm::vec3 StartPosition;
    glm::vec4 StartColor;
    glm::vec3 EndPosition;
    glm::vec4 EndColor;
};

struct SGpuMesh {
    std::string_view Name;
    uint32_t VertexPositionBuffer;
    uint32_t VertexNormalUvTangentBuffer;
    uint32_t IndexBuffer;

    size_t VertexCount;
    size_t IndexCount;

    glm::mat4 InitialTransform;
};

struct SGlobalLight {

    float Azimuth;
    float Elevation;
    glm::vec3 Color;
    float Strength;

    int32_t ShadowMapSize;
    float ShadowMapNear;
    float ShadowMapFar;

    bool ShowFrustum;
};

struct SGpuObject {
    glm::mat4x4 WorldMatrix;
    glm::ivec4 InstanceParameter;
};

struct SMeshInstance {
    glm::mat4 WorldMatrix;
};

struct SCamera {

    glm::vec3 Position = {0.0f, 0.0f, 5.0f};
    float Pitch = {};
    float Yaw = {glm::radians(-90.0f)};

    auto GetForwardDirection() -> const glm::vec3
    {
        return glm::vec3{cos(Pitch) * cos(Yaw), sin(Pitch), cos(Pitch) * sin(Yaw)};
    }

    auto GetViewMatrix() -> const glm::mat4
    {
        return glm::lookAt(Position, Position + GetForwardDirection(), glm::vec3(0, 1, 0));
    }
};

// - Io -----------------------------------------------------------------------

auto ReadBinaryFromFile(const std::filesystem::path& filePath) -> std::pair<std::unique_ptr<std::byte[]>, std::size_t> {
    std::size_t fileSize = std::filesystem::file_size(filePath);
    auto memory = std::make_unique<std::byte[]>(fileSize);
    std::ifstream file{filePath, std::ifstream::binary};
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), reinterpret_cast<char*>(memory.get()));
    return {std::move(memory), fileSize};
}

// - Assets -------------------------------------------------------------------

using SAssetMeshId = SId<struct TAssetMeshId>;
using SAssetImageId = SId<struct TAssetImageId>;
using SAssetTextureId = SId<struct TAssetTextureId>;
using SAssetSamplerId = SId<struct TAssetSamplerId>;
using SAssetMaterialId = SId<struct TAssetMaterialId>;

enum class EMaterialChannelUsage {
    SRgb, // use sRGB Format
    Normal, // use RG-format or bc7 when compressed
    MetalnessRoughness // single channelism or also RG?
};

struct SAssetMesh {
    std::string_view Name;
    glm::mat4 InitialTransform;
    std::vector<SGpuVertexPosition> VertexPositions;
    std::vector<SGpuVertexNormalUvTangent> VertexNormalUvTangents;
    std::vector<uint32_t> Indices;
};

struct SAssetSampler {
    ETextureAddressMode AddressModeU = ETextureAddressMode::ClampToEdge;
    ETextureAddressMode AddressModeV = ETextureAddressMode::ClampToEdge;
    ETextureMagFilter MagFilter = ETextureMagFilter::Linear;
    ETextureMinFilter MinFilter = ETextureMinFilter::Linear;
};

struct SAssetImage {

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

struct SAssetTexture {
    std::string ImageName;
    std::string SamplerName;
};

struct SAssetMaterialChannel {
    EMaterialChannelUsage Usage;
    std::string Image;
    std::string Sampler; 
};

struct SAssetMaterial {
    glm::vec4 BaseColorFactor;
    float MetallicFactor;
    glm::vec3 EmissiveFactor;
    float EmissiveStrength;
    std::optional<SAssetMaterialChannel> BaseColorChannel;
    std::optional<SAssetMaterialChannel> NormalsChannel;
    std::optional<SAssetMaterialChannel> OcclusionRoughnessMetallnessChannel;
    std::optional<SAssetMaterialChannel> EmissiveChannel;
};

std::unordered_map<std::string, SAssetImage> g_assetImages = {};
std::unordered_map<std::string, SAssetSampler> g_assetSamplers = {};
std::unordered_map<std::string, SAssetTexture> g_assetTextures = {};
std::unordered_map<std::string, SAssetMaterial> g_assetMaterials = {};
std::unordered_map<std::string, SAssetMesh> g_assetMeshes = {};
std::unordered_map<std::string, std::vector<std::string>> g_assetModelMeshes = {};

auto GetSafeResourceName(
    const char* const baseName,
    const char* const text,
    const char* const resourceType,
    const std::size_t resourceIndex) -> std::string {

    return (text == nullptr) || strlen(text) == 0
        ? std::format("{}-{}-{}", baseName, resourceType, resourceIndex)
        : std::format("{}-{}", baseName, text);
}

auto AddAssetMesh(
    const std::string& assetMeshName,
    const SAssetMesh& assetMesh) -> void {

    assert(!assetMeshName.empty());
    ZoneScopedN("AddAssetMesh");

    g_assetMeshes[assetMeshName] = assetMesh;
}

auto AddAssetImage(
    const std::string& assetImageName,
    SAssetImage&& assetImage) -> void {

    assert(!assetImageName.empty());
    ZoneScopedN("AddAssetImage");

    g_assetImages[assetImageName] = std::move(assetImage);
}

auto AddAssetMaterial(
    const std::string& assetMaterialName,
    const SAssetMaterial& assetMaterial) -> void {

    assert(!assetMaterialName.empty());
    ZoneScopedN("AddAssetMaterial");

    g_assetMaterials[assetMaterialName] = assetMaterial;
}

auto AddAssetTexture(
    const std::string& assetTextureName,
    const SAssetTexture& assetTexture) -> void {

    assert(!assetTextureName.empty());
    ZoneScopedN("AddAssetTexture");

    g_assetTextures[assetTextureName] = assetTexture;
}

auto AddAssetSampler(
    const std::string& assetSamplerName,
    const SAssetSampler& assetSampler) -> void {

    assert(!assetSamplerName.empty());
    ZoneScopedN("AddAssetSampler");

    g_assetSamplers[assetSamplerName] = assetSampler;
}

auto GetAssetImage(const std::string& assetImageName) -> SAssetImage& {

    assert(!assetImageName.empty() || g_assetImages.contains(assetImageName));
    ZoneScopedN("GetAssetImage");

    return g_assetImages.at(assetImageName);
}

auto GetAssetMesh(const std::string& assetMeshName) -> SAssetMesh& {

    assert(!assetMeshName.empty() || g_assetMeshes.contains(assetMeshName));
    ZoneScopedN("GetAssetMesh");

    return g_assetMeshes.at(assetMeshName);
}

auto GetAssetMaterial(const std::string& assetMaterialName) -> SAssetMaterial& {

    assert(!assetMaterialName.empty() || g_assetMaterials.contains(assetMaterialName));
    ZoneScopedN("GetAssetMaterial");

    return g_assetMaterials.at(assetMaterialName);
}

auto GetAssetModelMeshNames(const std::string& modelName) -> std::vector<std::string>& {

    assert(!modelName.empty() || g_assetModelMeshes.contains(modelName));
    ZoneScopedN("GetAssetModelMeshNames");

    return g_assetModelMeshes.at(modelName);
}

auto GetAssetSampler(const std::string& assetSamplerName) -> SAssetSampler& {

    assert(!assetSamplerName.empty() || g_assetSamplers.contains(assetSamplerName));
    ZoneScopedN("GetAssetSampler");

    return g_assetSamplers.at(assetSamplerName);
}

/*
auto GetAssetSampler(const SAssetSamplerId& assetSamplerId) -> SSamplerDescriptor {

    ZoneScopedN("GetAssetSampler");
    for (const auto& [key, value] : g_assetSamplers) {
        if (value == assetSamplerId) {
            return key;
        }
    }

    spdlog::error("Unable to find Asset Sampler");
    return SSamplerDescriptor {};
}
*/

auto GetAssetTexture(const std::string& assetTextureName) -> SAssetTexture& {

    ZoneScopedN("GetAssetTexture");
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
    else if (auto* mat = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform))
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
        ? ((1.0f - glm::abs(glm::vec2{encodedNormal.y, encodedNormal.x})) * SignNotZero(encodedNormal))
        : encodedNormal;
}

auto GetVertices(
    const fastgltf::Asset& asset, 
    const fastgltf::Primitive& primitive) -> std::pair<std::vector<SGpuVertexPosition>, std::vector<SGpuVertexNormalUvTangent>> {

    std::vector<glm::vec3> positions;
    auto& positionAccessor = asset.accessors[primitive.findAttribute("POSITION")->second];
    positions.resize(positionAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  positionAccessor,
                                                  [&](glm::vec3 position, std::size_t index) { positions[index] = position; });

    std::vector<glm::vec3> normals;
    auto& normalAccessor = asset.accessors[primitive.findAttribute("NORMAL")->second];
    normals.resize(normalAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  normalAccessor,
                                                  [&](glm::vec3 normal, std::size_t index) { normals[index] = normal; });

    std::vector<glm::vec4> tangents;
    auto& tangentAccessor = asset.accessors[primitive.findAttribute("TANGENT")->second];
    tangents.resize(tangentAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                  tangentAccessor,
                                                  [&](glm::vec4 tangent, std::size_t index) { tangents[index] = tangent; });

    std::vector<glm::vec3> uvs;
    if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
    {
        auto& uvAccessor = asset.accessors[primitive.findAttribute("TEXCOORD_0")->second];
        uvs.resize(uvAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                      uvAccessor,
                                                      [&](glm::vec2 uv, std::size_t index) { uvs[index] = glm::vec3{uv, 0.0f}; });
    }
    else
    {
        uvs.resize(positions.size(), {});
    }

    std::vector<SGpuVertexPosition> verticesPosition;
    std::vector<SGpuVertexNormalUvTangent> verticesNormalUvTangent;
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
            uvs[i]
        };
    }

    return {verticesPosition, verticesNormalUvTangent};
}

auto GetIndices(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive) -> std::vector<uint32_t> {

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
    std::string_view name,
    glm::mat4 initialTransform,
    const std::pair<std::vector<SGpuVertexPosition>, std::vector<SGpuVertexNormalUvTangent>>& vertices,
    const std::vector<uint32_t> indices) -> SAssetMesh {

    return SAssetMesh{
        .Name = name,
        .InitialTransform = initialTransform,
        .VertexPositions = std::move(vertices.first),
        .VertexNormalUvTangents = std::move(vertices.second),
        .Indices = std::move(indices)
    };
}

auto CreateAssetImage(
    const void* data, 
    std::size_t dataSize, 
    fastgltf::MimeType mimeType,
    std::string_view name) -> SAssetImage {

    auto dataCopy = std::make_unique<std::byte[]>(dataSize);
    std::copy_n(static_cast<const std::byte*>(data), dataSize, dataCopy.get());

    return SAssetImage {
        .Name = std::string(name),
        .EncodedData = std::move(dataCopy),
        .EncodedDataSize = dataSize,
    };
}

auto CreateAssetMaterial(
    const std::string& modelName,
    const fastgltf::Asset& asset,
    const fastgltf::Material& fgMaterial) -> void {

    SAssetMaterial assetMaterial = {};
    assetMaterial.BaseColorFactor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
    assetMaterial.MetallicFactor = fgMaterial.pbrData.metallicFactor;
    assetMaterial.EmissiveFactor = glm::make_vec3(fgMaterial.emissiveFactor.data());
    assetMaterial.EmissiveStrength = fgMaterial.emissiveStrength;

    if (fgMaterial.pbrData.baseColorTexture.has_value()) {
        auto& fgBaseColorTexture = fgMaterial.pbrData.baseColorTexture.value();
        auto& fgTexture = asset.textures[fgBaseColorTexture.textureIndex];
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), nullptr, "image", g_assetImages.size() + fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", g_assetSamplers.size() + fgTexture.samplerIndex.value_or(0));        

        assetMaterial.BaseColorChannel = SAssetMaterialChannel{
            .Usage = EMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.normalTexture.has_value()) {
        auto& fgNormalTexture = fgMaterial.normalTexture.value();
        auto& fgTexture = asset.textures[fgNormalTexture.textureIndex];
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), nullptr, "image", g_assetImages.size() + fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", g_assetSamplers.size() + fgTexture.samplerIndex.value_or(0));        

        assetMaterial.BaseColorChannel = SAssetMaterialChannel{
            .Usage = EMaterialChannelUsage::Normal,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.pbrData.metallicRoughnessTexture.has_value()) {
        auto& fgMetallicRoughnessTexture = fgMaterial.pbrData.metallicRoughnessTexture.value();
        auto& fgTexture = asset.textures[fgMetallicRoughnessTexture.textureIndex];
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), nullptr, "image", g_assetImages.size() + fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", g_assetSamplers.size() + fgTexture.samplerIndex.value_or(0));        

        assetMaterial.OcclusionRoughnessMetallnessChannel = SAssetMaterialChannel{
            .Usage = EMaterialChannelUsage::MetalnessRoughness,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.emissiveTexture.has_value()) {
        auto& fgEmissiveTexture = fgMaterial.emissiveTexture.value();
        auto& fgTexture = asset.textures[fgEmissiveTexture.textureIndex];
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), nullptr, "image", g_assetImages.size() + fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", g_assetSamplers.size() + fgTexture.samplerIndex.value_or(0));        

        assetMaterial.BaseColorChannel = SAssetMaterialChannel{
            .Usage = EMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }
    
    auto materialName = std::string(fgMaterial.name.data());
    return AddAssetMaterial(materialName, assetMaterial);
}

auto LoadModelFromFile(
    std::string modelName,
    std::filesystem::path filePath) -> void {

    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization);

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
        spdlog::error("fastgltf: Failed to load glTF: {}", fastgltf::getErrorMessage(assetResult.error()));
        return;
    }

    auto& fgAsset = assetResult.get();

    // images

    auto assetImageIds = std::vector<SAssetImageId>(fgAsset.images.size());
    const auto assetImageIndices = std::ranges::iota_view{(size_t)0, fgAsset.images.size()};

    std::transform(
        poolstl::execution::par,
        assetImageIndices.begin(),
        assetImageIndices.end(),
        assetImageIds.begin(),
        [&](size_t imageIndex) {

        ZoneScopedN("LoadModelFromFile - Load Image");
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
            
            return SAssetImage{};
        }();

        auto assetImageId = g_assetImages.size() + imageIndex;

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
        return static_cast<SAssetImageId>(assetImageId);
    });

    // samplers

    auto samplerIds = std::vector<SAssetSamplerId>(fgAsset.samplers.size());
    const auto samplerIndices = std::ranges::iota_view{(size_t)0, fgAsset.samplers.size()};
    std::transform(poolstl::execution::par, samplerIndices.begin(), samplerIndices.end(), samplerIds.begin(), [&](size_t samplerIndex) {

        ZoneScopedN("LoadModelFromFile - Load Sampler");
        const fastgltf::Sampler& fgSampler = fgAsset.samplers[samplerIndex];

        const auto getAddressMode = [](const fastgltf::Wrap& wrap) {
            switch (wrap) {
                case fastgltf::Wrap::ClampToEdge: return ETextureAddressMode::ClampToEdge;
                case fastgltf::Wrap::MirroredRepeat: return ETextureAddressMode::RepeatMirrored;
                case fastgltf::Wrap::Repeat: return ETextureAddressMode::Repeat;
                default:
                    return ETextureAddressMode::ClampToEdge;
            }
        };

        //TODO(deccer) reinvent min/mag filterisms, this sucks here
        const auto getMinFilter = [](const fastgltf::Filter& minFilter) {
            switch (minFilter) {
                case fastgltf::Filter::Linear: return ETextureMinFilter::Linear;
                case fastgltf::Filter::LinearMipMapLinear: return ETextureMinFilter::LinearMipmapLinear;
                case fastgltf::Filter::LinearMipMapNearest: return ETextureMinFilter::LinearMipmapNearest;
                case fastgltf::Filter::Nearest: return ETextureMinFilter::Nearest;
                case fastgltf::Filter::NearestMipMapLinear: return ETextureMinFilter::NearestMipmapLinear;
                case fastgltf::Filter::NearestMipMapNearest: return ETextureMinFilter::NearestMipmapNearest;
                default: std::unreachable();
            }
        };

        const auto getMagFilter = [](const fastgltf::Filter& magFilter) {
            switch (magFilter) {
                case fastgltf::Filter::Linear: return ETextureMagFilter::Linear;
                case fastgltf::Filter::Nearest: return ETextureMagFilter::Nearest;
                default: std::unreachable();
            }
        };

        auto assetSampler = SAssetSampler {
            .AddressModeU = getAddressMode(fgSampler.wrapS),
            .AddressModeV = getAddressMode(fgSampler.wrapT),
            .MagFilter = getMagFilter(fgSampler.magFilter.has_value() ? *fgSampler.magFilter : fastgltf::Filter::Nearest),
            .MinFilter = getMinFilter(fgSampler.minFilter.has_value() ? *fgSampler.minFilter : fastgltf::Filter::Nearest),
        };

        auto assetSamplerId = g_assetSamplers.size() + samplerIndex;
        AddAssetSampler(GetSafeResourceName(modelName.data(), fgSampler.name.data(), "sampler", assetSamplerId), assetSampler);
        return static_cast<SAssetSamplerId>(assetSamplerId);
    });

    // textures

    auto assetTextures = std::vector<SAssetTextureId>(fgAsset.textures.size());
    const auto assetTextureIndices = std::ranges::iota_view{(size_t)0, fgAsset.textures.size()};
    for (auto assetTextureIndex : assetTextureIndices) {

        auto& fgTexture = fgAsset.textures[assetTextureIndex];
        auto textureName = GetSafeResourceName(modelName.data(), fgTexture.name.data(), "texture", g_assetTextures.size() + assetTextureIndex);

        AddAssetTexture(textureName, SAssetTexture{
            .ImageName = GetSafeResourceName(modelName.data(), nullptr, "image", g_assetImages.size() + fgTexture.imageIndex.value_or(0)),
            .SamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", g_assetSamplers.size() + fgTexture.samplerIndex.value_or(0))
        });
    }

    // materials

    auto assetMaterialIds = std::vector<SAssetMaterialId>(fgAsset.materials.size());
    const auto assetMaterialIndices = std::ranges::iota_view{(size_t)0, fgAsset.materials.size()};
    for (auto assetMaterialIndex : assetMaterialIndices) {

        auto& fgMaterial = fgAsset.materials[assetMaterialIndex];
        auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", g_assetMaterials.size() + assetMaterialIndex);

        CreateAssetMaterial(modelName, fgAsset, fgMaterial);
    }

    std::stack<std::pair<const fastgltf::Node*, glm::mat4>> nodeStack;
    glm::mat4 rootTransform = glm::mat4(1.0f);

    for (auto nodeIndex : fgAsset.scenes[0].nodeIndices)
    {
        nodeStack.emplace(&fgAsset.nodes[nodeIndex], rootTransform);
    }
   
    while (!nodeStack.empty())
    {
        ZoneScopedN("LoadModelFromFile - Load Primitive");

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
            auto assetMesh = CreateAssetMesh(meshName, globalTransform, std::move(vertices), std::move(indices));
            AddAssetMesh(meshName, assetMesh);
            g_assetModelMeshes[modelName].push_back(meshName);

            primitiveIndex++;
        }
    }
}

// - Game ---------------------------------------------------------------------

SCamera g_mainCamera = {};
float g_cameraSpeed = 4.0f;

std::vector<SGlobalLight> g_globalLights;
bool g_globalLightsWasModified = true;

entt::registry g_gameRegistry = {};

// - Scene --------------------------------------------------------------------

struct SComponentWorldMatrix : public glm::mat4x4
{
    using glm::mat4x4::mat4x4;
    using glm::mat4x4::operator=;
    SComponentWorldMatrix() = default;
    SComponentWorldMatrix(glm::mat4x4 const& g) : glm::mat4x4(g) {}
    SComponentWorldMatrix(glm::mat4x4&& g) : glm::mat4x4(std::move(g)) {}
};

struct SComponentParent {
    std::vector<entt::entity> Children;
};

struct SComponentChildOf {
    entt::entity Parent;
};

struct SComponentMesh {
    SAssetMeshId Mesh;
};

struct SComponentMaterial {
    SAssetMaterialId Material;
};

auto AddEntity(
    std::optional<entt::entity> parent,
    SAssetMeshId assetMeshId,
    SAssetMaterialId assetMaterialId,
    SComponentWorldMatrix initialTransform) -> entt::entity {

    auto entityId = g_gameRegistry.create();
    if (parent.has_value()) {
        auto& parentComponent = g_gameRegistry.get<SComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);

        g_gameRegistry.emplace<SComponentChildOf>(entityId, parent.value());
    }
    g_gameRegistry.emplace<SAssetMeshId>(entityId, assetMeshId);
    g_gameRegistry.emplace<SAssetMaterialId>(entityId, assetMaterialId);    
    g_gameRegistry.emplace<SComponentWorldMatrix>(entityId, initialTransform);

    return entityId;
}

// - Renderer -----------------------------------------------------------------

SFramebuffer g_geometryFramebuffer = {};
SGraphicsPipeline g_geometryGraphicsPipeline = {};
std::vector<SGpuGlobalLight> g_gpuGlobalLights;

bool g_drawDebugLines = true;
std::vector<SDebugLine> g_debugLines;
uint32_t g_debugInputLayout = 0;
uint32_t g_debugLinesVertexBuffer = 0;
SGraphicsPipeline g_debugLinesGraphicsPipeline = {};

SGraphicsPipeline g_fullscreenTriangleGraphicsPipeline = {};
SSampler g_fullscreenSamplerNearestClampToEdge = {};

auto DrawFullscreenTriangleWithTexture(const STexture& texture) -> void {
    
    g_fullscreenTriangleGraphicsPipeline.Bind();
    g_fullscreenTriangleGraphicsPipeline.BindTextureAndSampler(0, texture.Id, g_fullscreenSamplerNearestClampToEdge.Id);
    g_fullscreenTriangleGraphicsPipeline.DrawArrays(0, 3);
}

auto CreateGpuMesh(const std::string& assetMeshName) -> SGpuMesh {

    auto& assetMesh = GetAssetMesh(assetMeshName);

    uint32_t buffers[3] = {};
    glCreateBuffers(3, buffers);

    glNamedBufferStorage(buffers[0], sizeof(SGpuVertexPosition) * assetMesh.VertexPositions.size(), assetMesh.VertexPositions.data(), 0);
    glNamedBufferStorage(buffers[1], sizeof(SGpuVertexNormalUvTangent) * assetMesh.VertexNormalUvTangents.size(), assetMesh.VertexNormalUvTangents.data(), 0);
    glNamedBufferStorage(buffers[2], sizeof(uint32_t) * assetMesh.Indices.size(), assetMesh.Indices.data(), 0);

    return {
        .Name = assetMesh.Name,
        .VertexPositionBuffer = buffers[0],
        .VertexNormalUvTangentBuffer = buffers[1],
        .IndexBuffer = buffers[2],

        .VertexCount = assetMesh.VertexPositions.size(),
        .IndexCount = assetMesh.Indices.size(),

        .InitialTransform = assetMesh.InitialTransform,
    };
}

/*
auto CreateAndUploadGpuTexture(const SAssetTextureId& assetTextureId) -> STextureId {

    auto& assetTexture = g_assetTextureLibrary.GetAssetTexture(assetTextureId);
    auto& assetImage = g_assetImageLibrary.GetAssetImage(assetTexture.ImageId)

    auto textureId = CreateTexture(SCreateTextureDescriptor{
        .TextureType = ETextureType::Texture2D,
        .Format = EFormat::R8G8B8A8_UNORM,                
        .Extent = SExtent3D{static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageData.Width), static_cast<float>(imageData.Height)))) + 1),
        .Layers = 1,
        .SampleCount = ESampleCount::One,
        .Label = std::string(fgAsset.textures[textureIndex].name),
    });
    
    UploadTexture(textureId, SUploadTextureDescriptor{
        .Level = 0,
        .Offset = SOffset3D{0, 0, 0},
        .Extent = SExtent3D{static_cast<uint32_t>(imageData.Width), static_cast<uint32_t>(imageData.Height), 1u},
        .UploadFormat = EUploadFormat::Auto,
        .UploadType = EUploadType::Auto,
        .PixelData = imageData.EncodedData.get()
    });

    return textureId;
}
*/

// - Application --------------------------------------------------------------

const glm::vec3 g_unitX = glm::vec3{1.0f, 0.0f, 0.0f};
const glm::vec3 g_unitY = glm::vec3{0.0f, 1.0f, 0.0f};
const glm::vec3 g_unitZ = glm::vec3{0.0f, 0.0f, 1.0f};

constexpr ImVec2 g_imvec2UnitX = ImVec2(1, 0);
constexpr ImVec2 g_imvec2UnitY = ImVec2(0, 1);

GLFWwindow* g_window = nullptr;
ImGuiContext* g_imguiContext = nullptr;

bool g_isEditor = false;
bool g_sleepWhenWindowHasNoFocus = true;
bool g_windowHasFocus = false;
bool g_windowFramebufferResized = false;
bool g_sceneViewerResized = false;
bool g_cursorJustEntered = false;
bool g_cursorIsActive = true;
float g_cursorSensitivity = 0.0025f;

glm::dvec2 g_cursorPosition = {};
glm::dvec2 g_cursorFrameOffset = {};

glm::ivec2 g_windowFramebufferSize = {};
glm::ivec2 g_windowFramebufferScaledSize = {};
glm::ivec2 g_sceneViewerSize = {};
glm::ivec2 g_sceneViewerScaledSize = {};

// - Implementation -----------------------------------------------------------

auto OnWindowKey(
    GLFWwindow* window,
    const int32_t key,
    [[maybe_unused]] int32_t scancode,
    const int32_t action,
    [[maybe_unused]] int32_t mods) -> void {

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        g_isEditor = !g_isEditor;
        g_windowFramebufferResized = !g_isEditor;
        g_sceneViewerResized = g_isEditor;
    }
}

auto OnWindowCursorEntered(
    [[maybe_unused]] GLFWwindow* window,
    int entered) -> void {

    if (entered) {
        g_cursorJustEntered = true;
    }

    g_windowHasFocus = entered == GLFW_TRUE;
}

auto OnWindowCursorPosition(
    [[maybe_unused]] GLFWwindow* window,
    double cursorPositionX,
    double cursorPositionY) -> void {

    if (g_cursorJustEntered)
    {
        g_cursorPosition = {cursorPositionX, cursorPositionY};
        g_cursorJustEntered = false;
    }        

    g_cursorFrameOffset += glm::dvec2{
        cursorPositionX - g_cursorPosition.x, 
        g_cursorPosition.y - cursorPositionY};
    g_cursorPosition = {cursorPositionX, cursorPositionY};
}

auto OnWindowFramebufferSizeChanged(
    [[maybe_unused]] GLFWwindow* window,
    const int width,
    const int height) -> void {

    g_windowFramebufferSize = glm::ivec2{width, height};
    g_windowFramebufferResized = true;
}

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

auto HandleCamera(float deltaTime) -> void {

    g_cursorIsActive = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE;
    glfwSetInputMode(g_window, GLFW_CURSOR, g_cursorIsActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (!g_cursorIsActive) {
        glfwSetCursorPos(g_window, 0, 0);
        g_cursorPosition.x = 0;
        g_cursorPosition.y = 0;
    }

    const glm::vec3 forward = g_mainCamera.GetForwardDirection();
    const glm::vec3 right = glm::normalize(glm::cross(forward, g_unitY));

    float tempCameraSpeed = g_cameraSpeed;
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        tempCameraSpeed *= 4.0f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        tempCameraSpeed *= 40.0f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        tempCameraSpeed *= 0.25f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) {
        g_mainCamera.Position += forward * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) {
        g_mainCamera.Position -= forward * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) {
        g_mainCamera.Position += right * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) {
        g_mainCamera.Position -= right * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS) {
        g_mainCamera.Position.y -= deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS) {
        g_mainCamera.Position.y += deltaTime * tempCameraSpeed;
    }

    if (!g_cursorIsActive) {

        g_mainCamera.Yaw += static_cast<float>(g_cursorFrameOffset.x * g_cursorSensitivity);
        g_mainCamera.Pitch += static_cast<float>(g_cursorFrameOffset.y * g_cursorSensitivity);
        g_mainCamera.Pitch = glm::clamp(g_mainCamera.Pitch, -glm::half_pi<float>() + 1e-4f, glm::half_pi<float>() - 1e-4f);    
    }

    g_cursorFrameOffset = {0.0, 0.0};
}

auto DeleteRendererFramebuffers() -> void {

    DeleteFramebuffer(g_geometryFramebuffer);
}

auto CreateRendererFramebuffers(const glm::vec2& scaledFramebufferSize) -> void {

    g_geometryFramebuffer = CreateFramebuffer({
        .Label = "Geometry",
        .ColorAttachments = {
            SFramebufferColorAttachmentDescriptor{
                .Label = "GeometryAlbedo",
                .Format = EFormat::R8G8B8A8_SRGB,
                .Extent = SExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = EFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            SFramebufferColorAttachmentDescriptor{
                .Label = "GeometryNormals",
                .Format = EFormat::R32G32B32A32_FLOAT,
                .Extent = SExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = EFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },                        
            },
        },
        .DepthStencilAttachment = SFramebufferDepthStencilAttachmentDescriptor{
            .Label = "GeometryDepth",
            .Format = EFormat::D24_UNORM_S8_UINT,
            .Extent = SExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
            .LoadOperation = EFramebufferAttachmentLoadOperation::Clear,
            .ClearDepthStencil = { 1.0f, 0 },
        }
    });
}

auto main(
    [[maybe_unused]] int32_t argc,
    [[maybe_unused]] char* argv[]) -> int32_t {

    SWindowSettings windowSettings = {
        .ResolutionWidth = 1920,
        .ResolutionHeight = 1080,
        .ResolutionScale = 1.0f,
        .WindowStyle = EWindowStyle::Windowed,
        .IsDebug = true,
        .IsVSyncEnabled = true,
    };

    if (glfwInit() == GLFW_FALSE) {
        spdlog::error("Glfw: Unable to initialize");
        return 0;
    }

    const auto isWindowWindowed = windowSettings.WindowStyle == EWindowStyle::Windowed;
    glfwWindowHint(GLFW_DECORATED, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    const auto primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        spdlog::error("Glfw: Unable to get the primary monitor");
        return 0;
    }

    const auto primaryMonitorVideoMode = glfwGetVideoMode(primaryMonitor);
    const auto screenWidth = primaryMonitorVideoMode->width;
    const auto screenHeight = primaryMonitorVideoMode->height;

    const auto windowWidth = windowSettings.ResolutionWidth;
    const auto windowHeight = windowSettings.ResolutionHeight;

    auto monitor = windowSettings.WindowStyle == EWindowStyle::FullscreenExclusive
        ? primaryMonitor
        : nullptr;

    g_window = glfwCreateWindow(windowWidth, windowHeight, "How To Script", monitor, nullptr);
    if (g_window == nullptr) {
        spdlog::error("Glfw: Unable to create a window");
        return 0;
    }

    int32_t monitorLeft = 0;
    int32_t monitorTop = 0;
    glfwGetMonitorPos(primaryMonitor, &monitorLeft, &monitorTop);
    if (isWindowWindowed) {
        glfwSetWindowPos(g_window, screenWidth / 2 - windowWidth / 2 + monitorLeft, screenHeight / 2 - windowHeight / 2 + monitorTop);
    } else {
        glfwSetWindowPos(g_window, monitorLeft, monitorTop);
    }

    glfwSetKeyCallback(g_window, OnWindowKey);
    glfwSetCursorPosCallback(g_window, OnWindowCursorPosition);
    glfwSetCursorEnterCallback(g_window, OnWindowCursorEntered);    
    glfwSetFramebufferSizeCallback(g_window, OnWindowFramebufferSizeChanged);
    glfwSetInputMode(g_window, GLFW_CURSOR, g_cursorIsActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    int32_t windowFramebufferWidth = 0;
    int32_t windowFramebufferHeight = 0;
    glfwGetFramebufferSize(g_window, &windowFramebufferWidth, &windowFramebufferHeight);
    OnWindowFramebufferSizeChanged(g_window, windowFramebufferWidth, windowFramebufferHeight);

    glfwMakeContextCurrent(g_window);
    if (!InitializeRhi()) {
        spdlog::error("Glad: Unable to initialize");
        return 0;
    }

    if (windowSettings.IsDebug) {
        glDebugMessageCallback(OnOpenGLDebugMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

    TracyGpuContext;

    g_imguiContext = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // this little shit doesn't do anything
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (!ImGui_ImplGlfw_InitForOpenGL(g_window, true)) {
        spdlog::error("ImGui: Unable to initialize the GLFW backend");
        return 0;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        spdlog::error("ImGui: Unable to initialize the OpenGL backend");
        return 0;
    }

    if (windowSettings.IsVSyncEnabled) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.05f, 0.07f, 1.0f);

    auto isSrgbDisabled = false;
    auto isCullfaceDisabled = false;

    g_sceneViewerSize = g_windowFramebufferSize;
    auto scaledFramebufferSize = glm::vec2(g_sceneViewerSize) * windowSettings.ResolutionScale;

    // - Initialize Renderer ///////////// 

    CreateRendererFramebuffers(scaledFramebufferSize);

    auto geometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "GeometryPipeline",
        .VertexShaderFilePath = "data/shaders/Simple.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/Simple.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = EPrimitiveTopology::Triangles
        },
    });
    if (!geometryGraphicsPipelineResult) {
        spdlog::error(geometryGraphicsPipelineResult.error());
        return -1;
    }
    g_geometryGraphicsPipeline = *geometryGraphicsPipelineResult;

    auto fullscreenTriangleGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FullscreenTrianglePipeline",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.GammaCorrected.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = EPrimitiveTopology::Triangles
        },
    });

    if (!fullscreenTriangleGraphicsPipelineResult) {
        spdlog::error(fullscreenTriangleGraphicsPipelineResult.error());
        return 0;
    }
    g_fullscreenTriangleGraphicsPipeline = *fullscreenTriangleGraphicsPipelineResult;

    auto debugLinesGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "DebugLinesPipeline",
        .VertexShaderFilePath = "data/shaders/DebugLines.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/DebugLines.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = EPrimitiveTopology::Lines,
            .IsPrimitiveRestartEnabled = false,
        },
        .VertexInput = SVertexInputDescriptor{
            .VertexInputAttributes = {
                SVertexInputAttributeDescriptor{
                    .Location = 0,
                    .Binding = 0,
                    .Format = EFormat::R32G32B32_FLOAT,
                    .Offset = offsetof(SDebugLine, StartPosition),
                },
                SVertexInputAttributeDescriptor{
                    .Location = 1,
                    .Binding = 0,
                    .Format = EFormat::R32G32B32A32_FLOAT,
                    .Offset = offsetof(SDebugLine, StartColor),
                },
            }
        }
    });

    if (!debugLinesGraphicsPipelineResult) {
        spdlog::error(debugLinesGraphicsPipelineResult.error());
        return 0;
    }

    g_debugLinesGraphicsPipeline = *debugLinesGraphicsPipelineResult;
    g_debugLinesVertexBuffer = CreateBuffer("VertexBuffer-DebugLines", sizeof(SDebugLine) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    auto cameraPosition = glm::vec3{0.0f, 0.0f, 20.0f};
    auto cameraDirection = glm::vec3{0.0f, 0.0f, -1.0f};
    auto cameraUp = glm::vec3{0.0f, 1.0f, 0.0f};
    SGpuGlobalUniforms globalUniforms = {
        .ProjectionMatrix = glm::infinitePerspective(glm::radians(70.0f), scaledFramebufferSize.x / static_cast<float>(scaledFramebufferSize.y), 0.1f),
        .ViewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp),
        .CameraPosition = glm::vec4{cameraPosition, 0.0f}
    };
    auto globalUniformsBuffer = CreateBuffer("SGpuGlobalUniforms", sizeof(SGpuGlobalUniforms), &globalUniforms, GL_DYNAMIC_STORAGE_BIT);

    auto objectsBuffer = CreateBuffer("SGpuObjects", sizeof(SGpuObject) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    // - Load Assets ////////////

    LoadModelFromFile("Test", "data/default/SM_Deccer_Cubes_Textured.glb");
    //LoadModelFromFile("Test", "data/scenes/Tower/scene.gltf");
    
    //LoadModelFromFile("SM_DeccerCube", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube_1by1by1", "data/basic/SM_Cube_1by1by1.glb");
    
    //LoadModelFromFile("SM_Cube_1by1_x5", "data/basic/SM_Cube_1by1_x5.glb");
    //LoadModelFromFile("SM_Cube_1by1_y5", "data/basic/SM_Cube_1by1_y5.glb");
    //LoadModelFromFile("SM_Cube_1by1_z5", "data/basic/SM_Cube_1by1_z5.glb");
    //LoadModelFromFile("Test", "data/basic/Cubes/SM_Cube_1by1_5y.gltf");
    //LoadModelFromFile("Test", "data/basic/Cubes2/SM_Cube_1by1_5y.gltf");
    
    //LoadModelFromFile("Test", "data/scenes/IntelSponza/NewSponza_Main_glTF_002.gltf");

    uint32_t g_gridTexture;

    int32_t iconWidth = 0;
    int32_t iconHeight = 0;
    int32_t iconComponents = 0;

    auto gridTexturePixels = LoadImageFromFile("data/basic/T_Grid_1024_D.png", &iconWidth, &iconHeight, &iconComponents);
    if (gridTexturePixels != nullptr) {
        glCreateTextures(GL_TEXTURE_2D, 1, &g_gridTexture);
        auto mipLevels = static_cast<int32_t>(1 + glm::floor(glm::log2(glm::max(static_cast<float>(iconWidth), static_cast<float>(iconHeight)))));
        glTextureStorage2D(g_gridTexture, mipLevels, GL_SRGB8_ALPHA8, iconWidth, iconHeight);
        glTextureSubImage2D(g_gridTexture, 0, 0, 0, iconWidth, iconHeight, GL_RGBA, GL_UNSIGNED_BYTE, gridTexturePixels);
        glGenerateTextureMipmap(g_gridTexture);
        FreeImage(gridTexturePixels);
    }
    
    /// Setup Scene ////////////

    /// Start Renderloop ///////

    uint64_t frameCounter = 0;
    auto currentTimeInSeconds = glfwGetTime();    
    auto previousTimeInSeconds = glfwGetTime();
    auto accumulatedTimeInSeconds = 0.0;
    while (!glfwWindowShouldClose(g_window)) {

        ZoneScopedN("Frame");

        auto deltaTimeInSeconds = currentTimeInSeconds - previousTimeInSeconds;
        accumulatedTimeInSeconds += deltaTimeInSeconds;
        previousTimeInSeconds = currentTimeInSeconds;
        currentTimeInSeconds = glfwGetTime();

        // Update State
        HandleCamera(deltaTimeInSeconds);

        // Update Per Frame Uniforms

        globalUniforms.ProjectionMatrix = glm::infinitePerspective(glm::radians(60.0f), (float)scaledFramebufferSize.x / (float)scaledFramebufferSize.y, 0.1f);
        globalUniforms.ViewMatrix = g_mainCamera.GetViewMatrix();
        globalUniforms.CameraPosition = glm::vec4(g_mainCamera.Position, 0.0f);
        UpdateBuffer(globalUniformsBuffer, 0, sizeof(SGpuGlobalUniforms), &globalUniforms);

        ///////

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Resize if necessary
        if (g_windowFramebufferResized || g_sceneViewerResized) {

            g_windowFramebufferScaledSize = glm::ivec2{g_windowFramebufferSize.x * windowSettings.ResolutionScale, g_windowFramebufferSize.y * windowSettings.ResolutionScale};
            g_sceneViewerScaledSize = glm::ivec2{g_sceneViewerSize.x * windowSettings.ResolutionScale, g_sceneViewerSize.y * windowSettings.ResolutionScale};

            if (g_isEditor) {
                scaledFramebufferSize = g_sceneViewerScaledSize;
            } else {
                scaledFramebufferSize = g_windowFramebufferScaledSize;
            }

            DeleteRendererFramebuffers();
            CreateRendererFramebuffers(scaledFramebufferSize);

            if (g_isEditor) {
                glViewport(0, 0, g_sceneViewerScaledSize.x, g_sceneViewerScaledSize.y);
            } else {
                glViewport(0, 0, g_windowFramebufferScaledSize.x, g_windowFramebufferScaledSize.y);
            }
        }

        if (g_drawDebugLines) {
            g_debugLines.clear();

            g_debugLines.push_back(SDebugLine{
                .StartPosition = glm::vec3{-150, 30, 4},
                .StartColor = glm::vec4{0.3f, 0.95f, 0.1f, 1.0f},
                .EndPosition = glm::vec3{150, -40, -4},
                .EndColor = glm::vec4{0.6f, 0.1f, 0.0f, 1.0f}
            });
        }

        if (isSrgbDisabled) {
            glEnable(GL_FRAMEBUFFER_SRGB);
            isSrgbDisabled = false;
        }        

        g_geometryGraphicsPipeline.Bind();
        BindFramebuffer(g_geometryFramebuffer);

        g_geometryGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);

        auto& assetMeshIds = GetAssetModelMeshNames("Test");
        for(auto& assetMeshId : assetMeshIds) {

            auto mesh = CreateGpuMesh(assetMeshId);

            g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(mesh.VertexPositionBuffer, 1);
            g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(mesh.VertexNormalUvTangentBuffer, 2);
            g_geometryGraphicsPipeline.SetUniform(0, mesh.InitialTransform);
            g_geometryGraphicsPipeline.SetUniform(4, 0, 0, 0, 0);
            g_geometryGraphicsPipeline.DrawElements(mesh.IndexBuffer, mesh.IndexCount);
            glBindTextureUnit(1, g_gridTexture);
            glBindSampler(1, g_fullscreenSamplerNearestClampToEdge.Id);
        }
        
        /*
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBlitNamedFramebuffer(g_geometryFramebuffer.Id, 0, 
            0, 0, scaledFramebufferSize.x, scaledFramebufferSize.y, 
            0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);
        */

        /////////////// Debug Lines // move out to some LineRenderer

        if (g_drawDebugLines && !g_debugLines.empty()) {

            PushDebugGroup("Debug Lines");
            glDisable(GL_CULL_FACE);

            UpdateBuffer(g_debugLinesVertexBuffer, 0, sizeof(SDebugLine) * g_debugLines.size(), g_debugLines.data());

            g_debugLinesGraphicsPipeline.Bind();
            g_debugLinesGraphicsPipeline.BindBufferAsVertexBuffer(g_debugLinesVertexBuffer, 0, 0, sizeof(SDebugLine) / 2);
            g_debugLinesGraphicsPipeline.BindBufferAsUniformBuffer(0, globalUniformsBuffer);
            g_debugLinesGraphicsPipeline.DrawArrays(0, g_debugLines.size() * 2);
            
            glEnable(GL_CULL_FACE);
            PopDebugGroup();
        }

        /////////////// UI

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (!g_isEditor) {
            g_fullscreenTriangleGraphicsPipeline.Bind();
            DrawFullscreenTriangleWithTexture(g_geometryFramebuffer.ColorAttachments[0].value().Texture);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (g_isEditor) {
            ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
        }

        if (!g_isEditor) {
            ImGui::SetNextWindowPos({32, 32});
            ImGui::SetNextWindowSize({168, 152});
            auto windowBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            windowBackgroundColor.w = 0.4f;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
            if (ImGui::Begin("#InGameStatistics", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoDecoration)) {

                ImGui::TextColored(ImVec4{0.9f, 0.7f, 0.0f, 1.0f}, "F1 to toggle editor");
                ImGui::SeparatorText("Frame Statistics");

                auto framesPerSecond = 1.0f / deltaTimeInSeconds;
                ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * framesPerSecond);
                ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * framesPerSecond));
                ImGui::Text("rfps: %.0f", framesPerSecond);
                ImGui::Text("rpms: %.0f", framesPerSecond * 60.0f);
                ImGui::Text("  ft: %.2f ms", deltaTimeInSeconds * 1000.0f);
                ImGui::Text("   f: %.d", frameCounter);
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

            // Scene Viewer
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if (ImGui::Begin("Scene")) {
                auto availableSceneWindowSize = ImGui::GetContentRegionAvail();
                if (availableSceneWindowSize.x != g_sceneViewerSize.x || availableSceneWindowSize.y != g_sceneViewerSize.y) {
                    g_sceneViewerSize = glm::ivec2(availableSceneWindowSize.x, availableSceneWindowSize.y);
                    g_sceneViewerResized = true;
                }

                auto texture = g_geometryFramebuffer.ColorAttachments[0].value().Texture.Id;
                auto imagePosition = ImGui::GetCursorPos();
                ImGui::Image(reinterpret_cast<ImTextureID>(texture), availableSceneWindowSize, g_imvec2UnitY, g_imvec2UnitX);
                ImGui::SetCursorPos(imagePosition);
                if (ImGui::BeginChild(1, ImVec2{192, -1})) {
                    if (ImGui::CollapsingHeader("Statistics")) {
                        ImGui::Text("Text");
                    }
                }
                ImGui::EndChild();
            }
            ImGui::PopStyleVar();
            ImGui::End();
        }        

        {
            ZoneScopedN("UI");

            ImGui::Render();
            auto* imGuiDrawData = ImGui::GetDrawData();
            if (imGuiDrawData != nullptr) {
                glDisable(GL_FRAMEBUFFER_SRGB);
                isSrgbDisabled = true;
                //PushDebugGroup("UI");
                glViewport(0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y);
                ImGui_ImplOpenGL3_RenderDrawData(imGuiDrawData);
                //PopDebugGroup();            
            }
        }
        {
            ZoneScopedN("SwapBuffers");
            glfwSwapBuffers(g_window);
        }

        TracyGpuCollect;
        FrameMark;

        glfwPollEvents();

        frameCounter++;

        if (!g_windowHasFocus && g_sleepWhenWindowHasNoFocus) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    /// Cleanup Resources //////

    DeleteFramebuffer(g_geometryFramebuffer);
    DeletePipeline(g_geometryGraphicsPipeline);
    DeletePipeline(g_debugLinesGraphicsPipeline);

    UnloadRhi();

    return 0;
}