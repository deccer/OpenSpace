#pragma once

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <entt/entt.hpp>

#include <string>
#include <vector>

struct TComponentName {
    std::string Name;
};

struct TComponentPosition : public glm::vec3
{
    using glm::vec3::vec3;
    using glm::vec3::operator=;
    TComponentPosition() = default;
    TComponentPosition(glm::vec3 const& m) : glm::vec3(m) {}
    TComponentPosition(glm::vec3&& m) : glm::vec3(std::move(m)) {}
};

struct TComponentPositionBackup : TComponentPosition {};

/*
struct TComponentOrientation : public glm::quat
{
    using glm::quat::quat;
    using glm::quat::operator=;
    TComponentOrientation() = default;
    TComponentOrientation(glm::quat const& m) : glm::quat(m) {}
    TComponentOrientation(glm::quat&& m) : glm::quat(std::move(m)) {}
};
*/

struct TComponentOrientationEuler {
    float Pitch;
    float Yaw;
    float Roll;
};

struct TComponentScale : public glm::vec3
{
    using glm::vec3::vec3;
    using glm::vec3::operator=;
    TComponentScale() = default;
    TComponentScale(glm::vec3 const& m) : glm::vec3(m) {}
    TComponentScale(glm::vec3&& m) : glm::vec3(std::move(m)) {}
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

struct TComponentPlanet {
    double Radius;
    bool ResourcesCreated;
};

struct TComponentCamera {

    float FieldOfView = 90.0f;
    float CameraSpeed = 2.0f;
    float Sensitivity = 0.0015f;

    bool IsPrimary = false;
};

auto EntityChangeParent(entt::registry& registry, entt::entity entity, entt::entity parent) -> void;
auto EntityGetGlobalTransform(entt::registry& registry, entt::entity entity) -> glm::mat4;