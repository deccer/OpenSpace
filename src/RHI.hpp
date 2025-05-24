#pragma once

using TTextureId = TId<struct GTextureId>;
using TSamplerId = TId<struct GSamplerId>;
using TFramebufferId = TId<struct GFramebufferId>;
using TBufferId = TId<struct GBufferId>;
using TGraphicsPipelineId = TId<struct GGraphicsPipelineId>;
using TComputePipelineId = TId<struct GComputePipelineId>;

enum class TFormat : uint32_t {

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

enum class TFormatClass {
    Float,
    Integer,
    Long
};

enum class TBaseTypeClass {
    Float,
    Integer,
    UnsignedInteger
};

enum class TTextureType : uint32_t {
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

enum class TSampleCount : uint32_t {
    One = 1,
    Two = 2,
    Four = 4,
    Eight = 8,
    SixTeen = 16,
    ThirtyTwo = 32,
};

enum class TUploadFormat : uint32_t
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

enum class TUploadType : uint32_t
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

enum class TAttachmentType : uint32_t {
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

enum class TTextureAddressMode {
    Repeat,
    RepeatMirrored,
    ClampToEdge,
    ClampToBorder,
    ClampToEdgeMirrored,
};

enum class TTextureMinFilter {
    Nearest,
    NearestMipmapLinear,
    NearestMipmapNearest,
    Linear,
    LinearMipmapLinear,
    LinearMipmapNearest,
};

enum class TTextureMagFilter {
    Nearest,
    Linear
};

enum class TPrimitiveTopology {
    Triangles,
    TriangleStrip,
    TriangleFan,
    Lines,
};

struct TCreateTextureDescriptor {
    TTextureType TextureType = {};
    TFormat Format = {};
    TExtent3D Extent = {};
    uint32_t MipMapLevels = 0;
    uint32_t Layers = 0;
    TSampleCount SampleCount = {};
    std::string Label = {};
};

struct TUploadTextureDescriptor {
    uint32_t Level;
    TOffset3D Offset;
    TExtent3D Extent;
    TUploadFormat UploadFormat = TUploadFormat::Auto;
    TUploadType UploadType = TUploadType::Auto;
    const void* PixelData = nullptr;
};

struct TTexture {
    uint32_t Id;
    TFormat Format;
    TExtent3D Extent;
    TTextureType TextureType;
};

enum class TMemoryAccess {
    ReadOnly,
    WriteOnly,
    ReadWrite
};

enum class TFramebufferAttachmentLoadOperation {
    Load,
    Clear,
    DontCare
};

struct TFramebufferAttachmentClearColor {
    TFramebufferAttachmentClearColor() = default;

    template<typename... Args>
    requires (sizeof...(Args) <= 4)
    TFramebufferAttachmentClearColor(const Args&... args)
      : Data(std::array<std::common_type_t<std::remove_cvref_t<Args>...>, 4>{ args...})
    {
    }

