#include "GameHost.hpp"
#include "WindowSettings.hpp"
#include "Core/Logger.hpp"
#include "Input/Input.hpp"
#include "Input/Controls.hpp"

#include <glad/gl.h>
#include <SDL2/SDL.h>

#include <chrono>

using TCreateGameDelegate = IGame*();
using TClock = std::chrono::high_resolution_clock;

auto TGameHost::Run(TWindowSettings* windowSettings) -> void {

    _windowSettings = std::move(windowSettings);
    TLogger::SetMinLogLevel(TLogLevel::Verbose);

    if (!Initialize()) {
        TLogger::Error("Unable to initialize GameHost");
        return;
    }

    TGameContext gameContext = {};
    TRenderContext renderContext = {};

    if (!std::filesystem::exists(_gameModuleFilePath.data())) {
        TLogger::Error(std::format("Unable to load game module file from {}. It doesn't exist.", _gameModuleFilePath));
        return;
    }
    _gameModuleLastModifiedTime = last_write_time(std::filesystem::path(_gameModuleFilePath.data()));
    if (!LoadGameModule()) {
        TLogger::Error("Unable to load game module during load");
        return;
    }

    auto previousTime = TClock::now();
    while (_gameContext->IsRunning) {
        auto currentTime = TClock::now();
        gameContext.DeltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        HandleEvents();

        Update(gameContext);
        Render(renderContext);

        SDL_GL_SwapWindow(_window);
    }

    Unload();
}

auto TGameHost::Initialize() -> bool {

    _gameModuleFilePath = "./libGame.so";

    _inputState = new TInputState();
    _controlState = new TControlState();

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        TLogger::Error("Unable to initialize SDL2");
        return false;
    }

    auto windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    const auto isWindowWindowed = _windowSettings->WindowStyle == TWindowStyle::Windowed;
    if (isWindowWindowed) {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    } else {
        windowFlags |= _windowSettings->WindowStyle == TWindowStyle::FullscreenExclusive
            ? SDL_WINDOW_FULLSCREEN
            : SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    const auto windowWidth = _windowSettings->ResolutionWidth;
    const auto windowHeight = _windowSettings->ResolutionHeight;
    const auto windowLeft = isWindowWindowed ? SDL_WINDOWPOS_CENTERED : 0;
    const auto windowTop = isWindowWindowed ? SDL_WINDOWPOS_CENTERED : 0;

    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_PROFILE_MASK, SDL_GLprofile::SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_ACCELERATED_VISUAL, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_DOUBLEBUFFER, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_RELEASE_BEHAVIOR, SDL_GLcontextReleaseFlag::SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH);
    auto contextFlags = SDL_GLcontextFlag::SDL_GL_CONTEXT_RESET_ISOLATION_FLAG | SDL_GLcontextFlag::SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG;
    if (_windowSettings->IsDebug) {
        contextFlags |= SDL_GLcontextFlag::SDL_GL_CONTEXT_DEBUG_FLAG;
    }
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_FLAGS, contextFlags);

    _window = SDL_CreateWindow(
        "Tenebrae",
        windowLeft,
        windowTop,
        windowWidth,
        windowHeight,
        windowFlags);
    if (_window == nullptr) {
        TLogger::Error("Unable to create window");
        return false;
    }

    _windowContext = SDL_GL_CreateContext(_window);
    if (_windowContext == nullptr) {
        SDL_DestroyWindow(_window);
        _window = nullptr;
        SDL_Quit();
        TLogger::Error(std::format("Unable to create context {}", SDL_GetError()));
        return false;
    }

    SDL_GL_MakeCurrent(_window, _windowContext);
    if (gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == GL_FALSE) {
        TLogger::Error("Unable to load opengl functions");
        return false;
    }

    _gameContext = new TGameContext {
        .IsRunning = true,
        .IsPaused = false,
        .DeltaTime = 0.0f,
        .ElapsedTime = 0.0f,
        .FramebufferSize = { windowWidth, windowHeight },
        .ScaledFramebufferSize = { windowWidth * _windowSettings->ResolutionScale, windowHeight * _windowSettings->ResolutionScale },
        .FramebufferResized = true,
    };

    _renderer = new TRenderer();

    return true;
}

