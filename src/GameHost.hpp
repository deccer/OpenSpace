#pragma once

#include "Core/Module.hpp"
#include "Game/IGame.hpp"
#include "Renderer/Renderer.hpp"

#include <filesystem>
#include <string>

struct GLFWwindow;
struct TWindowSettings;
struct TInputState;
struct TControlState;

using TFileTime = std::filesystem::file_time_type;

class TGameHost {
public:
    auto Run(TWindowSettings* windowSettings) -> void;

private:
    friend class TGameHostAccess;

    auto Initialize() -> bool;
    auto Load() -> bool;
    auto Unload() -> void;

    auto Render(TRenderContext& renderContext) -> void;
    auto Update(TGameContext& gameContext) -> void;

    auto LoadGameModule() -> bool;
    auto UnloadGameModule() -> void;
    auto CheckIfGameModuleNeedsReloading() -> bool;

    auto OnWindowFocusGained() -> void;
    auto OnWindowFocusLost() -> void;
    auto OnResizeWindowFramebuffer(int32_t framebufferWidth, int32_t framebufferHeight) -> void;

    TWindowSettings* _windowSettings = nullptr;
    TGameContext* _gameContext = nullptr;

    GLFWwindow* _window = nullptr;
    IGame* _game = nullptr;
    TModuleHandle _gameModule = nullptr;
    std::string _gameModuleFilePath = {};
    TFileTime _gameModuleLastModifiedTime = {};
    float _gameModuleChangedCheckInterval = 1.0f;
    float _gameModuleChangedTimeSinceLastCheck = 0.0f;

    TInputState* _inputState = nullptr;
    TControlState* _controlState = nullptr;

    TRenderer* _renderer = nullptr;
};
