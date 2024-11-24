#pragma once

#include "Renderer.hpp"
#include "Input.hpp"

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

namespace Scene {

auto Load() -> bool;
auto Unload() -> void;
auto Update(
    TRenderContext& renderContext,
    entt::registry& registry,
    const TInputState& inputState) -> void;
auto GetRegistry() -> entt::registry&;

}