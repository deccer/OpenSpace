#pragma once

struct TGameContext {
    bool IsPaused;
    float DeltaTime;
    float ElapsedTime;
};

struct IGame {
    virtual ~IGame() = default;

    virtual auto Load() -> bool = 0;
    virtual void Update(TGameContext& gameContext) = 0;
    virtual void Unload() = 0;
};
