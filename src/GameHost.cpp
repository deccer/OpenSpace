#include "GameHost.hpp"
#include "Game/GameContext.hpp"
#include "Renderer/RenderContext.hpp"
#include "Assets/AssetStorage.hpp"
#include "WindowSettings.hpp"
#include "Core/Logger.hpp"
#include "Input/Input.hpp"
#include "Input/Controls.hpp"

#include <glad/gl.h>
#include <SDL2/SDL.h>
#include <imgui_impl_sdl2.h>

#include <chrono>
#include <utility>

using TCreateGameDelegate = IGame*();
using TClock = std::chrono::high_resolution_clock;

constexpr static auto KeyCodeToInputKey(const int32_t keyCode) -> int32_t {
    switch (keyCode) {
        case SDLK_ESCAPE: return INPUT_KEY_ESCAPE;
        case SDLK_F1: return INPUT_KEY_F1;
        case SDLK_F2: return INPUT_KEY_F2;
        case SDLK_F3: return INPUT_KEY_F3;
        case SDLK_F4: return INPUT_KEY_F4;
        case SDLK_F5: return INPUT_KEY_F5;
        case SDLK_F6: return INPUT_KEY_F6;
        case SDLK_F7: return INPUT_KEY_F7;
        case SDLK_F8: return INPUT_KEY_F8;
        case SDLK_F9: return INPUT_KEY_F9;
        case SDLK_F10: return INPUT_KEY_F10;
        case SDLK_F11: return INPUT_KEY_F11;
        case SDLK_F12: return INPUT_KEY_F12;
        case SDLK_PRINTSCREEN: return INPUT_KEY_PRINT_SCREEN;
        case SDLK_SCROLLLOCK: return INPUT_KEY_SCROLL_LOCK;
        case SDLK_PAUSE: return INPUT_KEY_PAUSE;
        case SDLK_INSERT: return INPUT_KEY_INSERT;
        case SDLK_DELETE: return INPUT_KEY_DELETE;
        case SDLK_HOME: return INPUT_KEY_HOME;
        case SDLK_END: return INPUT_KEY_END;
        case SDLK_PAGEUP: return INPUT_KEY_PAGE_UP;
        case SDLK_PAGEDOWN: return INPUT_KEY_PAGE_DOWN;
        case SDLK_LEFT: return INPUT_KEY_LEFT;
        case SDLK_RIGHT: return INPUT_KEY_RIGHT;
        case SDLK_UP: return INPUT_KEY_UP;
        case SDLK_DOWN: return INPUT_KEY_DOWN;

        case SDLK_BACKQUOTE: return INPUT_KEY_GRAVE_ACCENT;
        case SDLK_1: return INPUT_KEY_1;
        case SDLK_2: return INPUT_KEY_2;
        case SDLK_3: return INPUT_KEY_3;
        case SDLK_4: return INPUT_KEY_4;
        case SDLK_5: return INPUT_KEY_5;
        case SDLK_6: return INPUT_KEY_6;
        case SDLK_7: return INPUT_KEY_7;
        case SDLK_8: return INPUT_KEY_8;
        case SDLK_9: return INPUT_KEY_9;
        case SDLK_0: return INPUT_KEY_0;
        case SDLK_MINUS: return INPUT_KEY_MINUS;
        case SDLK_EQUALS: return INPUT_KEY_EQUAL;
        case SDLK_BACKSPACE: return INPUT_KEY_BACKSPACE;
        case SDLK_TAB: return INPUT_KEY_TAB;
        case SDLK_q: return INPUT_KEY_Q;
        case SDLK_w: return INPUT_KEY_W;
        case SDLK_e: return INPUT_KEY_E;
        case SDLK_r: return INPUT_KEY_R;
        case SDLK_t: return INPUT_KEY_T;
        case SDLK_y: return INPUT_KEY_Y;
        case SDLK_u: return INPUT_KEY_U;
        case SDLK_i: return INPUT_KEY_I;
        case SDLK_o: return INPUT_KEY_O;
        case SDLK_p: return INPUT_KEY_P;
        case SDLK_LEFTBRACKET: return INPUT_KEY_LEFT_BRACKET;
        case SDLK_RIGHTBRACKET: return INPUT_KEY_RIGHT_BRACKET;
        case SDLK_BACKSLASH: return INPUT_KEY_BACKSLASH;
        case SDLK_CAPSLOCK: return INPUT_KEY_CAPS_LOCK;
        case SDLK_a: return INPUT_KEY_A;
        case SDLK_s: return INPUT_KEY_S;
        case SDLK_d: return INPUT_KEY_D;
        case SDLK_f: return INPUT_KEY_F;
        case SDLK_g: return INPUT_KEY_G;
        case SDLK_h: return INPUT_KEY_H;
        case SDLK_j: return INPUT_KEY_J;
        case SDLK_k: return INPUT_KEY_K;
        case SDLK_l: return INPUT_KEY_L;
        case SDLK_SEMICOLON: return INPUT_KEY_SEMICOLON;
        case SDLK_QUOTE: return INPUT_KEY_APOSTROPHE;
        case SDLK_RETURN: return INPUT_KEY_ENTER;
        case SDLK_LSHIFT: return INPUT_KEY_LEFT_SHIFT;
        case SDLK_z: return INPUT_KEY_Z;
        case SDLK_x: return INPUT_KEY_X;
        case SDLK_c: return INPUT_KEY_C;
        case SDLK_v: return INPUT_KEY_V;
        case SDLK_b: return INPUT_KEY_B;
        case SDLK_n: return INPUT_KEY_N;
        case SDLK_m: return INPUT_KEY_M;
        case SDLK_COMMA: return INPUT_KEY_COMMA;
        case SDLK_PERIOD: return INPUT_KEY_PERIOD;
        case SDLK_SLASH: return INPUT_KEY_SLASH;
        case SDLK_RSHIFT: return INPUT_KEY_RIGHT_SHIFT;
        case SDLK_LCTRL: return INPUT_KEY_LEFT_CONTROL;
        case SDLK_LGUI: return INPUT_KEY_LEFT_SUPER;
        case SDLK_LALT: return INPUT_KEY_LEFT_ALT;
        case SDLK_SPACE: return INPUT_KEY_SPACE;
        case SDLK_RALT: return INPUT_KEY_RIGHT_ALT;
        case SDLK_RGUI: return INPUT_KEY_RIGHT_SUPER;
        case SDLK_MENU: return INPUT_KEY_MENU;
        case SDLK_RCTRL: return INPUT_KEY_RIGHT_CONTROL;

        case SDLK_NUMLOCKCLEAR: return INPUT_KEY_NUM_LOCK;
        case SDLK_KP_DIVIDE: return INPUT_KEY_KP_DIVIDE;
        case SDLK_KP_MULTIPLY: return INPUT_KEY_KP_MULTIPLY;
        case SDLK_KP_MINUS: return INPUT_KEY_KP_SUBTRACT;
        case SDLK_KP_PLUS: return INPUT_KEY_KP_ADD;
        case SDLK_KP_ENTER: return INPUT_KEY_KP_ENTER;
        case SDLK_KP_1: return INPUT_KEY_KP_1;
        case SDLK_KP_2: return INPUT_KEY_KP_2;
        case SDLK_KP_3: return INPUT_KEY_KP_3;
        case SDLK_KP_4: return INPUT_KEY_KP_4;
        case SDLK_KP_5: return INPUT_KEY_KP_5;
        case SDLK_KP_6: return INPUT_KEY_KP_6;
        case SDLK_KP_7: return INPUT_KEY_KP_7;
        case SDLK_KP_8: return INPUT_KEY_KP_8;
        case SDLK_KP_9: return INPUT_KEY_KP_9;
        case SDLK_KP_0: return INPUT_KEY_KP_0;
        case SDLK_KP_DECIMAL: return INPUT_KEY_KP_DECIMAL;

        default: [[unlikely]] std::unreachable();
    }
}

