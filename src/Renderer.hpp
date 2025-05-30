#pragma once

#include "WindowSettings.hpp"

struct GLFWwindow;

namespace Renderer {
    struct TRenderContext {
        GLFWwindow* Window;
        float DeltaTimeInSeconds;
        float FramesPerSecond;
        float FramesPerSecond1P;
        float FramesPerSecond01P;
        float AverageFramesPerSecond;
        uint64_t FrameCounter;
    };

    auto Initialize(
        const TWindowSettings& windowSettings,
        GLFWwindow* window,
        const glm::vec2& initialFramebufferSize) -> bool;
    auto Unload() -> void;
    auto Render(
        TRenderContext& renderContext,
        entt::registry& registry) -> void;
    auto ResizeWindowFramebuffer(
        int32_t width,
        int32_t height) -> void;
    auto ToggleEditorMode() -> void;
}