    std::variant<std::array<float, 4>, std::array<uint32_t, 4>, std::array<int32_t, 4>> Data;
};

struct TFramebufferAttachmentClearDepthStencil {
    float Depth = {};
    int32_t Stencil = {};
};

struct TFramebufferColorAttachmentDescriptor {
    std::string_view Label;
    TFormat Format;
    TExtent2D Extent;
    TFramebufferAttachmentLoadOperation LoadOperation;
    TFramebufferAttachmentClearColor ClearColor;
};

struct TFramebufferDepthStencilAttachmentDescriptor {
    std::string_view Label;
    TFormat Format;
    TExtent2D Extent;
    TFramebufferAttachmentLoadOperation LoadOperation;
    TFramebufferAttachmentClearDepthStencil ClearDepthStencil;
};

struct TFramebufferExistingDepthStencilAttachmentDescriptor {
    TTexture& ExistingDepthTexture;
};

struct TFramebufferDescriptor {
    std::string_view Label;
    std::array<std::optional<TFramebufferColorAttachmentDescriptor>, 8> ColorAttachments;
    std::optional<std::variant<TFramebufferDepthStencilAttachmentDescriptor, TFramebufferExistingDepthStencilAttachmentDescriptor>> DepthStencilAttachment;
};

struct TFramebufferColorAttachment {
    TTexture Texture;
    TFramebufferAttachmentClearColor ClearColor;
    TFramebufferAttachmentLoadOperation LoadOperation;
};

struct TFramebufferDepthStencilAttachment {
    TTexture Texture;
    TFramebufferAttachmentClearDepthStencil ClearDepthStencil;
    TFramebufferAttachmentLoadOperation LoadOperation;
};

struct TFramebuffer {
    uint32_t Id;
    std::array<std::optional<TFramebufferColorAttachment>, 8> ColorAttachments;
    std::optional<TFramebufferDepthStencilAttachment> DepthStencilAttachment;
};

struct TInputAssemblyDescriptor {
    TPrimitiveTopology PrimitiveTopology = TPrimitiveTopology::Triangles;
    bool IsPrimitiveRestartEnabled = false;
};

struct TVertexInputAttributeDescriptor {
    uint32_t Location;
    uint32_t Binding;
    TFormat Format;
    uint32_t Offset;
};

struct TVertexInputDescriptor {
    std::array<std::optional<const TVertexInputAttributeDescriptor>, 16> VertexInputAttributes = {};
};

enum class TFillMode {
    Solid,
    Wireframe,
    Points
};

enum class TCullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class TFaceWindingOrder {
    Clockwise,
    CounterClockwise
};

enum class TDepthFunction {
    Never,
    Less,
    Equal,
    LessThanOrEqual,
    Greater,
    NotEqual,
    GreaterThanOrEqual,
    Always,
};

struct TClipControl {

};

struct TRasterizerState {
    TFillMode FillMode;
    TCullMode CullMode;
    TFaceWindingOrder FaceWindingOrder;
    bool IsDepthBiasEnabled;
    float DepthBiasSlopeFactor;
    float DepthBiasConstantFactor;
    bool IsDepthClampEnabled;
    bool IsScissorEnabled;
    TClipControl ClipControl;
    bool IsRasterizerDisabled;
    float LineWidth = 1.0f;
    float PointSize = 1.0f;
};

struct TBlendState {
    bool IsBlendEnabled;
};

struct TDepthState {
    bool IsDepthTestEnabled;
    bool IsDepthWriteEnabled;
    TDepthFunction DepthFunction;
};

struct TStencilState {

};

enum class TColorMaskBits {
    None = 0,
    R = 1 << 0,
    G = 1 << 1,
    B = 1 << 2,
    A = 1 << 3
};
DECLARE_FLAG_TYPE(TColorMask, TColorMaskBits, uint32_t)

enum class TMemoryBarrierMaskBits : uint32_t
{
    VertexAttribArray = 1,
    ElementArray = 2,
    Uniform = 4,
    TextureFetch = 8,
    ShaderGlobalAccess = 16,
    ShaderImageAccess = 32,
    Command = 64,
    PixelBuffer = 128,
    TextureUpdate = 256,
    BufferUpdate = 512,
    Framebuffer = 1024,
    TransformFeedback = 2048,
    AtomicCounter = 4096,
    ShaderStorage = 8192,
    ClientMappedBuffer = 16384,
    QueryBuffer = 32768,
    All = 4294967295,
};
DECLARE_FLAG_TYPE(TMemoryBarrierMask, TMemoryBarrierMaskBits, uint32_t)

struct TOutputMergerState {
    TColorMask ColorMask = TColorMaskBits::R | TColorMaskBits::G | TColorMaskBits::B | TColorMaskBits::A;
    TBlendState BlendState = {};
    TDepthState DepthState = {};
    TStencilState StencilState = {};
};

struct TGraphicsPipelineDescriptor {
    std::string_view Label;
    std::string_view VertexShaderFilePath;
    std::string_view FragmentShaderFilePath;

    TInputAssemblyDescriptor InputAssembly = {};
    std::optional<TVertexInputDescriptor> VertexInput;

    TRasterizerState RasterizerState = {};
    TOutputMergerState OutputMergerState = {};
};

struct TComputePipelineDescriptor {
    std::string_view Label;
    std::string_view ComputeShaderFilePath;
};

struct TPipeline {
    uint32_t Id;

    virtual auto Bind() -> void;
    auto BindBufferAsUniformBuffer(
        uint32_t buffer,
        int32_t bindingIndex) const -> void;
    auto BindBufferAsUniformBuffer(
        uint32_t buffer,
        int32_t bindingIndex,
        int64_t offset,
        int64_t size) const -> void;
    auto BindBufferAsShaderStorageBuffer(
        uint32_t buffer,
        int32_t bindingIndex) const -> void;
    auto BindBufferAsShaderStorageBuffer(
        uint32_t buffer,
        int32_t bindingIndex,
        int64_t offset,
        int64_t size) const -> void;
    auto BindTexture(int32_t bindingIndex, uint32_t texture) const -> void;
    auto BindTextureAndSampler(int32_t bindingIndex, uint32_t texture, uint32_t sampler) const -> void;
    auto BindImage(uint32_t bindingIndex, uint32_t image, int32_t level, int32_t layer, TMemoryAccess memoryAccess, TFormat format) -> void;
    auto InsertMemoryBarrier(TMemoryBarrierMask memoryBarrierMask) -> void;
    auto SetUniform(int32_t location, float value) const -> void;
    auto SetUniform(int32_t location, int32_t value) const -> void;
    auto SetUniform(int32_t location, uint32_t value) const -> void;
    auto SetUniform(int32_t location, uint64_t value) const -> void;
    auto SetUniform(int32_t location, const glm::vec2& value) const -> void;
    auto SetUniform(int32_t location, const glm::vec3& value) const -> void;
    auto SetUniform(int32_t location, float value1, float value2, float value3, float value4) const -> void;
    auto SetUniform(int32_t location, int32_t value1, int32_t value2, int32_t value3, int32_t value4) const -> void;
    auto SetUniform(int32_t location, const glm::vec4& value) const -> void;
    auto SetUniform(int32_t location, const glm::mat3& value) const -> void;
    auto SetUniform(int32_t location, const glm::mat4& value) const -> void;
};

struct TGraphicsPipeline : public TPipeline {

