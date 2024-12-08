#include "Components.hpp"

// EntityChangeParent(registry, g_playerEntity, g_Ship) -> move player into ship
// EntityChangeParent(registry, g_playerEntity, g_RootEntity) -> move player out of ship into world

auto EntityChangeParent(
    entt::registry& registry,
    entt::entity entity,
    entt::entity parent) -> void {

    if (!registry.any_of<TComponentParent>(entity) || !registry.any_of<TComponentChildOf>(parent)) {
        return;
    }

    auto& entityParent = registry.get<TComponentChildOf>(entity);
    auto& entityParentChildren = registry.get<TComponentParent>(entityParent.Parent).Children;
    entityParentChildren.erase(std::remove(entityParentChildren.begin(), entityParentChildren.end(), entity), entityParentChildren.end());

    auto& parentParent = registry.get<TComponentParent>(parent);
    parentParent.Children.push_back(entity);
    entityParent.Parent = parent;
}