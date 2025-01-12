#include "GameHost.hpp"
#include "IGame.hpp"
#include "Module.hpp"
#include "Logger.hpp"

#include <dlfcn.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <sys/stat.h>
#include <chrono>
#include <filesystem>

using TCreateGameDelegate = IGame*();
using TClock = std::chrono::high_resolution_clock;

std::filesystem::file_time_type g_lastModifiedTime;

auto TGameHost::Run() -> void {

    _gameModuleFilePath = "./libGame.so";

    glfwSetErrorCallback([](int32_t errorCode, const char* errorDescription) -> void {
        TLogger::Error(std::format("Glfw error {} {}", errorCode, errorDescription));
    });
    if (glfwInit() == GLFW_FALSE) {
        TLogger::Error("Unable to initialize glfw");
        return;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_FLUSH);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_NO_ROBUSTNESS);

    _window = glfwCreateWindow(1920, 1080, "OpenSpace", nullptr, nullptr);
    if (_window == nullptr) {
        TLogger::Error("Unable to create window");
        return;
    }

    glfwMakeContextCurrent(_window);
    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        TLogger::Error("Unable to load opengl functions");
        return;
    }

    g_lastModifiedTime = std::filesystem::last_write_time(std::filesystem::path(_gameModuleFilePath.data()));
    if (!LoadGameModule()) {
        TLogger::Error("Unable to load game module during load");
        return;
    }

    TGameContext gameContext = {};

    float gameLibraryChangedCheckInterval = 1.0f;
    float gameLibraryChangedTimeSinceLastCheck = 0.0f;

    auto previousTime = TClock::now();
    while (!glfwWindowShouldClose(_window)) {
        auto currentTime = TClock::now();
        gameContext.DeltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        gameLibraryChangedTimeSinceLastCheck += gameContext.DeltaTime;
        if (gameLibraryChangedTimeSinceLastCheck >= gameLibraryChangedCheckInterval) {
            if (CheckIfGameModuleNeedsReloading()) {
                UnloadGameModule();
                LoadGameModule();
            }
    
            gameLibraryChangedTimeSinceLastCheck = 0.0f;
        }


        glfwPollEvents();

        if (_game != nullptr) {
            _game->Update(gameContext);
        }

        glfwSwapBuffers(_window);
    }

    UnloadGameModule();

    glfwDestroyWindow(_window);
    glfwTerminate();
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
        if (currentModifiedTime != g_lastModifiedTime) {
            g_lastModifiedTime = currentModifiedTime;
            TLogger::Debug("Detected change in game library. Reloading...");
            return true;
        }

        return false;
    } catch (const std::filesystem::filesystem_error& e) {
        TLogger::Error(std::format("Error checking file timestamp {}", e.what()));
        return false;
    }
}