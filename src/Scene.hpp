#pragma once

#include "Renderer.hpp"

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

extern bool g_cursorIsActive;
extern float g_cursorSensitivity;
extern glm::dvec2 g_cursorFrameOffset;

auto SceneLoad() -> bool;
auto SceneUnload() -> void;
auto SceneUpdate(TRenderContext& renderContext, entt::registry& registry) -> void;
auto SceneGetRegistry() -> entt::registry&;
