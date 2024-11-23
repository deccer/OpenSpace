#pragma once

#include "Renderer.hpp"

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

extern glm::dvec2 g_cursorFrameOffset;

auto SceneLoad() -> bool;
auto SceneUnload() -> void;
auto SceneUpdate(TRenderContext& renderContext, entt::registry& registry) -> void;
auto SceneGetRegistry() -> entt::registry&;