TGameHost::~TGameHost() {

}

auto TGameHost::Run(TWindowSettings* windowSettings) -> void {

    _windowSettings = std::move(windowSettings);
    TLogger::SetMinLogLevel(TLogLevel::Verbose);

    if (!Initialize()) {
        TLogger::Error("Unable to initialize GameHost");
        return;
    }

    if (!Load()) {
        TLogger::Error("Unable to load GameHost");
        return;
    }

    if (!std::filesystem::exists(_gameModuleFilePath.data())) {
        TLogger::Error(std::format("Unable to load game module file from {}. It doesn't exist.", _gameModuleFilePath));
        return;
    }
    _gameModuleLastModifiedTime = last_write_time(std::filesystem::path(_gameModuleFilePath.data()));
    if (!LoadGameModule()) {
        TLogger::Error("Unable to load game module during load");
        return;
    }

    std::vector<float> frameTimes(512, 60.0f);
    auto accumulatedTimeInSeconds = 0.0;
    auto averageFramesPerSecond = 0.0f;
    auto updateIntervalInSeconds = 1.0f;

    auto previousTime = TClock::now();
    while (_gameContext->IsRunning) {
        auto currentTime = TClock::now();
        _gameContext->DeltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        auto framesPerSecond = 1.0f / _gameContext->DeltaTime;
        previousTime = currentTime;

        accumulatedTimeInSeconds += _gameContext->DeltaTime;

        frameTimes[_gameContext->FrameCounter % frameTimes.size()] = framesPerSecond;
        if (accumulatedTimeInSeconds >= updateIntervalInSeconds) {
            auto summedFrameTime = 0.0f;
            for (auto i = 0; i < frameTimes.size(); i++) {
                summedFrameTime += frameTimes[i];
            }
            averageFramesPerSecond = summedFrameTime / frameTimes.size();
            accumulatedTimeInSeconds = 0.0f;
        }

        _gameContext->FramesPerSecond = static_cast<float>(framesPerSecond);
        _gameContext->AverageFramesPerSecond = averageFramesPerSecond;

        HandleEvents();
        TInputState inputState = *_inputState;
        ResetInputState();
        if (_inputState->IsModified) {
            MapInputStateToControlState(&inputState);
            _inputState->IsModified = false;
        }

        Update();
        Render();

        SDL_GL_SwapWindow(_window);

        _gameContext->FrameCounter++;
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

    //SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_ACCELERATED_VISUAL, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_DOUBLEBUFFER, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, SDL_TRUE);
    //SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_RELEASE_BEHAVIOR, SDL_GLcontextReleaseFlag::SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH);
    auto contextFlags = 0; //SDL_GLcontextFlag::SDL_GL_CONTEXT_RESET_ISOLATION_FLAG | SDL_GLcontextFlag::SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG;
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
        .IsEditor = false,
        .IsDebugUI = false,
        .DeltaTime = 0.0f,
        .ElapsedTime = 0.0f,
        .FramesPerSecond = 0.0f,
        .AverageFramesPerSecond = 0.0f,
        .FrameCounter = 0,
        .FramebufferSize = glm::vec2{ windowWidth, windowHeight },
        .ScaledFramebufferSize = glm::vec2{ windowWidth * _windowSettings->ResolutionScale, windowHeight * _windowSettings->ResolutionScale },
        .FramebufferResized = true,
    };

    _renderContext = new TRenderContext();
    _renderer = CreateScoped<TRenderer>();

    return true;
}

