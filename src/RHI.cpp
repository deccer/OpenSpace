#include "RHI.hpp"
#include "Profiler.hpp"
#include "Images.hpp"

#include <glad/gl.h>

#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_GLSL
#include <stb_include.h>

std::vector<TTexture> g_textures;
TTextureId g_textureCounter = TTextureId::Invalid;

std::vector<TSampler> g_samplers;
TSamplerId g_samplerCounter = TSamplerId::Invalid;

std::unordered_map<TSamplerDescriptor, TSamplerId> g_samplerDescriptors;

uint32_t g_defaultInputLayout = 0;
uint32_t g_lastIndexBuffer = 0;

float g_maxTextureAnisotropy = 0.0f;

///// Conversions

constexpr auto PrimitiveTopologyToGL(TPrimitiveTopology primitiveTopology) -> uint32_t {
    switch (primitiveTopology) {
        case TPrimitiveTopology::Lines: return GL_LINES;
        case TPrimitiveTopology::TriangleFan: return GL_TRIANGLE_FAN;
        case TPrimitiveTopology::Triangles: return GL_TRIANGLES;
        case TPrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        default: std::unreachable();
    }
}

constexpr auto TextureAddressModeToGL(TTextureAddressMode textureAddressMode) -> uint32_t {
    switch (textureAddressMode) {
        case TTextureAddressMode::ClampToBorder : return GL_CLAMP_TO_BORDER;
        case TTextureAddressMode::ClampToEdge : return GL_CLAMP_TO_EDGE;
        case TTextureAddressMode::ClampToEdgeMirrored: return GL_MIRROR_CLAMP_TO_EDGE;
        case TTextureAddressMode::Repeat: return GL_REPEAT;
        case TTextureAddressMode::RepeatMirrored: return GL_MIRRORED_REPEAT;
        default: std::unreachable();
    }
}

constexpr auto TextureMagFilterToGL(TTextureMagFilter textureMagFilter) -> uint32_t {
    switch (textureMagFilter) {
        case TTextureMagFilter::Linear: return GL_LINEAR;
        case TTextureMagFilter::Nearest: return GL_NEAREST;
        default: std::unreachable();
    }
}

constexpr auto TextureMinFilterToGL(TTextureMinFilter textureMinFilter) -> uint32_t {
    switch (textureMinFilter) {
        case TTextureMinFilter::Linear: return GL_LINEAR;
        case TTextureMinFilter::Nearest: return GL_NEAREST;
        case TTextureMinFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
        case TTextureMinFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
        case TTextureMinFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
        case TTextureMinFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        default: std::unreachable();
    }
}

constexpr auto TextureTypeToGL(TTextureType textureType) -> uint32_t {

    switch (textureType)
    {
        case TTextureType::Texture1D: return GL_TEXTURE_1D;
        case TTextureType::Texture2D: return GL_TEXTURE_2D;
        case TTextureType::Texture3D: return GL_TEXTURE_3D;
        case TTextureType::Texture1DArray: return GL_TEXTURE_1D_ARRAY;
        case TTextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
        case TTextureType::TextureCube: return GL_TEXTURE_CUBE_MAP;
        case TTextureType::TextureCubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
        case TTextureType::Texture2DMultisample: return GL_TEXTURE_2D_MULTISAMPLE;
        case TTextureType::Texture2DMultisampleArray: return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        default: std::unreachable();
    }
}

constexpr auto UploadFormatToGL(TUploadFormat uploadFormat) -> uint32_t {

    switch (uploadFormat) {
        case TUploadFormat::R: return GL_RED;
        case TUploadFormat::Rg: return GL_RG;
        case TUploadFormat::Rgb: return GL_RGB;
        case TUploadFormat::Bgr: return GL_BGR;
        case TUploadFormat::Rgba: return GL_RGBA;
        case TUploadFormat::Bgra: return GL_BGRA;
        case TUploadFormat::RInteger: return GL_RED_INTEGER;
        case TUploadFormat::RgInteger: return GL_RG_INTEGER;
        case TUploadFormat::RgbInteger: return GL_RGB_INTEGER;
        case TUploadFormat::BgrInteger: return GL_BGR_INTEGER;
        case TUploadFormat::RgbaInteger: return GL_RGBA_INTEGER;
        case TUploadFormat::BgraInteger: return GL_BGRA_INTEGER;
        case TUploadFormat::Depth: return GL_DEPTH_COMPONENT;
        case TUploadFormat::StencilIndex: return GL_STENCIL_INDEX;
        case TUploadFormat::DepthStencilIndex: return GL_DEPTH_STENCIL;
        default: std::unreachable();
    }
}

constexpr auto UploadTypeToGL(TUploadType uploadType) -> uint32_t {

    switch (uploadType) {
        case TUploadType::UnsignedByte: return GL_UNSIGNED_BYTE;
        case TUploadType::SignedByte: return GL_BYTE;
        case TUploadType::UnsignedShort: return GL_UNSIGNED_SHORT;
        case TUploadType::SignedShort: return GL_SHORT;
        case TUploadType::UnsignedInteger: return GL_UNSIGNED_INT;
        case TUploadType::SignedInteger: return GL_INT;
        case TUploadType::Float: return GL_FLOAT;
        case TUploadType::UnsignedByte332: return GL_UNSIGNED_BYTE_3_3_2;
        case TUploadType::UnsignedByte233Reversed: return GL_UNSIGNED_BYTE_2_3_3_REV;
        case TUploadType::UnsignedShort565: return GL_UNSIGNED_SHORT_5_6_5;
        case TUploadType::UnsignedShort565Reversed: return GL_UNSIGNED_SHORT_5_6_5_REV;
        case TUploadType::UnsignedShort4444: return GL_UNSIGNED_SHORT_4_4_4_4;
        case TUploadType::UnsignedShort4444Reversed: return GL_UNSIGNED_SHORT_4_4_4_4_REV;
        case TUploadType::UnsignedShort5551: return GL_UNSIGNED_SHORT_5_5_5_1;
        case TUploadType::UnsignedShort1555Reversed: return GL_UNSIGNED_SHORT_1_5_5_5_REV;
        case TUploadType::UnsignedInteger8888: return GL_UNSIGNED_INT_8_8_8_8;
        case TUploadType::UnsignedInteger8888Reversed: return GL_UNSIGNED_INT_8_8_8_8_REV;
        case TUploadType::UnsignedInteger1010102: return GL_UNSIGNED_INT_10_10_10_2;
        case TUploadType::UnsignedInteger2101010Reversed: return GL_UNSIGNED_INT_2_10_10_10_REV;
        default: std::unreachable();
    }
}

