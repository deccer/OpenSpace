#include "Game.hpp"
#include "GameContext.hpp"

#include "../Core/Logger.hpp"
#include <entt/entt.hpp>
#include "Entity.hpp"
#include "Scene.hpp"

#include <format>

float timer = 0.0f;
float duration = 1.0f;

auto TGame::Load(const TScoped<TAssetProvider>& assetProvider) -> bool {

    _scene = CreateReference<TScene>();

    TLogger::Warning("Loading Game");
    return true;
}

auto TGame::Update(TGameContext* gameContext) -> void {
    timer += gameContext->DeltaTime;
    if (timer >= duration) {
        TLogger::Warning(std::format("Hello from Game v17 {}", gameContext->DeltaTime));
        timer = 0.0f;
    }

    auto entity = _scene->CreateEntity("Foo");
}

auto TGame::Unload() -> void {

    _scene.reset();
    TLogger::Warning("Unloading Game");
}

auto TGame::GetScene() -> TReference<TScene> {
    return _scene;
}

extern "C" auto CreateGame() -> IGame* {
    return new TGame();
}
