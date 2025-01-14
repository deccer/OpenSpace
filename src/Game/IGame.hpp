#pragma once

struct TGameContext {
    bool IsPaused;
    float DeltaTime;
    float ElapsedTime;
};

class TScene;

struct IGame {
    virtual ~IGame() = default;

    virtual auto Load() -> bool = 0;
    virtual auto Update(TGameContext& gameContext) -> void = 0;
    virtual auto Unload() -> void = 0;

    virtual auto GetScene() -> TScene* = 0;
};