constexpr auto SampleCountToGL(TSampleCount sampleCount) -> uint32_t {

    switch (sampleCount) {
        case TSampleCount::One: return 1;
        case TSampleCount::Two: return 2;
        case TSampleCount::Four: return 4;
        case TSampleCount::Eight: return 8;
        case TSampleCount::SixTeen: return 16;
        case TSampleCount::ThirtyTwo: return 32;
        default: std::unreachable();
    }
}

constexpr auto MemoryAccessToGL(TMemoryAccess memoryAccess) -> uint32_t {
    switch (memoryAccess) {
        case TMemoryAccess::ReadOnly: return GL_READ_ONLY;
        case TMemoryAccess::WriteOnly: return GL_WRITE_ONLY;
        case TMemoryAccess::ReadWrite: return GL_READ_WRITE;
        default: std::unreachable();
    }
}

auto FormatToGL(TFormat format) -> uint32_t {
    switch (format) {
        case TFormat::R8_UNORM: return GL_R8;
        case TFormat::R8_SNORM: return GL_R8_SNORM;
        case TFormat::R16_UNORM: return GL_R16;
        case TFormat::R16_SNORM: return GL_R16_SNORM;
        case TFormat::R8G8_UNORM: return GL_RG8;
        case TFormat::R8G8_SNORM: return GL_RG8_SNORM;
        case TFormat::R16G16_UNORM: return GL_RG16;
        case TFormat::R16G16_SNORM: return GL_RG16_SNORM;
        case TFormat::R3G3B2_UNORM: return GL_R3_G3_B2;
        case TFormat::R4G4B4_UNORM: return GL_RGB4;
        case TFormat::R5G5B5_UNORM: return GL_RGB5;
        case TFormat::R8G8B8_UNORM: return GL_RGB8;
        case TFormat::R8G8B8_SNORM: return GL_RGB8_SNORM;
        case TFormat::R10G10B10_UNORM: return GL_RGB10;
        case TFormat::R12G12B12_UNORM: return GL_RGB12;
        // GL_RG16?
        case TFormat::R16G16B16_SNORM: return GL_RGB16_SNORM;
        case TFormat::R2G2B2A2_UNORM: return GL_RGBA2;
        case TFormat::R4G4B4A4_UNORM: return GL_RGBA4;
        case TFormat::R5G5B5A1_UNORM: return GL_RGB5_A1;
        case TFormat::R8G8B8A8_UNORM: return GL_RGBA8;
        case TFormat::R8G8B8A8_SNORM: return GL_RGBA8_SNORM;
        case TFormat::R10G10B10A2_UNORM: return GL_RGB10_A2;
        case TFormat::R10G10B10A2_UINT: return GL_RGB10_A2UI;
        case TFormat::R12G12B12A12_UNORM: return GL_RGBA12;
        case TFormat::R16G16B16A16_UNORM: return GL_RGBA16;
        case TFormat::R16G16B16A16_SNORM: return GL_RGBA16_SNORM;
        case TFormat::R8G8B8_SRGB: return GL_SRGB8;
        case TFormat::R8G8B8A8_SRGB: return GL_SRGB8_ALPHA8;
        case TFormat::R16_FLOAT: return GL_R16F;
        case TFormat::R16G16_FLOAT: return GL_RG16F;
        case TFormat::R16G16B16_FLOAT: return GL_RGB16F;
        case TFormat::R16G16B16A16_FLOAT: return GL_RGBA16F;
        case TFormat::R32_FLOAT: return GL_R32F;
        case TFormat::R32G32_FLOAT: return GL_RG32F;
        case TFormat::R32G32B32_FLOAT: return GL_RGB32F;
        case TFormat::R32G32B32A32_FLOAT: return GL_RGBA32F;
        case TFormat::R11G11B10_FLOAT: return GL_R11F_G11F_B10F;
        case TFormat::R9G9B9_E5: return GL_RGB9_E5;
        case TFormat::R8_SINT: return GL_R8I;
        case TFormat::R8_UINT: return GL_R8UI;
        case TFormat::R16_SINT: return GL_R16I;
        case TFormat::R16_UINT: return GL_R16UI;
        case TFormat::R32_SINT: return GL_R32I;
        case TFormat::R32_UINT: return GL_R32UI;
        case TFormat::R8G8_SINT: return GL_RG8I;
        case TFormat::R8G8_UINT: return GL_RG8UI;
        case TFormat::R16G16_SINT: return GL_RG16I;
        case TFormat::R16G16_UINT: return GL_RG16UI;
        case TFormat::R32G32_SINT: return GL_RG32I;
        case TFormat::R32G32_UINT: return GL_RG32UI;
        case TFormat::R8G8B8_SINT: return GL_RGB8I;
        case TFormat::R8G8B8_UINT: return GL_RGB8UI;
        case TFormat::R16G16B16_SINT: return GL_RGB16I;
        case TFormat::R16G16B16_UINT: return GL_RGB16UI;
        case TFormat::R32G32B32_SINT: return GL_RGB32I;
        case TFormat::R32G32B32_UINT: return GL_RGB32UI;
        case TFormat::R8G8B8A8_SINT: return GL_RGBA8I;
        case TFormat::R8G8B8A8_UINT: return GL_RGBA8UI;
        case TFormat::R16G16B16A16_SINT: return GL_RGBA16I;
        case TFormat::R16G16B16A16_UINT: return GL_RGBA16UI;
        case TFormat::R32G32B32A32_SINT: return GL_RGBA32I;
        case TFormat::R32G32B32A32_UINT: return GL_RGBA32UI;
        case TFormat::D32_FLOAT: return GL_DEPTH_COMPONENT32F;
        case TFormat::D32_UNORM: return GL_DEPTH_COMPONENT32;
        case TFormat::D24_UNORM: return GL_DEPTH_COMPONENT24;
        case TFormat::D16_UNORM: return GL_DEPTH_COMPONENT16;
        case TFormat::D32_FLOAT_S8_UINT: return GL_DEPTH32F_STENCIL8;
        case TFormat::D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
        case TFormat::S8_UINT: return GL_STENCIL_INDEX8;
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
        case TFormat::BC4_R_UNORM: return GL_COMPRESSED_RED_RGTC1;
        case TFormat::BC4_R_SNORM: return GL_COMPRESSED_SIGNED_RED_RGTC1;
        case TFormat::BC5_RG_UNORM: return GL_COMPRESSED_RG_RGTC2;
        case TFormat::BC5_RG_SNORM: return GL_COMPRESSED_SIGNED_RG_RGTC2;
        case TFormat::BC6H_RGB_UFLOAT: return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case TFormat::BC6H_RGB_SFLOAT: return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
        case TFormat::BC7_RGBA_UNORM: return GL_COMPRESSED_RGBA_BPTC_UNORM;
        case TFormat::BC7_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
        default:
            std::string message = "Format not mappable";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

auto AttachmentTypeToGL(TAttachmentType attachmentType) -> uint32_t {
    switch (attachmentType) {
        case TAttachmentType::ColorAttachment0: return GL_COLOR_ATTACHMENT0;
        case TAttachmentType::ColorAttachment1: return GL_COLOR_ATTACHMENT1;
        case TAttachmentType::ColorAttachment2: return GL_COLOR_ATTACHMENT2;
        case TAttachmentType::ColorAttachment3: return GL_COLOR_ATTACHMENT3;
        case TAttachmentType::ColorAttachment4: return GL_COLOR_ATTACHMENT4;
        case TAttachmentType::ColorAttachment5: return GL_COLOR_ATTACHMENT5;
        case TAttachmentType::ColorAttachment6: return GL_COLOR_ATTACHMENT6;
        case TAttachmentType::ColorAttachment7: return GL_COLOR_ATTACHMENT7;
        case TAttachmentType::DepthAttachment: return GL_DEPTH_ATTACHMENT;
        case TAttachmentType::StencilAttachment: return GL_STENCIL_ATTACHMENT;
        default:
            std::string message = "AttachmentType not mappable";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

auto FormatToUploadFormat(TFormat format) -> TUploadFormat {

    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R16_FLOAT:
        case TFormat::R32_FLOAT:
            return TUploadFormat::R;
        case TFormat::R8_SINT:
        case TFormat::R8_UINT:
        case TFormat::R16_SINT:
        case TFormat::R16_UINT:
        case TFormat::R32_SINT:
        case TFormat::R32_UINT:
            return TUploadFormat::RInteger;
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R16G16_FLOAT:
        case TFormat::R32G32_FLOAT:
            return TUploadFormat::Rg;
        case TFormat::R8G8_SINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_SINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_SINT:
        case TFormat::R32G32_UINT:
            return TUploadFormat::RgInteger;
        case TFormat::R3G3B2_UNORM:
        case TFormat::R4G4B4_UNORM:
        case TFormat::R5G5B5_UNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R10G10B10_UNORM:
        case TFormat::R12G12B12_UNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R8G8B8_SRGB:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R9G9B9_E5:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R11G11B10_FLOAT:
            return TUploadFormat::Rgb;
        case TFormat::R8G8B8_SINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R32G32B32_UINT:
            return TUploadFormat::RgbInteger;
        case TFormat::R2G2B2A2_UNORM:
        case TFormat::R4G4B4A4_UNORM:
        case TFormat::R5G5B5A1_UNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R10G10B10A2_UNORM:
        case TFormat::R12G12B12A12_UNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16G16B16A16_SNORM:
        case TFormat::R8G8B8A8_SRGB:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
            return TUploadFormat::Rgba;
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R32G32B32A32_UINT:
            return TUploadFormat::RgbaInteger;
        case TFormat::D32_FLOAT:
        case TFormat::D32_UNORM:
        case TFormat::D24_UNORM:
        case TFormat::D16_UNORM:
            return TUploadFormat::Depth;
        case TFormat::D32_FLOAT_S8_UINT:
        case TFormat::D24_UNORM_S8_UINT:
            return TUploadFormat::DepthStencilIndex;
        case TFormat::S8_UINT:
            return TUploadFormat::StencilIndex;
        default:
            std::unreachable();
	}
}

auto FormatToUnderlyingOpenGLType(TFormat format) -> uint32_t {

    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8_UINT:
        case TFormat::R8G8_UINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R8G8B8A8_SRGB:
        case TFormat::R8G8B8_SRGB:
            return GL_UNSIGNED_BYTE;
        case TFormat::R8_SNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R8_SINT:
        case TFormat::R8G8_SINT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R8G8B8A8_SINT:
            return GL_BYTE;
        case TFormat::R16_UNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R16G16B16A16_UINT:
            return GL_UNSIGNED_SHORT;
        case TFormat::R16_SNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R16G16B16A16_SNORM:
        case TFormat::R16_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R16G16B16A16_SINT:
            return GL_SHORT;
        case TFormat::R16_FLOAT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R16G16B16A16_FLOAT:
            return GL_HALF_FLOAT;
        case TFormat::R32_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
            return GL_FLOAT;
        case TFormat::R32_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R32G32B32A32_SINT:
            return GL_INT;
        case TFormat::R32_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R32G32B32A32_UINT:
            return GL_UNSIGNED_INT;
        default:
            std::string message = "Format not mappable to opengl type";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

constexpr auto TextureTypeToDimension(TTextureType textureType) -> uint32_t {

    switch (textureType) {
        case TTextureType::Texture1D:
            return 1;
        case TTextureType::Texture2D:
        case TTextureType::Texture2DMultisample:
        case TTextureType::Texture1DArray:
            return 2;
        case TTextureType::Texture3D:
        case TTextureType::Texture2DArray:
        case TTextureType::Texture2DMultisampleArray:
        case TTextureType::TextureCube:
        case TTextureType::TextureCubeArray:
            return 3;
        default:
            std::unreachable();
    }
}

auto FormatToBaseTypeClass(TFormat format) -> TBaseTypeClass
{
    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R3G3B2_UNORM:
        case TFormat::R4G4B4_UNORM:
        case TFormat::R5G5B5_UNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R10G10B10_UNORM:
        case TFormat::R12G12B12_UNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R2G2B2A2_UNORM:
        case TFormat::R4G4B4A4_UNORM:
        case TFormat::R5G5B5A1_UNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R10G10B10A2_UNORM:
        case TFormat::R12G12B12A12_UNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R8G8B8_SRGB:
        case TFormat::R8G8B8A8_SRGB:
        case TFormat::R16_FLOAT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
        case TFormat::R11G11B10_FLOAT:
        case TFormat::R9G9B9_E5:
            return TBaseTypeClass::Float;
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
            return TBaseTypeClass::Integer;
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return TBaseTypeClass::UnsignedInteger;
        default:
            std::unreachable();
    }
}

constexpr auto FormatToAttachmentType(TFormat attachmentFormat, size_t colorAttachmentIndex) -> TAttachmentType {

    switch (attachmentFormat) {
        case TFormat::D24_UNORM:
        case TFormat::D24_UNORM_S8_UINT:
        case TFormat::D32_FLOAT:
        case TFormat::D32_FLOAT_S8_UINT:
        case TFormat::D32_UNORM:
            return TAttachmentType::DepthAttachment;
        default:
            return static_cast<TAttachmentType>(static_cast<std::underlying_type<TAttachmentType>::type>(TAttachmentType::ColorAttachment0) + colorAttachmentIndex);
    }
}

auto FormatToComponentCount(TFormat format) -> int32_t {
    switch (format) {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
            return 1;
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R32G32_FLOAT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
            return 2;
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
            return 3;
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return 4;
        default:
            std::unreachable();
    }
}

auto IsFormatNormalized(TFormat format) -> int32_t {

    switch (format) {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R16G16B16A16_UNORM:
            return GL_TRUE;
        case TFormat::R16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return GL_FALSE;
        default:
            std::unreachable();
    }
}

auto FormatToFormatClass(TFormat format) -> TFormatClass
  {
    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16_FLOAT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
            return TFormatClass::Float;
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return TFormatClass::Integer;
        default:
            return TFormatClass::Long;
    }
}

auto ReadShaderSourceFromFile(const std::filesystem::path& filePath) -> std::expected<std::string, std::string> {

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

auto SetDebugLabel(
    const uint32_t object,
    const uint32_t objectType,
    const std::string_view label) -> void {

    glObjectLabel(objectType, object, static_cast<GLsizei>(label.size()), label.data());
}

auto PushDebugGroup(const std::string_view label) -> void {

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(label.size()), label.data());
}

auto PopDebugGroup() -> void {

    glPopDebugGroup();
}


auto CreateBuffer(
    std::string_view label,
    int64_t sizeInBytes,
    const void* data,
    uint32_t flags) -> uint32_t {

    uint32_t buffer = 0;
    glCreateBuffers(1, &buffer);
    SetDebugLabel(buffer, GL_BUFFER, label);
    glNamedBufferStorage(buffer, sizeInBytes, data, flags);
    return buffer;
}

auto UpdateBuffer(
    uint32_t buffer,
    int64_t offsetInBytes,
    int64_t sizeInBytes,
    const void* data) -> void {

    glNamedBufferSubData(buffer, offsetInBytes, sizeInBytes, data);
}

auto DeleteBuffer(uint32_t buffer) -> void {

    glDeleteBuffers(1, &buffer);
}

auto DeletePipeline(TPipeline& pipeline) -> void {

    glDeleteProgram(pipeline.Id);
}

auto TPipeline::Bind() -> void {

    glUseProgram(Id);
}

auto TPipeline::BindBufferAsUniformBuffer(
    uint32_t buffer,
    int32_t bindingIndex) const -> void {

    glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, buffer);
}

auto TPipeline::BindBufferAsUniformBuffer(
    uint32_t buffer,
    int32_t bindingIndex,
    int64_t offset,
    int64_t size) const -> void {

    glBindBufferRange(GL_UNIFORM_BUFFER, bindingIndex, buffer, offset, size);
}

auto TPipeline::BindBufferAsShaderStorageBuffer(
    uint32_t buffer,
    int32_t bindingIndex) const -> void {

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, buffer);
}

