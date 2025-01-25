#pragma once

#include <string>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct TAssetModelNode {
    std::string Name;
    glm::vec3 LocalPosition;
    glm::quat LocalRotation;
    glm::vec3 LocalScale;
    std::optional<std::string> MeshName;
    std::vector<TAssetModelNode> Children;
};

struct TAssetModel {
    std::string Name;
    std::vector<std::string> Animations;
    std::vector<std::string> Skins;
    std::vector<std::string> Images;
    std::vector<std::string> Samplers;
    std::vector<std::string> Textures;
    std::vector<std::string> Materials;
    std::vector<std::string> Meshes;
    std::vector<TAssetModelNode> Hierarchy;
};
