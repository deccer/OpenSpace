#pragma once

#include "IGame.hpp"

class TGame : public IGame {
public:
    ~TGame() override = default;

    auto Load() -> bool override;
    auto Update(TGameContext* gameContext) -> void override;
    auto Unload() -> void override;

    auto GetScene() -> TReference<TScene> override;
private:
    TReference<TScene> _scene = nullptr;
};

extern "C" auto CreateGame() -> IGame*;
