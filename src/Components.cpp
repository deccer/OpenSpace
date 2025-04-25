#include "Components.hpp"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

auto TComponentTransformComposite::GetGlobalTransform() const -> glm::mat4 {
    return GlobalTransform;
}

auto TComponentTransformComposite::GetGlobalPosition() const -> glm::vec3 {
    return GlobalPosition;
}

auto TComponentTransformComposite::GetGlobalOrientation() const -> glm::quat {
    return GlobalOrientation;
}

auto TComponentTransformComposite::GetGlobalScale() const -> glm::vec3 {
    return GlobalScale;
}

auto TComponentTransformComposite::GetLocalTransform() const -> glm::mat4 {
    return LocalTransform;
}

auto TComponentTransformComposite::GetLocalPosition() const -> glm::vec3 {
    return LocalPosition;
}

auto TComponentTransformComposite::GetLocalOrientation() const -> glm::quat {
    return LocalOrientation;
}

auto TComponentTransformComposite::GetLocalScale() const -> glm::vec3 {
    return LocalScale;
}

auto TComponentTransformComposite::SetGlobalTransform(const glm::mat4& globalTransform) -> void {
    GlobalTransform = globalTransform;
}

auto TComponentTransformComposite::SetGlobalPosition(const glm::vec3& position) -> void {
    GlobalPosition = position;
}

auto TComponentTransformComposite::SetGlobalOrientation(const glm::quat& orientation) -> void {
    GlobalOrientation = orientation;
}

auto TComponentTransformComposite::SetGlobalScale(const glm::vec3& scale) -> void {
    GlobalScale = scale;
}

auto TComponentTransformComposite::SetLocalTransform(const glm::mat4& localTransform) -> void {
    LocalTransform = localTransform;
}

auto TComponentTransformComposite::SetLocalPosition(const glm::vec3& position) -> void {
    LocalPosition = position;
    IsDirty = true;
}

auto TComponentTransformComposite::SetLocalOrientation(const glm::quat& orientation) -> void {
    LocalOrientation = glm::normalize(orientation);
    IsDirty = true;
}

auto TComponentTransformComposite::SetLocalScale(const glm::vec3& scale) -> void {
    LocalScale = scale;
    IsDirty = true;
}

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

    auto localTranslation = glm::translate(glm::mat4(1.0f), positionComponent);
    auto localOrientation = glm::eulerAngleYXZ(orientationComponent.Yaw, orientationComponent.Pitch, orientationComponent.Roll);
    auto localScale = glm::scale(glm::mat4(1.0f), scaleComponent);
    auto localTransform = localTranslation * localOrientation * localScale;

    if (!registry.all_of<TComponentChildOf>(entity)) {
        return localTransform;
    }

    auto& parentComponent = registry.get<TComponentChildOf>(entity);
    if (parentComponent.Parent == entt::null) {
        return localTransform;
    }

    const auto& parentPositionComponent = registry.get<TComponentPosition>(parentComponent.Parent);
    const auto& parentOrientationComponent = registry.get<TComponentOrientationEuler>(parentComponent.Parent);
    const auto& parentScaleComponent = registry.get<TComponentScale>(parentComponent.Parent);

    auto parentTranslation = glm::translate(glm::mat4(1.0f), parentPositionComponent);
    auto parentOrientation = glm::eulerAngleYXZ(parentOrientationComponent.Yaw, parentOrientationComponent.Pitch, parentOrientationComponent.Roll);
    auto parentScale = glm::scale(glm::mat4(1.0f), parentScaleComponent);
    auto parentTransform = parentTranslation * parentOrientation * parentScale;

    return parentTransform * localTransform;
}