auto TPipeline::BindBufferAsShaderStorageBuffer(
    uint32_t buffer,
    int32_t bindingIndex,
    int64_t offset,
    int64_t size) const -> void {

    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingIndex, buffer, offset, size);
}

auto TPipeline::BindTexture(
    int32_t bindingIndex,
    uint32_t texture) const  -> void {

    glBindTextureUnit(bindingIndex, texture);
}

auto TPipeline::BindTextureAndSampler(
    int32_t bindingIndex,
    uint32_t texture,
    uint32_t sampler) const  -> void {

    glBindTextureUnit(bindingIndex, texture);
    glBindSampler(bindingIndex, sampler);
}

auto TPipeline::BindImage(
    uint32_t bindingIndex,
    uint32_t texture,
    int32_t level,
    int32_t layer,
    TMemoryAccess memoryAccess,
    TFormat format) -> void {

    glBindImageTexture(bindingIndex, texture, level, GL_TRUE, layer, MemoryAccessToGL(memoryAccess), FormatToGL(format));
}

auto TPipeline::InsertMemoryBarrier(TMemoryBarrierMask memoryBarrierMask) -> void {
    glMemoryBarrier(static_cast<uint32_t>(memoryBarrierMask));
}

auto TPipeline::SetUniform(int32_t location, float value) const -> void {
    glProgramUniform1f(Id, location, value);
}

