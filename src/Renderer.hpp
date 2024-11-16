#pragma once

#include "WindowSettings.hpp"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

struct TCamera {

    glm::vec3 Position = {0.0f, 0.0f, 5.0f};
    float Pitch = {};
    float Yaw = {glm::radians(-90.0f)}; // look at 0, 0, -1

    auto GetForwardDirection() -> glm::vec3
    {
        return glm::vec3{cos(Pitch) * cos(Yaw), sin(Pitch), cos(Pitch) * sin(Yaw)};
    }

    auto GetViewMatrix() -> glm::mat4
    {
        return glm::lookAt(Position, Position + GetForwardDirection(), glm::vec3(0, 1, 0));
    }
};

struct TRenderContext {
    float DeltaTimeInSeconds;
    float FramesPerSecond;
    float AverageFramesPerSecond;
    uint64_t FrameCounter;
};

auto RendererInitialize(const TWindowSettings& windowSettings,
    GLFWwindow* window,
    const glm::vec2& initialFramebufferSize) -> bool;
auto RendererUnload() -> void;
auto RendererRender(TRenderContext& renderContext, entt::registry& registry, TCamera& camera) -> void;
auto RendererResizeWindowFramebuffer(int32_t width, int32_t height) -> void;
auto RendererToggleEditorMode() -> void;
