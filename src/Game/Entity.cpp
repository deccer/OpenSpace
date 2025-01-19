#include "Entity.hpp"
#include "Components/NameComponent.hpp"
#include "Components/ParentComponent.hpp"

TEntity::TEntity(entt::entity handle, TScene* scene) {
    _entityHandle = handle;
    _scene = scene;
}

TEntity::TEntity(const TEntity& entity) {
    _entityHandle = entity._entityHandle;
    _scene = entity._scene;
}

TEntity::TEntity() {
    _entityHandle = entt::null;
    _scene = nullptr;
}

auto TEntity::AddChild(TEntity& entity) -> void {
    if (_entityHandle != entity.GetHandle())
    {
        entity.GetComponent<TParentComponent>().HasParent = true;
        entity.GetComponent<TParentComponent>().Parent = *this;

        GetComponent<TParentComponent>().Children.push_back(entity);
    }
}

auto TEntity::GetHandle() const -> entt::entity {
    return _entityHandle;
}

auto TEntity::GetId() const -> int32_t {
    return GetComponent<TNameComponent>().Id;
}

auto TEntity::Destroy() -> void {
    _scene->GetRegistry().destroy(_entityHandle);
}