auto TPipeline::SetUniform(int32_t location, int32_t value) const -> void {
    glProgramUniform1i(Id, location, value);
}

auto TPipeline::SetUniform(int32_t location, uint32_t value) const -> void {
    glProgramUniform1ui(Id, location, value);
}

auto TPipeline::SetUniform(int32_t location, uint64_t value) const -> void {
    glProgramUniformHandleui64ARB(Id, location, value); //TODO(deccer) add check for bindless support
}

auto TPipeline::SetUniform(int32_t location, const glm::vec2& value) const -> void {
    glProgramUniform2fv(Id, location, 1, glm::value_ptr(value));
}

auto TPipeline::SetUniform(int32_t location, const glm::vec3& value) const -> void {
    glProgramUniform3fv(Id, location, 1, glm::value_ptr(value));
}

auto TPipeline::SetUniform(int32_t location, float value1, float value2, float value3, float value4) const -> void {
    glProgramUniform4f(Id, location, value1, value2, value3, value4);
}

auto TPipeline::SetUniform(int32_t location, int32_t value1, int32_t value2, int32_t value3, int32_t value4) const -> void {
    glProgramUniform4i(Id, location, value1, value2, value3, value4);
}

auto TPipeline::SetUniform(int32_t location, const glm::vec4& value) const -> void {
    glProgramUniform4fv(Id, location, 1, glm::value_ptr(value));
}

