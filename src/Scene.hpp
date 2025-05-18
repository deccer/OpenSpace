#pragma once

#include "Renderer.hpp"
#include "Controls.hpp"

namespace Scene {

    auto Load() -> bool;
    auto Unload() -> void;
    auto Update(
        Renderer::TRenderContext& renderContext,
        entt::registry& registry,
        const TControlState& inputState) -> void;
    auto GetRegistry() -> entt::registry&;
}
