#pragma once

#include <string>
#include <glm/mat4x4.hpp>

struct TAssetMeshInstance {
    std::string MeshName;
    std::string MaterialName;
    glm::mat4 Transform;
};