auto TPipeline::SetUniform(int32_t location, const glm::mat4& value) const -> void {
    glProgramUniformMatrix4fv(Id, location, 1, GL_FALSE, glm::value_ptr(value));
}

auto GetTexture(TTextureId id) -> TTexture& {

    assert(id != TTextureId::Invalid);
    return g_textures[size_t(id)];
}

auto CreateTexture2DFromFile(const std::filesystem::path& filePath, TFormat format) -> TTextureId {

    int32_t imageWidth = 0;
    int32_t imageHeight = 0;
    int32_t imageComponents = 0;
    auto imageData = Image::LoadImageFromFile(filePath, &imageWidth, &imageHeight, &imageComponents);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = format,
        .Extent = TExtent3D{ static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1u},
        .MipMapLevels = 1 + static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(imageWidth), static_cast<float>(imageHeight))))),
        .Layers = 0,
        .SampleCount = TSampleCount::One,
        .Label = filePath.filename().string(),
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{0, 0, 0},
        .Extent = TExtent3D{static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = imageData
    });

    Image::FreeImage(imageData);

    return textureId;
}

auto CreateTexture(const TCreateTextureDescriptor& createTextureDescriptor) -> TTextureId {

    PROFILER_ZONESCOPEDN("CreateTexture");
    TTexture texture = {};

    glCreateTextures(TextureTypeToGL(createTextureDescriptor.TextureType), 1, &texture.Id);
    if (!createTextureDescriptor.Label.empty()) {
        SetDebugLabel(texture.Id, GL_TEXTURE, createTextureDescriptor.Label);
    }

    texture.Extent = createTextureDescriptor.Extent;
    texture.Format = createTextureDescriptor.Format;
    texture.TextureType = createTextureDescriptor.TextureType;

    switch (createTextureDescriptor.TextureType)
    {
        case TTextureType::Texture1D:
            glTextureStorage1D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width);
            break;
        case TTextureType::Texture2D:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height);
            break;
        case TTextureType::Texture3D:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Extent.Depth);
            break;
        case TTextureType::Texture1DArray:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Layers);
            break;
        case TTextureType::Texture2DArray:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Layers);
            break;
        case TTextureType::TextureCube:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height);
            break;
        case TTextureType::TextureCubeArray:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Layers);
            break;
        case TTextureType::Texture2DMultisample:
            glTextureStorage2DMultisample(texture.Id,
                                          SampleCountToGL(createTextureDescriptor.SampleCount),
                                          FormatToGL(createTextureDescriptor.Format),
                                          createTextureDescriptor.Extent.Width,
                                          createTextureDescriptor.Extent.Height,
                                          GL_TRUE);
            break;
        case TTextureType::Texture2DMultisampleArray:
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

    const auto textureId = static_cast<TTextureId>(g_textures.size());
    g_textures.emplace_back(texture);

    return textureId;
}

