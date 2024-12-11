#include "Components.hpp"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// EntityChangeParent(registry, g_playerEntity, g_Ship) -> move player into ship
// EntityChangeParent(registry, g_playerEntity, g_RootEntity) -> move player out of ship into world

auto EntityChangeParent(
    entt::registry& registry,
    entt::entity entity,
    entt::entity parent) -> void {

    //if (!registry.any_of<TComponentParent>(entity) || !registry.any_of<TComponentChildOf>(parent)) {
    //    return;
    //}

    auto& entityParent = registry.get<TComponentChildOf>(entity);
    auto& entityParentChildren = registry.get<TComponentParent>(entityParent.Parent).Children;
    entityParentChildren.erase(std::remove(entityParentChildren.begin(), entityParentChildren.end(), entity), entityParentChildren.end());

    auto& parentParent = registry.get<TComponentParent>(parent);
    parentParent.Children.push_back(entity);
    entityParent.Parent = parent;
}

auto EntityGetGlobalTransform(entt::registry& registry, entt::entity entity) -> glm::mat4 {

    auto& positionComponent = registry.get<TComponentPosition>(entity);
    auto& orientationComponent = registry.get<TComponentOrientationEuler>(entity);
    auto scaleComponent = registry.all_of<TComponentScale>(entity)
        ? registry.get<TComponentScale>(entity)
        : static_cast<TComponentScale>(glm::vec3{1.0f});

    auto parentTransform = glm::mat4(1.0f);
    if (registry.any_of<TComponentChildOf>(entity)) {
        auto& parentComponent = registry.get<TComponentChildOf>(entity);
        parentTransform = registry.get<TComponentTransform>(parentComponent.Parent);

        glm::vec3 parentPosition;
        glm::quat parentOrientation;
        glm::vec3 parentScale;
        glm::vec3 parentSkew;
        glm::vec4 parentPerspective;

        glm::decompose(parentTransform, parentScale, parentOrientation, parentPosition, parentSkew, parentPerspective);

        parentTransform = glm::translate(glm::mat4(1.0f), parentPosition) * glm::mat4_cast(parentOrientation);
    }

    auto localTranslation = glm::translate(glm::mat4(1.0f), positionComponent);
    auto localOrientation = glm::eulerAngleYXZ(orientationComponent.Yaw, orientationComponent.Pitch, orientationComponent.Roll);
    auto localScale = glm::scale(glm::mat4(1.0f), scaleComponent);
    auto localTransform = localTranslation * localOrientation * localScale;

    return parentTransform * localTransform;
}