#pragma once

#include "Core/Module.hpp"
#include <string>

struct GLFWwindow;
struct IGame;

class TGameHost {
public:
    auto Run() -> void;
protected:
private:

    auto LoadGameModule() -> bool;
    auto UnloadGameModule() -> void;
    auto CheckIfGameModuleNeedsReloading() -> bool;

    GLFWwindow* _window;
    IGame* _game;
    ModuleHandle _gameModule;
    std::string _gameModuleFilePath;
};
