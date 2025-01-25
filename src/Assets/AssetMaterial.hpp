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
    std::string SamplerName;
    std::string TextureName;
};

struct TAssetMaterial {
    std::string Name;
    TAssetMaterialChannel BaseColorChannel;
    TAssetMaterialChannel NormalChannel;
    TAssetMaterialChannel SpecularChannel;
    TAssetMaterialChannel EmissiveChannel;
    TAssetMaterialChannel ArmChannel;
    TAssetMaterialChannel OcclusionChannel;
    TAssetMaterialChannel MetallicRoughnessChannel;
    bool IsArmPacked; // if true, use Occlusion/MetallicRoughness-Channel, else ArmChannel
    glm::vec4 BaseColor;
    float NormalStrength = 1.0f;
    float Metallic = 0.0f;
    float Roughness = 0.0f;
    glm::vec3 EmissiveFactor = glm::vec3(1.0f);
    float IndexOfRefraction = 1.0f;
    bool IsUnlit = false;
    bool IsDoubleSided = false;
};
