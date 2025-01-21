#pragma once

#include "Core/Module.hpp"
#include "Game/IGame.hpp"
#include "Renderer/Renderer.hpp"
#include "Assets/AssetStorage.hpp"

#include <filesystem>
#include <string>

struct SDL_Window;
struct TWindowSettings;

struct TGameHostContext;
struct TRenderContext;
struct TInputState;
struct TControlState;

using TFileTime = std::filesystem::file_time_type;

class TGameHost final {
public:
    ~TGameHost();
    auto Run(TWindowSettings* windowSettings) -> void;

private:
    auto Initialize() -> bool;
    auto Load() -> bool;
    auto Unload() -> void;
    auto HandleEvents() -> void;

    auto Render() -> void;
    auto Update() -> void;

    auto LoadGameModule() -> bool;
    auto UnloadGameModule() -> void;
#ifdef OPENSPACE_HOTRELOAD_GAME_ENABLED
    auto CheckIfGameModuleNeedsReloading() -> bool;
#endif

    auto OnResizeWindowFramebuffer(int32_t framebufferWidth, int32_t framebufferHeight) -> void;
    auto OnWindowFocusGained() -> void;
    auto OnWindowFocusLost() -> void;
    auto MapInputStateToControlState(TInputState* inputState) -> void;
    auto ResetInputState() -> void;

    TWindowSettings* _windowSettings = nullptr;
    TGameContext* _gameContext;
    TRenderContext* _renderContext;
    TScoped<TRenderer> _renderer;
    TScoped<TAssetStorage> _assetStorage;
    TInputState* _inputState = nullptr;
    TControlState* _controlState = nullptr;

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
};
