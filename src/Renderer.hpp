#pragma once

#include <entt/entt.hpp>

auto RendererInitialize() -> bool;
auto RendererUnload() -> void;
auto RendererRender(entt::registry& registry) -> void;