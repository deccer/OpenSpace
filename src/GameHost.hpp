#pragma once

#include "Core/Module.hpp"
#include "Game/IGame.hpp"

#include <filesystem>
#include <string>

struct GLFWwindow;

using TFileTime = std::filesystem::file_time_type;

class TGameHost {
public:
    auto Run() -> void;

private:
    auto Initialize() -> bool;
    auto Load() -> bool;
    auto Unload() -> void;

    auto Render() -> void;
    auto Update(TGameContext& gameContext) -> void;

    auto LoadGameModule() -> bool;
    auto UnloadGameModule() -> void;
    auto CheckIfGameModuleNeedsReloading() -> bool;

    GLFWwindow* _window = nullptr;
    IGame* _game = nullptr;
    TModuleHandle _gameModule = nullptr;
    std::string _gameModuleFilePath = {};
    TFileTime _gameModuleLastModifiedTime = {};
    float _gameModuleChangedCheckInterval = 1.0f;
    float _gameModuleChangedTimeSinceLastCheck = 0.0f;
};
