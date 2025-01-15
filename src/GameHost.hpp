#pragma once

#include "Core/Module.hpp"
#include "Game/IGame.hpp"
#include "Renderer/Renderer.hpp"

#include <filesystem>
#include <string>

struct SDL_Window;
struct TWindowSettings;
struct TInputState;
struct TControlState;

using TFileTime = std::filesystem::file_time_type;

class TGameHost {
public:
    auto Run(TWindowSettings* windowSettings) -> void;

private:
    auto Initialize() -> bool;
    auto Load() -> bool;
    auto Unload() -> void;
    auto HandleEvents() -> void;

    auto Render(TRenderContext& renderContext) -> void;
    auto Update(TGameContext& gameContext) -> void;

    auto LoadGameModule() -> bool;
    auto UnloadGameModule() -> void;
#ifdef OPENSPACE_HOTRELOAD_GAME_ENABLED
    auto CheckIfGameModuleNeedsReloading() -> bool;
#endif

    auto OnWindowFocusGained() -> void;
    auto OnWindowFocusLost() -> void;
    auto OnResizeWindowFramebuffer(int32_t framebufferWidth, int32_t framebufferHeight) -> void;

    TWindowSettings* _windowSettings = nullptr;
    TGameContext* _gameContext = nullptr;

    SDL_Window* _window = nullptr;
    void* _windowContext = nullptr;
    IGame* _game = nullptr;
    TModuleHandle _gameModule = nullptr;
    std::string _gameModuleFilePath = {};
    TFileTime _gameModuleLastModifiedTime = {};
#ifdef OPENSPACE_HOTRELOAD_GAME_ENABLED
    float _gameModuleChangedCheckInterval = 1.0f;
    float _gameModuleChangedTimeSinceLastCheck = 0.0f;
#endif

    TInputState* _inputState = nullptr;
    TControlState* _controlState = nullptr;

    TRenderer* _renderer = nullptr;
};
