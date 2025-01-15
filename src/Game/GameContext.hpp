#pragma once

#include <glm/vec2.hpp>

struct TGameContext {
    bool IsRunning;
    bool IsPaused;
    float DeltaTime;
    float ElapsedTime;
    glm::vec2 FramebufferSize;
    glm::vec2 ScaledFramebufferSize;
    bool FramebufferResized;
};
