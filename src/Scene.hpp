#pragma once

#include "Renderer.hpp"

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

extern glm::dvec2 g_cursorFrameOffset;

namespace Scene {

auto Load() -> bool;
auto Unload() -> void;
auto Update(TRenderContext& renderContext, entt::registry& registry) -> void;
auto GetRegistry() -> entt::registry&;

}