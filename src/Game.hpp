#pragma once

#include "IGame.hpp"

class TGame : public IGame {
public:
    auto Load() -> bool override;
    auto Update(TGameContext& gameContext) -> void override;
    auto Unload() -> void override;
};

extern "C" auto CreateGame() -> IGame*;
