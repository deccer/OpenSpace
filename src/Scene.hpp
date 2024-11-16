#pragma once

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

auto SceneLoad() -> bool;
auto SceneUnload() -> void;
auto SceneGetRegistry() -> entt::registry&;
