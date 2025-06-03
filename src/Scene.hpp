#pragma once

#include "Renderer.hpp"
#include "Controls.hpp"

class TScene {
public:
    TScene() = default;
    ~TScene() = default;

    auto Load() -> bool;
    auto Unload() -> void;
    auto Update(
        Renderer::TRenderContext& renderContext,
        entt::registry& registry,
        const TControlState& controlState) -> void;
    auto GetRegistry() -> entt::registry&;

    auto CreateEmpty(const std::string& name = "Empty") -> entt::entity;
    auto CreateCamera(const std::string& name = "Camera") -> entt::entity;
    auto CreateModel(
        const std::string& name,
        std::string_view assetModelName) -> entt::entity;
    auto SetParent(
        entt::entity entity,
        entt::entity parent) -> void;
    auto SetPosition(
        entt::entity entity,
        const glm::vec3& position) -> void;
    auto SetOrientation(
        entt::entity entity,
        float pitch,
        float yaw,
        float roll) -> void;
    auto SetScale(
        entt::entity entity,
        const glm::vec3& scale) -> void;

    template<typename TComponent, typename... Args>
    TComponent& AddComponent(entt::entity entity, Args&&... args) {
        return _registry.emplace<TComponent>(entity, std::forward<Args>(args)...);
    }

    template<typename TComponent>
    void RemoveComponent(const entt::entity entity) {
        _registry.remove<TComponent>(entity);
    }

    template<typename TComponent>
    bool HasComponent(const entt::entity entity) const {
        return _registry.all_of<TComponent>(entity);
    }

private:
    auto PlayerControlShip(
        Renderer::TRenderContext& renderContext,
        entt::registry& registry,
        const TControlState& controlState) const -> void;
    auto PlayerControlPlayer(
        Renderer::TRenderContext& renderContext,
        entt::registry& registry,
        const TControlState& controlState) -> void;

    auto CreateMesh(
        const std::string& name,
        const std::string& assetMeshName,
        const std::string& assetMaterialName) -> entt::entity;

    entt::registry _registry = {};
    entt::entity _rootEntity = entt::null;
    entt::entity _cameraEntity = entt::null;
    entt::entity _marsEntity = entt::null;
    entt::entity _shipEntity = entt::null;
    entt::entity _celestialBodySun = entt::null;
    entt::entity _celestialBodyPlanet = entt::null;
    entt::entity _celestialBodyMoon = entt::null;
    entt::entity _landingPadEntity = entt::null;
    entt::entity _playerEntity = entt::null;

    bool _isPlayerMounted = false;
};
