#pragma once

#include "../Core/Types.hpp"

struct TGameContext;

class TScene;

struct IGame {
    virtual ~IGame() = default;

    virtual auto Load() -> bool = 0;
    virtual auto Update(TGameContext* gameContext) -> void = 0;
    virtual auto Unload() -> void = 0;

    virtual auto GetScene() -> TReference<TScene> = 0;
};
