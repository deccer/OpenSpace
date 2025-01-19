#include "Scene.hpp"
#include "Entity.hpp"

TScene::TScene() {

}

TScene::~TScene() {

}

auto TScene::GetRegistry() -> entt::registry& {
    return _registry;
}

auto TScene::CreateEntity(const std::string& name) -> TEntity {

    return CreateEntity(name, static_cast<int32_t>(std::chrono::system_clock::now().time_since_epoch().count()));
}

auto TScene::CreateEntity(const std::string& name, int32_t id) -> TEntity {

    TEntity entity = { _registry.create(), this };
/*
    entity.AddComponent<TTransformComponent>();
    entity.AddComponent<TParentComponent>();
    entity.AddComponent<TVisibilityComponent>();

    auto& nameComponent = entity.AddComponent<TNameComponent>();
    nameComponent.Name = entityName;
    nameComponent.ID = id;

    _entitiesIDMap[id] = entity.GetID();
    _entitiesNameMap[entityName] = entity.GetName();
*/
    return entity;
}

auto TScene::DestroyEntity(TEntity entity) -> TEntity {

    return TEntity();
}

auto TScene::EntityExists(entt::hashed_string name) -> bool {

    //return _entitiesIDMap.find(name) != _entitiesIDMap.end();
    return true;
}
