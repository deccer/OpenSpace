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

class TScene;

struct IGame {
    virtual ~IGame() = default;

    virtual auto Load() -> bool = 0;
    virtual auto Update(TGameContext& gameContext) -> void = 0;
    virtual auto Unload() -> void = 0;

    virtual auto GetScene() -> TScene* = 0;
};
