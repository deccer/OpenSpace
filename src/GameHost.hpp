#pragma once

#include "Core/Module.hpp"
#include <string>

struct GLFWwindow;
struct IGame;

class TGameHost {
public:
    auto Run() -> void;
private:

    auto LoadGameModule() -> bool;
    auto UnloadGameModule() -> void;
    auto CheckIfGameModuleNeedsReloading() -> bool;

    GLFWwindow* _window = nullptr;
    IGame* _game = nullptr;
    ModuleHandle _gameModule = nullptr;
    std::string _gameModuleFilePath = {};
};