auto TGameHost::Load() -> bool {

    if (!_renderer->Load(static_cast<void*>(_window), _windowContext)) {
        TLogger::Error("Unable to load renderer");
        return false;
    }

    _assetStorage = CreateScoped<TAssetStorage>();

    return true;
}

auto TGameHost::Unload() -> void {

    if (_renderer != nullptr) {
        _renderer->Unload();
    }

    if (_renderContext != nullptr) {
        delete _renderContext;
        _renderContext = nullptr;
    }

    delete _inputState;
    _inputState = nullptr;

    delete _controlState;
    _controlState = nullptr;

    delete _gameContext;
    _gameContext = nullptr;

    UnloadGameModule();

    SDL_GL_DeleteContext(_windowContext);
    SDL_DestroyWindow(_window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}

auto TGameHost::Render() -> void {

    _renderer->Render(
        _gameContext,
        _renderContext,
        _game->GetScene().get());
}

auto TGameHost::Update() -> void {

#ifdef OPENSPACE_HOTRELOAD_GAME_ENABLED
    _gameModuleChangedTimeSinceLastCheck += _gameContext->DeltaTime;
    if (_gameModuleChangedTimeSinceLastCheck >= _gameModuleChangedCheckInterval) {
        if (CheckIfGameModuleNeedsReloading()) {
            UnloadGameModule();
            LoadGameModule();
        }

        _gameModuleChangedTimeSinceLastCheck = 0.0f;
    }
#endif

    if (_game != nullptr) {
        _game->Update(_gameContext);
    }
}

auto TGameHost::HandleEvents() -> void {
    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
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
            _inputState->IsModified = true;

            if (_inputState->Keys[INPUT_KEY_ESCAPE].JustPressed) {
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
            _inputState->IsModified = true;
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
            _inputState->IsModified = true;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN) {
            _inputState->MouseButtons[event.button.button].JustPressed = true;
            _inputState->MouseButtons[event.button.button].IsDown = true;
            _inputState->IsModified = true;
        }

        if (event.type == SDL_MOUSEBUTTONUP) {
            _inputState->MouseButtons[event.button.button].JustReleased = true;
            _inputState->MouseButtons[event.button.button].IsDown = false;
            _inputState->IsModified = true;
        }

        if (event.type == SDL_MOUSEWHEEL) {
            _inputState->ScrollDelta += glm::vec2(event.wheel.mouseX, event.wheel.mouseY);
            _inputState->IsModified = true;
        }
    }
}

