#pragma once

#include <string>
#include <glm/vec4.hpp>

enum class TAssetMaterialChannelType {
    Color,
    Normal,
    Scalar
};

struct TAssetMaterialChannel {
    TAssetMaterialChannelType Type;
    std::string TextureName;
    std::string SamplerName;
};

struct TAssetMaterial {
    std::string Name;
    TAssetMaterialChannel BaseColorChannel;
    TAssetMaterialChannel NormalChannel;
    TAssetMaterialChannel SpecularChannel;
    TAssetMaterialChannel EmissiveChannel;
    TAssetMaterialChannel ArmChannel;
    TAssetMaterialChannel OcclusionChannel;
    TAssetMaterialChannel RoughnessChannel;
    TAssetMaterialChannel MetalnessChannel;
    bool IsArmSeparateChannels; // if true, use Occlusion/Roughness/Metalness-Channel, else ArmChannel
    glm::vec4 BaseColor;
    float NormalStrength = 1.0f;
    float Metalness = 0.0f;
    float Roughness = 0.0f;
    float EmissiveFactor = 1.0f;
    float IndexOfRefraction = 1.0f;
};
