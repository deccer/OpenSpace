#pragma once

#include "../Core/Types.hpp"

#include <entt/entt.hpp>
#include <parallel_hashmap/phmap.h>

#include <string>

class TEntity;
using TEntityIdHashMap = phmap::flat_hash_map<uint32_t, uint32_t>;
using TEntityNameHashMap = phmap::flat_hash_map<std::string, uint32_t>;

class TScene final {
public:
    TScene();
    ~TScene();

    auto GetRegistry() -> entt::registry&;


    auto CreateEntity(const std::string& name) -> TEntity;
    auto CreateEntity(const std::string& name, int id) -> TEntity;
    auto DestroyEntity(TEntity entity) -> TEntity;
    auto EntityExists(entt::hashed_string name) -> bool;

private:
    entt::registry _registry = {};

    TEntityIdHashMap _entitiesIdMap = {};
    TEntityNameHashMap _entitiesNameMap = {};
};
