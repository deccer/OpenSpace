#pragma once

#include "WindowSettings.hpp"
#include "Input.hpp"

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    entt::registry& registry,
    const TInputState& inputState) -> void;
auto RendererResizeWindowFramebuffer(
    int32_t width,
    int32_t height) -> void;
auto RendererToggleEditorMode() -> void;
