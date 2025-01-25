#pragma once

#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct TAssetMesh {
    std::string Name;
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> Uvs;
    std::vector<glm::vec4> Tangents;

    std::vector<uint32_t> Indices;
    std::string MaterialName;
};