auto TGameHost::Load() -> bool {

    if (!_renderer->Load()) {
        TLogger::Error("Unable to load renderer");
        return false;
    }

    return true;
}

auto TGameHost::Unload() -> void {

    if (_renderer != nullptr) {
        _renderer->Unload();
        delete _renderer;
        _renderer = nullptr;
    }

    UnloadGameModule();

    SDL_GL_DeleteContext(_windowContext);
    SDL_DestroyWindow(_window);
    SDL_Quit();

    delete _inputState;
    _inputState = nullptr;

    delete _controlState;
    _controlState = nullptr;

    delete _gameContext;
    _gameContext = nullptr;
}

auto TGameHost::Render(TRenderContext& renderContext) -> void {

    _renderer->Render(renderContext, _game->GetScene());
}

auto TGameHost::Update(TGameContext& gameContext) -> void {

#ifdef OPENSPACE_HOTRELOAD_GAME_ENABLED
    _gameModuleChangedTimeSinceLastCheck += gameContext.DeltaTime;
    if (_gameModuleChangedTimeSinceLastCheck >= _gameModuleChangedCheckInterval) {
        if (CheckIfGameModuleNeedsReloading()) {
            UnloadGameModule();
            LoadGameModule();
        }

        _gameModuleChangedTimeSinceLastCheck = 0.0f;
    }
#endif

    if (_game != nullptr) {
        _game->Update(gameContext);
    }
}

auto TGameHost::HandleEvents() -> void {
    SDL_Event event = {};
    SDL_PollEvent(&event);

    if (event.type == SDL_QUIT) {
        _gameContext->IsRunning = false;
    }

    if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            int32_t framebufferWidth = 0;
            int32_t framebufferHeight = 0;
            SDL_GL_GetDrawableSize(_window, &framebufferWidth, &framebufferHeight);
            if (framebufferWidth * framebufferHeight > 0) {
                OnResizeWindowFramebuffer(framebufferWidth, framebufferHeight);
            }
        }

        if (event.window.event == SDL_WINDOWEVENT_ENTER) {
            OnWindowFocusGained();
        }

        if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
            OnWindowFocusLost();
        }
    }

    if (event.type == SDL_KEYDOWN) {

        if (event.key.repeat == 1) {
            return;
        }

        const auto inputKey = KeyCodeToInputKey(event.key.keysym.sym);
        _inputState->Keys[inputKey].JustPressed = true;
        _inputState->Keys[inputKey].IsDown = true;

        if (_inputState->Keys[INPUT_KEY_LEFT].JustPressed) {
            _gameContext->IsRunning = false;
        }

        if (_inputState->Keys[INPUT_KEY_F1].JustPressed) {
            //RendererToggleEditorMode();
        }
    }

    if (event.type == SDL_KEYUP) {

        const auto inputKey = KeyCodeToInputKey(event.key.keysym.sym);
        _inputState->Keys[inputKey].JustReleased = true;
        _inputState->Keys[inputKey].IsDown = false;

    }

    if (event.type == SDL_TEXTEDITING) {

    }

    if (event.type == SDL_TEXTINPUT) {

    }

    if (event.type == SDL_MOUSEMOTION) {

        const glm::vec2 mousePosition = {event.motion.x, event.motion.y};
        const auto mousePositionDelta = mousePosition - _inputState->MousePosition;

        _inputState->MousePositionDelta += mousePositionDelta;
        _inputState->MousePosition = mousePosition;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        _inputState->MouseButtons[event.button.button].JustPressed = true;
        _inputState->MouseButtons[event.button.button].IsDown = true;
    }

    if (event.type == SDL_MOUSEBUTTONUP) {
        _inputState->MouseButtons[event.button.button].JustReleased = true;
        _inputState->MouseButtons[event.button.button].IsDown = false;
    }

    if (event.type == SDL_MOUSEWHEEL) {
        _inputState->ScrollDelta += glm::vec2(event.wheel.mouseX, event.wheel.mouseY);
    }
}