auto TGameHost::LoadGameModule() -> bool {

    _gameModule = LoadModule(_gameModuleFilePath);
    if (_gameModule == nullptr) {
#if WIN32
        TLogger::Error(std::format("Unable to load game library: {}", GetLastError()));
#else
        TLogger::Error(std::format("Unable to load game library: {}", dlerror()));
#endif
        return false;
    }

    TLogger::Debug("Game Module Loaded");

    auto createGameDelegate = reinterpret_cast<TCreateGameDelegate*>(GetModuleSymbol(_gameModule, "CreateGame"));
    if (createGameDelegate == nullptr) {
#if WIN32
        TLogger::Error(std::format("Unable to load create game delegate: {}", GetLastError()));
#else
        TLogger::Error(std::format("Unable to load create game delegate: {}", dlerror()));
#endif
        UnloadModule(_gameModule);
        UnloadModule(_gameModule);
        return false;
    }

    _game = createGameDelegate();
    if (!_game->Load(_assetStorage.get())) {
        TLogger::Error("Unable to load game");
        return false;
    }

    auto& model = _assetStorage->GetAssetModel("Test");
    auto& xx = _assetStorage->GetAssetMesh("Test-mesh-0");

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

#ifdef OPENSPACE_HOTRELOAD_GAME_ENABLED
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
#endif

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

auto TGameHost::MapInputStateToControlState(TInputState* inputState) -> void {

    _controlState->Fast = inputState->Keys[INPUT_KEY_LEFT_SHIFT];
    _controlState->Faster = inputState->Keys[INPUT_KEY_LEFT_ALT];
    _controlState->Slow = inputState->Keys[INPUT_KEY_LEFT_CONTROL];

    _controlState->MoveForward = inputState->Keys[INPUT_KEY_W];
    _controlState->MoveBackward = inputState->Keys[INPUT_KEY_S];
    _controlState->MoveLeft = inputState->Keys[INPUT_KEY_A];
    _controlState->MoveRight = inputState->Keys[INPUT_KEY_D];
    _controlState->MoveUp = inputState->Keys[INPUT_KEY_Q];
    _controlState->MoveDown = inputState->Keys[INPUT_KEY_Z];

    _controlState->CursorMode = inputState->Keys[INPUT_KEY_LEFT_SHIFT];
    _controlState->CursorDelta = inputState->MousePositionDelta;
    _controlState->ToggleMount = inputState->Keys[INPUT_KEY_M];

    _controlState->IsFreeLook = inputState->MouseButtons[INPUT_MOUSE_BUTTON_RIGHT].IsDown;
}

auto TGameHost::ResetInputState() -> void {
    for (auto& key : _inputState->Keys)
    {
        key.JustPressed = false;
        key.JustReleased = false;
    }
    for (auto& button : _inputState->MouseButtons)
    {
        button.JustPressed = false;
        button.JustReleased = false;
    }
    _inputState->MousePositionDelta = glm::vec2(0.0f);
    _inputState->ScrollDelta = glm::vec2(0.0f);
}
