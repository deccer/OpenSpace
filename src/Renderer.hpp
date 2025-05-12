#pragma once

#include "WindowSettings.hpp"

struct GLFWwindow;

struct TRenderContext {
    GLFWwindow* Window;
    float DeltaTimeInSeconds;
    float FramesPerSecond;
    float AverageFramesPerSecond;
    uint64_t FrameCounter;
};

auto RendererInitialize(
    const TWindowSettings& windowSettings,
    GLFWwindow* window,
    const glm::vec2& initialFramebufferSize) -> bool;
auto RendererUnload() -> void;
auto RendererRender(
    TRenderContext& renderContext,
    entt::registry& registry) -> void;
auto RendererResizeWindowFramebuffer(
    int32_t width,
    int32_t height) -> void;
auto RendererToggleEditorMode() -> void;