auto TGameHost::LoadGameModule() -> bool {

    _gameModule = LoadModule(_gameModuleFilePath);
    if (_gameModule == nullptr) {
        TLogger::Error("Unable to load game library");
        return false;
    }

    TLogger::Debug("Game Module Loaded");

    auto createGameDelegate = reinterpret_cast<TCreateGameDelegate*>(GetModuleSymbol(_gameModule, "CreateGame"));
    if (createGameDelegate == nullptr) {
        TLogger::Error(std::format("Unable to load create game delegate: {}", dlerror()));
        UnloadModule(_gameModule);
        UnloadModule(_gameModule);
        return false;
    }

    _game = createGameDelegate();
    if (!_game->Load()) {
        TLogger::Error("Unable to load game");
        return false;
    }

    TLogger::Info("Game loaded");

    return true;
}

auto TGameHost::UnloadGameModule() -> void {
    if (_game != nullptr) {
        _game->Unload();
        delete _game;
        _game = nullptr;
        TLogger::Debug("Game Unloaded");
    }

    if (_gameModule != nullptr) {
        UnloadModule(_gameModule);
        UnloadModule(_gameModule);
        _gameModule = nullptr;
        TLogger::Debug("Game Module Unloaded");
    }
}

auto TGameHost::CheckIfGameModuleNeedsReloading() -> bool {

    try {

        std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>> foundFiles;
        auto rootPath = std::filesystem::current_path();
        for (const auto& fileSystemEntry : std::filesystem::directory_iterator(rootPath)) {
            if (!fileSystemEntry.is_regular_file()) {
                continue;
            }
            if (fileSystemEntry.path().extension().string() != ".so") {
                continue;
            }
            if (!fileSystemEntry.path().filename().string().contains("libGame")) {
                continue;
            }

            foundFiles.push_back(std::make_pair(fileSystemEntry.path(), fileSystemEntry.last_write_time()));
        }

        // sort found files by last write date
        auto lastWriteTimeComparator = [](
            std::pair<std::filesystem::path, std::filesystem::file_time_type> a,
            std::pair<std::filesystem::path, std::filesystem::file_time_type> b) {
            return a.second < b.second;
        };
        std::sort(foundFiles.begin(), foundFiles.end(), lastWriteTimeComparator);
        _gameModuleFilePath = foundFiles.begin()->first;

        std::error_code errorCode;
        auto currentModifiedTime = std::filesystem::last_write_time(std::filesystem::path(_gameModuleFilePath.data()), errorCode);
        if (currentModifiedTime != _gameModuleLastModifiedTime) {
            _gameModuleLastModifiedTime = currentModifiedTime;
            TLogger::Debug("Detected change in game library. Reloading...");
            return true;
        }

        return false;
    } catch (const std::filesystem::filesystem_error& e) {
        TLogger::Error(std::format("Error checking file timestamp {}", e.what()));
        return false;
    }
}

auto TGameHost::OnResizeWindowFramebuffer(
    const int32_t framebufferWidth,
    const int32_t framebufferHeight) -> void {

    _gameContext->FramebufferSize = glm::vec2(framebufferWidth, framebufferHeight);
    _gameContext->ScaledFramebufferSize = glm::vec2(framebufferWidth * _windowSettings->ResolutionScale, framebufferHeight * _windowSettings->ResolutionScale);
    _gameContext->FramebufferResized = true;
}

auto TGameHost::OnWindowFocusGained() -> void {

}

auto TGameHost::OnWindowFocusLost() -> void {

}
