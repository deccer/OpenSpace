#include "Scene.hpp"
#include <entt/entt.hpp>

auto TScene::GetRegistry() -> entt::registry& {
    return _registry;
}
