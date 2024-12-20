#pragma once

#include "Renderer.hpp"
#include "Controls.hpp"

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

namespace Scene {

auto Load() -> bool;
auto Unload() -> void;
auto Update(
    TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& inputState) -> void;
auto GetRegistry() -> entt::registry&;

}