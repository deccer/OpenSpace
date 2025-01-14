#include "GameHost.hpp"
#include "WindowSettings.hpp"
#include "Core/Logger.hpp"
#include "Input/Input.hpp"
#include "Input/Controls.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <chrono>

using TCreateGameDelegate = IGame*();
using TClock = std::chrono::high_resolution_clock;

class TGameHostAccess {
public:
    static auto OnWindowKey(
        [[maybe_unused]] GLFWwindow* window,
        const int32_t key,
        [[maybe_unused]] int32_t scancode,
        const int32_t action,
        [[maybe_unused]] int32_t mods) -> void {

        const auto* gameHost = reinterpret_cast<TGameHost*>(glfwGetWindowUserPointer(window));
        if (gameHost == nullptr) {
            TLogger::Fatal("TGameHostAccess::OnWindowKey: window is null");
            return;
        }
        if (action == GLFW_REPEAT) {
            return;
        }
        if (action == GLFW_PRESS)
        {
            gameHost->_inputState->Keys[key].JustPressed = true;
            gameHost->_inputState->Keys[key].IsDown = true;
        }
        else if (action == GLFW_RELEASE) {
            gameHost->_inputState->Keys[key].JustReleased = true;
            gameHost->_inputState->Keys[key].IsDown = false;
        }

        if (gameHost->_inputState->Keys[INPUT_KEY_ESCAPE].JustPressed) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (gameHost->_inputState->Keys[INPUT_KEY_F1].JustPressed) {
            //RendererToggleEditorMode();
        }
    }

    static auto OnWindowCursorEntered(
        [[maybe_unused]] GLFWwindow* window,
        const int32_t entered) -> void {

        auto* gameHost = reinterpret_cast<TGameHost*>(glfwGetWindowUserPointer(window));
        if (gameHost == nullptr) {
            TLogger::Fatal("TGameHostAccess::OnWindowKey: window is null");
            return;
        }
        if (entered) {
            gameHost->OnWindowFocusGained();
        } else {
            gameHost->OnWindowFocusLost();
        }
    }

    static auto OnWindowMouseMove(
        [[maybe_unused]] GLFWwindow* window,
        const double mousePositionX,
        const double mousePositionY) -> void {

        const auto* gameHost = reinterpret_cast<TGameHost*>(glfwGetWindowUserPointer(window));
        if (gameHost == nullptr) {
            TLogger::Fatal("TGameHostAccess::OnWindowKey: window is null");
            return;
        }

        const glm::vec2 mousePosition = {mousePositionX, mousePositionY};
        const auto mousePositionDelta = mousePosition - gameHost->_inputState->MousePosition;

        gameHost->_inputState->MousePositionDelta += mousePositionDelta;
        gameHost->_inputState->MousePosition = mousePosition;
    }

    static auto OnWindowMouseClick(
        [[maybe_unused]] GLFWwindow* window,
        const int32_t button,
        const int32_t action,
        [[maybe_unused]] int32_t modifier) {

        const auto* gameHost = reinterpret_cast<TGameHost*>(glfwGetWindowUserPointer(window));
        if (gameHost == nullptr) {
            TLogger::Fatal("TGameHostAccess::OnWindowKey: window is null");
            return;
        }

        if (action == GLFW_PRESS)
        {
            gameHost->_inputState->MouseButtons[button].JustPressed = true;
            gameHost->_inputState->MouseButtons[button].IsDown = true;
        }
        else if (action == GLFW_RELEASE)
        {
            gameHost->_inputState->MouseButtons[button].JustReleased = true;
            gameHost->_inputState->MouseButtons[button].IsDown = false;
        }
    }

    static auto OnWindowMouseScroll(
        [[maybe_unused]] GLFWwindow* window,
        const double offsetX,
        const double offsetY) {

        const auto* gameHost = reinterpret_cast<TGameHost*>(glfwGetWindowUserPointer(window));
        if (gameHost == nullptr) {
            TLogger::Fatal("TGameHostAccess::OnWindowKey: window is null");
            return;
        }
        gameHost->_inputState->ScrollDelta += glm::vec2(offsetX, offsetY);
    }

