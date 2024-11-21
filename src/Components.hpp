#pragma once

#include <glm/mat4x4.hpp>
#include <entt/entt.hpp>

#include <string>
#include <vector>

struct TComponentName {
    std::string Name;
};

struct TComponentTransform : public glm::mat4x4
{
    using glm::mat4x4::mat4x4;
    using glm::mat4x4::operator=;
    TComponentTransform() = default;
    TComponentTransform(glm::mat4x4 const& m) : glm::mat4x4(m) {}
    TComponentTransform(glm::mat4x4&& m) : glm::mat4x4(std::move(m)) {}
};

struct TComponentParent {
    std::vector<entt::entity> Children;
};

struct TComponentChildOf {
    entt::entity Parent;
};

struct TComponentMesh {
    std::string Mesh;
};

struct TComponentMaterial {
    std::string Material;
};

struct TComponentCreateGpuResourcesNecessary {
};

struct TComponentGpuMesh {
    std::string GpuMesh;
};

struct TComponentGpuMaterial {
    std::string GpuMaterial;
};
