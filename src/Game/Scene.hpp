#pragma once

#include <entt/entt.hpp>

class TScene {
public:
    auto GetRegistry() -> entt::registry&;
private:
    entt::registry _registry;
};
