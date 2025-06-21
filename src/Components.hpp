#pragma once

struct TComponentHierarchy {
    auto AddChild(entt::entity child) -> void;
    auto RemoveChild(entt::entity child) -> void;

    entt::entity Parent = entt::null;
    std::vector<entt::entity> Children;
};

struct TComponentName {
    std::string Name;
};

struct TComponentPosition : public glm::vec3 {
    using glm::vec3::vec3;
    using glm::vec3::operator=;
    TComponentPosition() = default;
    TComponentPosition(glm::vec3 const& m) : glm::vec3(m) {}
    TComponentPosition(glm::vec3&& m) : glm::vec3(std::move(m)) {}
};

struct TComponentPositionBackup : TComponentPosition {};

struct TComponentOrientationEuler {
    float Pitch;
    float Yaw;
    float Roll;
};

struct TComponentScale : public glm::vec3 {
    using glm::vec3::vec3;
    using glm::vec3::operator=;
    TComponentScale() = default;
    TComponentScale(glm::vec3 const& m) : glm::vec3(m) {}
    TComponentScale(glm::vec3&& m) : glm::vec3(std::move(m)) {}
};

struct TComponentTransform : public glm::mat4x4 {
    using glm::mat4x4::mat4x4;
    using glm::mat4x4::operator=;
    TComponentTransform() = default;
    TComponentTransform(glm::mat4x4 const& m) : glm::mat4x4(m) {}
    TComponentTransform(glm::mat4x4&& m) : glm::mat4x4(std::move(m)) {}
};

struct TComponentRenderTransform : public glm::mat4x4 {
    using glm::mat4x4::mat4x4;
    using glm::mat4x4::operator=;
    TComponentRenderTransform() = default;
    TComponentRenderTransform(glm::mat4x4 const& m) : glm::mat4x4(m) {}
    TComponentRenderTransform(glm::mat4x4&& m) : glm::mat4x4(std::move(m)) {}
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

struct TComponentPlanet {
    double Radius;
    bool ResourcesCreated;
};

struct TComponentCamera {

    float FieldOfView = 60.0f;
    float CameraSpeed = 2.0f;
    float Sensitivity = 0.0015f;

    bool IsPrimary = false;
};

struct TComponentGlobalLight {
    float Azimuth;
    float Elevation;
    glm::vec3 Color = {1.0f, 1.0f, 1.0f};
    float Intensity = 1.0f;
    bool IsEnabled = false;
    bool CanCastShadows = false;
    bool IsDebugEnabled = true;
};

auto EntityChangeParent(entt::registry& registry, entt::entity entity, entt::entity parent) -> void;
