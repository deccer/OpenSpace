#include "Game.hpp"
#include "GameContext.hpp"

#include "../Core/Logger.hpp"
#include "Entity.hpp"
#include "Scene.hpp"

#include "Components/TransformComponent.hpp"

#include <format>

float timer = 0.0f;
float duration = 1.0f;

auto TGame::Load(IAssetImporter* assetImporter) -> bool {

    _scene = CreateReference<TScene>();

    const auto modelResult = assetImporter->ImportModel("Test", "data/Default/SM_Deccer_Cubes_Textured.gltf");
    if (!modelResult) {
        TLogger::Error(modelResult.error());
        return false;
    }

    auto model = *modelResult;
    TLogger::Info(std::format("Loaded Model: {}", model));

    TLogger::Warning("Loading Game");
    return true;
}

auto TGame::Update(TGameContext* gameContext) -> void {
    timer += gameContext->DeltaTime;
    if (timer >= duration) {
        TLogger::Warning(std::format("Hello from Game {}", gameContext->DeltaTime));
        timer = 0.0f;
    }

    //auto entity = _scene->CreateEntity("Foo");
    //entity.AddComponent<TTransformComponent>();
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
