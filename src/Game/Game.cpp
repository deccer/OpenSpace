#include "Game.hpp"

#include "../Core/Logger.hpp"
#include <format>

#include <glad/gl.h>

float timer = 0.0f;
float duration = 1.0f;

auto TGame::Load() -> bool {
    TLogger::Warning("Loading Game");
    return true;
}

auto TGame::Update(TGameContext& gameContext) -> void {
    timer += gameContext.DeltaTime;
    if (timer >= duration) {
        TLogger::Warning(std::format("Hello from Game v12 {}", gameContext.DeltaTime));
        timer = 0.0f;
    }

    glClearColor(0.2f, 0.6f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

auto TGame::Unload() -> void {
    TLogger::Warning("Unloading Game");
}

extern "C" auto CreateGame() -> IGame* {
    return new TGame();
}
