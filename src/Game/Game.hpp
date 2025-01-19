#pragma once

#include "IGame.hpp"

class TGame final : public IGame {
public:
    ~TGame() override = default;

    auto Load(const TScoped<TAssetProvider>& assetProvider) -> bool override;
    auto Update(TGameContext* gameContext) -> void override;
    auto Unload() -> void override;

    auto GetScene() -> TReference<TScene> override;
private:
    TReference<TScene> _scene = nullptr;
};

extern "C" auto CreateGame() -> IGame*;