auto UploadTexture(const TTextureId& textureId, const TUploadTextureDescriptor& updateTextureDescriptor) -> void {

    PROFILER_ZONESCOPEDN("UploadTexture");
    auto& texture = GetTexture(textureId);

    uint32_t format = 0;
    if (updateTextureDescriptor.UploadFormat == TUploadFormat::Auto)
    {
        format = UploadFormatToGL(FormatToUploadFormat(texture.Format));
    }
    else
    {
        format = UploadFormatToGL(updateTextureDescriptor.UploadFormat);
    }

    uint32_t type = 0;
    if (updateTextureDescriptor.UploadType == TUploadType::Auto)
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

auto MakeTextureResident(const TTextureId& textureId) -> uint64_t {

    auto& texture = GetTexture(textureId);

    auto textureHandle = glGetTextureHandleARB(texture.Id);
    glMakeTextureHandleResidentARB(textureHandle);

    return textureHandle;
}

auto GenerateMipmaps(const TTextureId& textureId) -> void {

    auto& texture = GetTexture(textureId);
    glGenerateTextureMipmap(texture.Id);
}

auto DeleteTextures() -> void {
    for(auto& texture : g_textures) {
        glDeleteTextures(1, &texture.Id);
    }
}

auto GetSampler(const TSamplerId& id) -> TSampler&
{
    assert(id != TSamplerId::Invalid);
    return g_samplers[static_cast<size_t>(id)];
}

auto GetOrCreateSampler(const TSamplerDescriptor& samplerDescriptor) -> TSamplerId {

    if (const auto existingSamplerDescriptor = g_samplerDescriptors.find(samplerDescriptor); existingSamplerDescriptor != g_samplerDescriptors.end()) {
        return existingSamplerDescriptor->second;
    }

    TSampler sampler = {};
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

    if (samplerDescriptor.MinFilter == TTextureMinFilter::Linear ||
        samplerDescriptor.MinFilter == TTextureMinFilter::NearestMipmapLinear ||
        samplerDescriptor.MinFilter == TTextureMinFilter::LinearMipmapLinear) {
        glSamplerParameterf(sampler.Id, GL_TEXTURE_MAX_ANISOTROPY, g_maxTextureAnisotropy);
    }

    const auto samplerId = static_cast<TSamplerId>(g_samplers.size());
    g_samplerDescriptors.insert({samplerDescriptor, samplerId});
    g_samplers.emplace_back(sampler);

    return samplerId;
}

auto CreateFramebuffer(const TFramebufferDescriptor& framebufferDescriptor) -> TFramebuffer {

    PROFILER_ZONESCOPEDN("CreateFramebuffer");

    TFramebuffer framebuffer = {};
    glCreateFramebuffers(1, &framebuffer.Id);
    if (!framebufferDescriptor.Label.empty()) {
        SetDebugLabel(framebuffer.Id, GL_FRAMEBUFFER, framebufferDescriptor.Label);
    }

    std::array<uint32_t, 8> drawBuffers = {};

    for (auto colorAttachmentIndex = 0; auto colorAttachmentDescriptorValue : framebufferDescriptor.ColorAttachments) {
        if (colorAttachmentDescriptorValue.has_value()) {
            auto& colorAttachmentDescriptor = *colorAttachmentDescriptorValue;
            const auto colorAttachmentTextureId = CreateTexture({
                .TextureType = TTextureType::Texture2D,
                .Format = colorAttachmentDescriptor.Format,
                .Extent = TExtent3D(colorAttachmentDescriptor.Extent.Width, colorAttachmentDescriptor.Extent.Height, 1),
                .MipMapLevels = 1,
                .Layers = 0,
                .SampleCount = TSampleCount::One,
                .Label = std::format("{}_{}x{}", colorAttachmentDescriptor.Label, colorAttachmentDescriptor.Extent.Width, colorAttachmentDescriptor.Extent.Height)
            });
            const auto& colorAttachmentTexture = GetTexture(colorAttachmentTextureId);

            framebuffer.ColorAttachments[colorAttachmentIndex] = {
                .Texture = colorAttachmentTexture,
                .ClearColor = colorAttachmentDescriptor.ClearColor,
                .LoadOperation = colorAttachmentDescriptor.LoadOperation,
            };

            const auto attachmentType = FormatToAttachmentType(colorAttachmentDescriptor.Format, colorAttachmentIndex);
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
        if (auto* createDepthStencilAttachment = std::get_if<TFramebufferDepthStencilAttachmentDescriptor>(&depthStencilAttachment)) {
            auto depthTextureId = CreateTexture({
                .TextureType = TTextureType::Texture2D,
                .Format = createDepthStencilAttachment->Format,
                .Extent = TExtent3D(createDepthStencilAttachment->Extent.Width,
                                    createDepthStencilAttachment->Extent.Height, 1),
                .MipMapLevels = 1,
                .Layers = 0,
                .SampleCount = TSampleCount::One,
                .Label = std::format("{}_{}x{}", createDepthStencilAttachment->Label,
                                     createDepthStencilAttachment->Extent.Width,
                                     createDepthStencilAttachment->Extent.Height)
            });
            const auto& depthTexture = g_textures[size_t(depthTextureId)];

            const auto attachmentType = FormatToAttachmentType(createDepthStencilAttachment->Format, 0);
            glNamedFramebufferTexture(framebuffer.Id, AttachmentTypeToGL(attachmentType), depthTexture.Id, 0);

            framebuffer.DepthStencilAttachment = {
                .Texture = depthTexture,
                .ClearDepthStencil = createDepthStencilAttachment->ClearDepthStencil,
                .LoadOperation = createDepthStencilAttachment->LoadOperation,
            };
        } else if (auto* existingDepthStencilAttachment = std::get_if<TFramebufferExistingDepthStencilAttachmentDescriptor>(&depthStencilAttachment)) {
            const auto& depthTexture = existingDepthStencilAttachment->ExistingDepthTexture;
            glNamedFramebufferTexture(framebuffer.Id, GL_DEPTH_ATTACHMENT, depthTexture.Id, 0);
            framebuffer.DepthStencilAttachment = {
                .Texture = depthTexture,
                .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
            };
        }
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

auto BindFramebuffer(const TFramebuffer& framebuffer) -> void {

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.Id);

    auto hasColorAttachment = true;
    for (auto colorAttachmentIndex = 0; auto colorAttachmentValue : framebuffer.ColorAttachments) {
        if (colorAttachmentValue.has_value()) {
            auto& colorAttachment = *colorAttachmentValue;
            glViewportIndexedf(colorAttachmentIndex, 0, 0, colorAttachment.Texture.Extent.Width, colorAttachment.Texture.Extent.Height);
            glColorMaski(colorAttachmentIndex, true, true, true, true);
            if (colorAttachment.LoadOperation == TFramebufferAttachmentLoadOperation::Clear) {
                auto baseTypeClass = FormatToBaseTypeClass(colorAttachment.Texture.Format);
                switch (baseTypeClass) {
                    case TBaseTypeClass::Float:
                        glClearNamedFramebufferfv(framebuffer.Id, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<float, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    case TBaseTypeClass::Integer:
                        glClearNamedFramebufferiv(framebuffer.Id, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<int32_t, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    case TBaseTypeClass::UnsignedInteger:
                        glClearNamedFramebufferuiv(framebuffer.Id, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<uint32_t, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    default:
                        std::unreachable_sentinel;
                }
            }
        } else {
            hasColorAttachment = false;
        }
        colorAttachmentIndex++;
    }

    if (framebuffer.DepthStencilAttachment.has_value()) {
        auto& depthStencilAttachment = *framebuffer.DepthStencilAttachment;
        if (!hasColorAttachment) {
            glViewport(0, 0, depthStencilAttachment.Texture.Extent.Width, depthStencilAttachment.Texture.Extent.Height);
        }
        if (depthStencilAttachment.LoadOperation == TFramebufferAttachmentLoadOperation::Clear) {
            glDepthMask(GL_TRUE);
            glStencilMask(GL_TRUE);
            glClearNamedFramebufferfi(framebuffer.Id, GL_DEPTH_STENCIL, 0, depthStencilAttachment.ClearDepthStencil.Depth, depthStencilAttachment.ClearDepthStencil.Stencil);
        }
    }
}

auto DeleteFramebuffer(const TFramebuffer& framebuffer) -> void {

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
    const std::string_view vertexShaderFilePath,
    const std::string_view fragmentShaderFilePath) -> std::expected<uint32_t, std::string> {

    PROFILER_ZONESCOPEDN("CreateGraphicsProgram");

    auto vertexShaderSourceResult = ReadShaderSourceFromFile(vertexShaderFilePath);
    if (!vertexShaderSourceResult) {
        return std::unexpected(vertexShaderSourceResult.error());
    }

    auto fragmentShaderSourceResult = ReadShaderSourceFromFile(fragmentShaderFilePath);
    if (!fragmentShaderSourceResult) {
        return std::unexpected(fragmentShaderSourceResult.error());
    }

    const auto vertexShaderSource = *vertexShaderSourceResult;
    const auto fragmentShaderSource = *fragmentShaderSourceResult;

    const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    SetDebugLabel(vertexShader, GL_SHADER, std::format("{}-VS", label).data());
    const auto vertexShaderSourcePtr = vertexShaderSource.data();
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    glCompileShader(vertexShader);
    int32_t status = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(vertexShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(vertexShader);

        return std::unexpected(std::format("Vertex shader in program {} has errors\n{}", label, infoLog));
    }

    const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    SetDebugLabel(fragmentShader, GL_SHADER, std::format("{}-FS", label).data());
    const auto fragmentShaderSourcePtr = fragmentShaderSource.data();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(fragmentShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(fragmentShader);

        return std::unexpected(std::format("Fragment shader in program {} has errors\n{}", label, infoLog));
    }

    auto program = glCreateProgram();
    SetDebugLabel(program, GL_PROGRAM, std::format("{}-Program", label).data());
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetProgramInfoLog(program, infoLength, nullptr, infoLog.data());
        glDeleteProgram(program);

        return std::unexpected(std::format("Graphics program {} has linking errors\n{}", label, infoLog));
    }

    return program;
}

auto CreateComputeProgram(
    std::string_view label,
    const std::string_view computeShaderFilePath) -> std::expected<uint32_t, std::string> {

    PROFILER_ZONESCOPEDN("CreateComputeProgram");

    auto computeShaderSourceResult = ReadShaderSourceFromFile(computeShaderFilePath);
    if (!computeShaderSourceResult) {
        return std::unexpected(computeShaderSourceResult.error());
    }

    int32_t status = 0;

    const auto computeShader = glCreateShader(GL_COMPUTE_SHADER);
    SetDebugLabel(computeShader, GL_SHADER, std::format("{}-CS", label).data());
    const auto computeShaderSourcePtr = (*computeShaderSourceResult).data();
    glShaderSource(computeShader, 1, &computeShaderSourcePtr, nullptr);
    glCompileShader(computeShader);
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetShaderiv(computeShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(computeShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(computeShader);

        return std::unexpected(std::format("Compute shader in program {} has errors\n{}", label, infoLog));
    }

    auto program = glCreateProgram();
    SetDebugLabel(program, GL_PROGRAM, std::format("{}-Program", label).data());
    glAttachShader(program, computeShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    glDetachShader(program, computeShader);
    glDeleteShader(computeShader);

    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetProgramInfoLog(program, infoLength, nullptr, infoLog.data());

        return std::unexpected(std::format("Compute program {} has linking errors\n{}", label, infoLog));
    }

    return program;
}

auto FillModeToGL(TFillMode fillMode) -> uint32_t {
    switch (fillMode) {
    case TFillMode::Solid: return GL_FILL;
    case TFillMode::Wireframe: return GL_LINE;
    case TFillMode::Points: return GL_POINT;
    default: std::unreachable();
    }
}

auto CullModeToGL(TCullMode cullMode) -> uint32_t {
    switch (cullMode) {
    case TCullMode::None: return GL_NONE;
    case TCullMode::Back: return GL_BACK;
    case TCullMode::Front: return GL_FRONT;
    case TCullMode::FrontAndBack: return GL_FRONT_AND_BACK;
    default: std::unreachable();
    }
}

auto FaceWindingOrderToGL(TFaceWindingOrder faceWindingOrder) -> uint32_t {
    switch (faceWindingOrder) {
    case TFaceWindingOrder::CounterClockwise: return GL_CCW;
    case TFaceWindingOrder::Clockwise: return GL_CW;
    default: std::unreachable();
    }
}

auto DepthFunctionToGL(TDepthFunction depthFunction) -> uint32_t {
    switch (depthFunction) {
    case TDepthFunction::Never: return GL_NEVER;
    case TDepthFunction::Less: return GL_LESS;
    case TDepthFunction::Equal: return GL_EQUAL;
    case TDepthFunction::LessThanOrEqual: return GL_LEQUAL;
    case TDepthFunction::Greater: return GL_GREATER;
    case TDepthFunction::NotEqual: return GL_NOTEQUAL;
    case TDepthFunction::GreaterThanOrEqual: return GL_GEQUAL;
    case TDepthFunction::Always: return GL_ALWAYS;
    default: std::unreachable();
    }
}

auto CreateGraphicsPipeline(const TGraphicsPipelineDescriptor& graphicsPipelineDescriptor) -> std::expected<TGraphicsPipeline, std::string> {

    PROFILER_ZONESCOPEDN("CreateGraphicsPipeline");

    TGraphicsPipeline pipeline = {};

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
        SetDebugLabel(inputLayout, GL_VERTEX_ARRAY, std::format("InputLayout-{}", graphicsPipelineDescriptor.Label));

        auto& vertexInput = *graphicsPipelineDescriptor.VertexInput;
        for(auto inputAttributeIndex = 0; auto& inputAttribute : vertexInput.VertexInputAttributes) {
            if (inputAttribute.has_value()) {
                auto& inputAttributeValue = *inputAttribute;

                glEnableVertexArrayAttrib(inputLayout, inputAttributeValue.Location);
                glVertexArrayAttribBinding(inputLayout, inputAttributeValue.Location, inputAttributeValue.Binding);

                const auto type = FormatToUnderlyingOpenGLType(inputAttributeValue.Format);
                const auto componentCount = FormatToComponentCount(inputAttributeValue.Format);
                const auto isFormatNormalized = IsFormatNormalized(inputAttributeValue.Format);
                const auto formatClass = FormatToFormatClass(inputAttributeValue.Format);
                switch (formatClass)
                {
                    case TFormatClass::Float:
                        glVertexArrayAttribFormat(inputLayout, inputAttributeValue.Location, componentCount, type, isFormatNormalized, inputAttributeValue.Offset);
                        break;
                    case TFormatClass::Integer:
                        glVertexArrayAttribIFormat(inputLayout, inputAttributeValue.Location, componentCount, type, inputAttributeValue.Offset);
                        break;
                    case TFormatClass::Long:
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

        pipeline.InputLayout = inputLayout;
    } else {
        pipeline.InputLayout = std::nullopt;
    }
    // Input Assembly
    pipeline.PrimitiveTopology = PrimitiveTopologyToGL(graphicsPipelineDescriptor.InputAssembly.PrimitiveTopology);
    pipeline.IsPrimitiveRestartEnabled = graphicsPipelineDescriptor.InputAssembly.IsPrimitiveRestartEnabled;
    // Rasterizer Stage
    pipeline.FillMode = graphicsPipelineDescriptor.RasterizerState.FillMode;
    pipeline.CullMode = graphicsPipelineDescriptor.RasterizerState.CullMode;
    pipeline.FaceWindingOrder = graphicsPipelineDescriptor.RasterizerState.FaceWindingOrder;
    pipeline.IsDepthBiasEnabled = graphicsPipelineDescriptor.RasterizerState.IsDepthBiasEnabled;
    pipeline.DepthBiasSlopeFactor = graphicsPipelineDescriptor.RasterizerState.DepthBiasSlopeFactor;
    pipeline.DepthBiasConstantFactor = graphicsPipelineDescriptor.RasterizerState.DepthBiasConstantFactor;
    pipeline.IsDepthClampEnabled = graphicsPipelineDescriptor.RasterizerState.IsDepthClampEnabled;
    pipeline.ClipControl = graphicsPipelineDescriptor.RasterizerState.ClipControl;
    pipeline.IsRasterizerDisabled = graphicsPipelineDescriptor.RasterizerState.IsRasterizerDisabled;
    pipeline.LineWidth = graphicsPipelineDescriptor.RasterizerState.LineWidth;
    pipeline.PointSize = graphicsPipelineDescriptor.RasterizerState.PointSize;
    // Output Merger Stage
    pipeline.ColorMask = graphicsPipelineDescriptor.OutputMergerState.ColorMask;
    pipeline.IsDepthTestEnabled = graphicsPipelineDescriptor.OutputMergerState.DepthState.IsDepthTestEnabled;
    pipeline.IsDepthWriteEnabled = graphicsPipelineDescriptor.OutputMergerState.DepthState.IsDepthWriteEnabled;
    pipeline.DepthFunction = graphicsPipelineDescriptor.OutputMergerState.DepthState.DepthFunction;

    return pipeline;
}

auto CreateComputePipeline(const TComputePipelineDescriptor& computePipelineDescriptor) -> std::expected<TComputePipeline, std::string> {

    PROFILER_ZONESCOPEDN("CreateComputePipeline");

    TComputePipeline pipeline = {};

    auto computeProgramResult = CreateComputeProgram(computePipelineDescriptor.Label,
                                                     computePipelineDescriptor.ComputeShaderFilePath);
    if (!computeProgramResult) {
        return std::unexpected(std::format("RHI: Unable to build ComputePipeline {}\n{}", computePipelineDescriptor.Label, computeProgramResult.error()));
    }

    pipeline.Id = *computeProgramResult;

    return pipeline;
}

auto TGraphicsPipeline::Bind() -> void {

    TPipeline::Bind();
    // Input Assembly
    glBindVertexArray(InputLayout.has_value() ? *InputLayout : g_defaultInputLayout);

    // Rasterizer Stage
    glPolygonMode(GL_FRONT_AND_BACK, FillModeToGL(FillMode));
    if (CullMode == TCullMode::None) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(CullModeToGL(CullMode));
    }

    if (IsRasterizerDisabled) {
        glEnable(GL_RASTERIZER_DISCARD);
    }
    if (IsScissorEnabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glFrontFace(FaceWindingOrderToGL(FaceWindingOrder));
    if (IsDepthBiasEnabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glEnable(GL_POLYGON_OFFSET_POINT);

        glPolygonOffset(DepthBiasSlopeFactor, DepthBiasConstantFactor);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
        glDisable(GL_POLYGON_OFFSET_POINT);
    }

    glLineWidth(LineWidth);
    glPointSize(PointSize);

    // Output Merger Stage

    if (IsDepthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(DepthFunctionToGL(DepthFunction));
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(IsDepthWriteEnabled ? GL_TRUE : GL_FALSE);
    glColorMask((ColorMask & TColorMaskBits::R) == TColorMaskBits::R,
                (ColorMask & TColorMaskBits::G) == TColorMaskBits::G,
                (ColorMask & TColorMaskBits::B) == TColorMaskBits::B,
                (ColorMask & TColorMaskBits::A) == TColorMaskBits::A);
}

auto TGraphicsPipeline::BindBufferAsVertexBuffer(
    const uint32_t buffer,
    const uint32_t bindingIndex,
    const size_t offset,
    const size_t stride) -> void {

    if (InputLayout.has_value()) {
        glVertexArrayVertexBuffer(*InputLayout, bindingIndex, buffer, offset, stride);
    }
}

auto TGraphicsPipeline::DrawArrays(
    const int32_t vertexOffset,
    const size_t vertexCount) -> void {

    glDrawArrays(PrimitiveTopology, vertexOffset, vertexCount);
}

auto TGraphicsPipeline::DrawElements(
    const uint32_t indexBuffer,
    const size_t elementCount) -> void {

    if (g_lastIndexBuffer != indexBuffer) {
        glVertexArrayElementBuffer(InputLayout.has_value() ? InputLayout.value() : g_defaultInputLayout, indexBuffer);
        g_lastIndexBuffer = indexBuffer;
    }

    glDrawElements(PrimitiveTopology, elementCount, GL_UNSIGNED_INT, nullptr);
}

auto TGraphicsPipeline::DrawElementsInstanced(
    const uint32_t indexBuffer,
    const size_t elementCount,
    const size_t instanceCount) -> void {

    if (g_lastIndexBuffer != indexBuffer) {
        glVertexArrayElementBuffer(InputLayout.has_value() ? InputLayout.value() : g_defaultInputLayout, indexBuffer);
        g_lastIndexBuffer = indexBuffer;
    }

    glDrawElementsInstanced(PrimitiveTopology, elementCount, GL_UNSIGNED_INT, nullptr, instanceCount);
}

auto TComputePipeline::Dispatch(
    const int32_t workGroupSizeX,
    const int32_t workGroupSizeY,
    const int32_t workGroupSizeZ) -> void {

    glDispatchCompute(workGroupSizeX, workGroupSizeY, workGroupSizeZ);
}

auto RhiInitialize() -> void {
    g_samplers.reserve(128);
    g_textures.reserve(16384);

    glCreateVertexArrays(1, &g_defaultInputLayout);
    SetDebugLabel(g_defaultInputLayout, GL_VERTEX_ARRAY, "InputLayout-Default");

    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &g_maxTextureAnisotropy);
}

auto RhiShutdown() -> void {
    DeleteTextures();
    glDeleteVertexArrays(1, &g_defaultInputLayout);
}