    auto Bind() -> void override;
    auto BindBufferAsVertexBuffer(uint32_t buffer, uint32_t bindingIndex, size_t offset, size_t stride) -> void;
    auto DrawArrays(int32_t vertexOffset, size_t vertexCount) -> void;
    auto DrawElements(uint32_t indexBuffer, size_t elementCount) -> void;
    auto DrawElementsInstanced(uint32_t indexBuffer, size_t elementCount, size_t instanceCount) -> void;

    // Input Assembly
    std::optional<uint32_t> InputLayout = {};
    uint32_t PrimitiveTopology = {};
    bool IsPrimitiveRestartEnabled = {};

    // Rasterizer Stage
    TFillMode FillMode = {};
    TCullMode CullMode = {};
    TFaceWindingOrder FaceWindingOrder = {};
    bool IsDepthBiasEnabled = {};
    float DepthBiasSlopeFactor = {};
    float DepthBiasConstantFactor = {};
    bool IsDepthClampEnabled = {};
    bool IsScissorEnabled = {};
    TClipControl ClipControl = {};
    bool IsRasterizerDisabled = {};
    float LineWidth = {};
    float PointSize = {};

    // Output Merger State
    TColorMask ColorMask = {};
    bool IsDepthTestEnabled = {};
    bool IsDepthWriteEnabled = {};
    TDepthFunction DepthFunction = {};
};

struct TComputePipeline : public TPipeline {
    auto Dispatch(
        int32_t workGroupSizeX,
        int32_t workGroupSizeY,
        int32_t workGroupSizeZ) -> void;
};

struct TSampler {
    uint32_t Id;
};

struct TSamplerDescriptor {
    std::string_view Label;
    TTextureAddressMode AddressModeU = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeV = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeW = TTextureAddressMode::ClampToEdge;
    TTextureMagFilter MagFilter = TTextureMagFilter::Linear;
    TTextureMinFilter MinFilter = TTextureMinFilter::Linear;
    float LodBias = 0;
    float LodMin = -1000;
    float LodMax = 1000;

    bool operator==(const TSamplerDescriptor& rhs) const {
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
    struct hash<TSamplerDescriptor> {
        size_t operator()(const TSamplerDescriptor& samplerDescriptor) const {
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

auto SetDebugLabel(
    const uint32_t object,
    const uint32_t objectType,
    const std::string_view label) -> void;
auto PushDebugGroup(const std::string_view label) -> void;
auto PopDebugGroup() -> void;

auto CreateBuffer(
    std::string_view label,
    int64_t sizeInBytes,
    const void* data,
    uint32_t flags) -> uint32_t;
auto UpdateBuffer(
    uint32_t buffer,
    int64_t offsetInBytes,
    int64_t sizeInBytes,
    const void* data) -> void;
auto DeleteBuffer(uint32_t buffer) -> void;
auto DeletePipeline(const TPipeline& pipeline) -> void;

auto GetTexture(TTextureId id) -> TTexture&;
auto CreateTexture(const TCreateTextureDescriptor& createTextureDescriptor) -> TTextureId;
auto CreateTexture2DFromFile(const std::filesystem::path& filePath, TFormat format) -> TTextureId;
auto UploadTexture(const TTextureId& textureId, const TUploadTextureDescriptor& updateTextureDescriptor) -> void;
auto MakeTextureResident(const TTextureId& textureId) -> uint64_t;
auto GenerateMipmaps(const TTextureId& textureId) -> void;

auto GetSampler(const TSamplerId& id) -> TSampler&;
auto GetOrCreateSampler(const TSamplerDescriptor& samplerDescriptor) -> TSamplerId;

auto CreateFramebuffer(const TFramebufferDescriptor& framebufferDescriptor) -> TFramebuffer;
auto BindFramebuffer(const TFramebuffer& framebuffer) -> void;
auto DeleteFramebuffer(const TFramebuffer& framebuffer) -> void;

auto CreateGraphicsPipeline(const TGraphicsPipelineDescriptor& graphicsPipelineDescriptor) -> std::expected<TGraphicsPipeline, std::string>;
auto CreateComputePipeline(const TComputePipelineDescriptor& computePipelineDescriptor) -> std::expected<TComputePipeline, std::string>;

auto RhiInitialize(bool isDebug) -> void;
auto RhiShutdown() -> void;
