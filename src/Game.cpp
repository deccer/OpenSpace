#include "Game.hpp"

#include "Core/Logger.hpp"
#include <format>

float timer = 0.0f;
float duration = 1.0f;

auto TGame::Load() -> bool {
    TLogger::Warning("Loading Game");
    return true;
}

auto TGame::Update(TGameContext& gameContext) -> void {
    timer += gameContext.DeltaTime;
    if (timer >= duration) {
        TLogger::Fatal(std::format("Hello from Game v10 {}", gameContext.DeltaTime));
        timer = 0.0f;
    }
}

auto TGame::Unload() -> void {
    TLogger::Warning("Unloading Game");
}

extern "C" auto CreateGame() -> IGame* {
    return new TGame();
}
