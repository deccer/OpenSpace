#pragma once

#include "Scene.hpp"

class TEntity {
public:
    TEntity(entt::entity handle, TScene* scene);
    TEntity(const TEntity& entity);
    TEntity();

    auto AddChild(TEntity& entity) -> void;

    auto GetHandle() const -> entt::entity;
    auto GetId() const -> int32_t;

    template<typename Component>
    auto HasComponent() const -> bool {
        return _scene->GetRegistry().all_of<Component>(_entityHandle);
    }

    template<typename Component>
    auto AddComponent() const -> Component& {
        Component& component = _scene->GetRegistry().emplace_or_replace<Component>(_entityHandle);
        return component;
    }

    template<typename Component>
    auto RemoveComponent() const -> void {
        _scene->GetRegistry().remove<Component>(_entityHandle);
    }

    template<typename Component>
    auto GetComponent() const -> Component&
    {
        Component& component = _scene->GetRegistry().get<Component>(_entityHandle);
        return component;
    }

    auto Destroy() -> void;

    auto operator==(const TEntity& other) const -> bool {
        return _entityHandle == other._entityHandle && _scene == other._scene;
    }

    auto operator!=(const TEntity& other) const -> bool {
        return !(*this == other);
    }

private:
    entt::entity _entityHandle;
    TScene* _scene;
};
