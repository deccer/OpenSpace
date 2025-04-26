#include "Components.hpp"

#include <glm/gtx/euler_angles.hpp>

auto TComponentHierarchy::AddChild(entt::entity child) -> void {
    assert(std::count(Children.begin(), Children.end(), child) == 0);
    Children.emplace_back(child);
}

auto TComponentHierarchy::RemoveChild(entt::entity child) -> void {
    assert(std::count(Children.begin(), Children.end(), child) == 1);
    std::erase(Children, child);
}

auto EntityChangeParent(
    entt::registry& registry,
    entt::entity entity,
    entt::entity parent) -> void {

    auto& entityHierarchy = registry.get<TComponentHierarchy>(entity);
    if (entityHierarchy.Parent != entt::null) {
        auto& oldParentHierarchy = registry.get<TComponentHierarchy>(entityHierarchy.Parent);
        oldParentHierarchy.RemoveChild(entity);
    }

    if (parent != entt::null) {
        auto& newParentHierarchy = registry.get<TComponentHierarchy>(parent);
        newParentHierarchy.AddChild(entity);

        entityHierarchy.Parent = parent;
    } else {
        entityHierarchy.Parent = entt::null;
    }
}