    static auto OnWindowFramebufferSizeChanged(
        [[maybe_unused]] GLFWwindow* window,
        const int32_t width,
        const int32_t height) -> void {

        auto* gameHost = reinterpret_cast<TGameHost*>(glfwGetWindowUserPointer(window));
        if (gameHost == nullptr) {
            TLogger::Fatal("TGameHostAccess::OnWindowKey: window is null");
            return;
        }

        gameHost->OnResizeWindowFramebuffer(width, height);
    }
};

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
    while (!glfwWindowShouldClose(_window)) {
        auto currentTime = TClock::now();
        gameContext.DeltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        glfwPollEvents();

        Update(gameContext);
        Render(renderContext);

        glfwSwapBuffers(_window);
    }

    Unload();
}

auto TGameHost::Initialize() -> bool {

    _gameModuleFilePath = "./libGame.so";

    _inputState = new TInputState();
    _controlState = new TControlState();
    _gameContext = new TGameContext();

    glfwSetErrorCallback([](int32_t errorCode, const char* errorDescription) -> void {
        TLogger::Error(std::format("Glfw error {} {}", errorCode, errorDescription));
    });
    if (glfwInit() == GLFW_FALSE) {
        TLogger::Error("Unable to initialize glfw");
        return false;
    }

    const auto isWindowWindowed = _windowSettings->WindowStyle == TWindowStyle::Windowed;
    glfwWindowHint(GLFW_DECORATED, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_FLUSH);
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_NO_ROBUSTNESS);
    if (_windowSettings->IsDebug) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    const auto primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        TLogger::Error("Glfw: Unable to get the primary monitor");
        return false;
    }

    const auto* primaryMonitorVideoMode = glfwGetVideoMode(primaryMonitor);
    const auto screenWidth = primaryMonitorVideoMode->width;
    const auto screenHeight = primaryMonitorVideoMode->height;

    const auto windowWidth = _windowSettings->ResolutionWidth;
    const auto windowHeight = _windowSettings->ResolutionHeight;

    const auto monitor = _windowSettings->WindowStyle == TWindowStyle::FullscreenExclusive
        ? primaryMonitor
        : nullptr;

    _window = glfwCreateWindow(windowWidth, windowHeight, "OpenSpace", monitor, nullptr);
    if (_window == nullptr) {
        TLogger::Error("Unable to create window");
        return false;
    }

    glfwSetWindowUserPointer(_window, this);

    int32_t monitorLeft = 0;
    int32_t monitorTop = 0;
    glfwGetMonitorPos(primaryMonitor, &monitorLeft, &monitorTop);
    if (isWindowWindowed) {
        glfwSetWindowPos(_window, screenWidth / 2 - windowWidth / 2 + monitorLeft, screenHeight / 2 - windowHeight / 2 + monitorTop);
    } else {
        glfwSetWindowPos(_window, monitorLeft, monitorTop);
    }

    glfwSetKeyCallback(_window, TGameHostAccess::OnWindowKey);
    glfwSetCursorEnterCallback(_window, TGameHostAccess::OnWindowCursorEntered);
    glfwSetCursorPosCallback(_window, TGameHostAccess::OnWindowMouseMove);
    glfwSetMouseButtonCallback(_window, TGameHostAccess::OnWindowMouseClick);
    glfwSetScrollCallback(_window, TGameHostAccess::OnWindowMouseScroll);
    glfwSetFramebufferSizeCallback(_window, TGameHostAccess::OnWindowFramebufferSizeChanged);

    glfwMakeContextCurrent(_window);
    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        TLogger::Error("Unable to load opengl functions");
        return false;
    }

    _renderer = new TRenderer();

    return true;
}

auto TGameHost::Load() -> bool {

    if (!_renderer->Load(_)) {
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

    glfwSetKeyCallback(_window, nullptr);
    glfwSetCursorEnterCallback(_window, nullptr);
    glfwSetCursorPosCallback(_window, nullptr);
    glfwSetMouseButtonCallback(_window, nullptr);
    glfwSetScrollCallback(_window, nullptr);
    glfwSetFramebufferSizeCallback(_window, nullptr);

    UnloadGameModule();

    glfwDestroyWindow(_window);
    glfwTerminate();

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

    _gameModuleChangedTimeSinceLastCheck += gameContext.DeltaTime;
    if (_gameModuleChangedTimeSinceLastCheck >= _gameModuleChangedCheckInterval) {
        if (CheckIfGameModuleNeedsReloading()) {
            UnloadGameModule();
            LoadGameModule();
        }

        _gameModuleChangedTimeSinceLastCheck = 0.0f;
    }

    if (_game != nullptr) {
        _game->Update(gameContext);
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

}

auto TGameHost::OnWindowFocusGained() -> void {

}

auto TGameHost::OnWindowFocusLost() -> void {

}
