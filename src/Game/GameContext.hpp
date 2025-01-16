#pragma once

#include <glm/vec2.hpp>

struct TGameContext {
    bool IsRunning;
    bool IsPaused;
    float DeltaTime;
    float ElapsedTime;
    float FramesPerSecond;
    float AverageFramesPerSecond;
    uint64_t FrameCounter;

    glm::vec2 FramebufferSize;
    glm::vec2 ScaledFramebufferSize;
    bool FramebufferResized;
};
