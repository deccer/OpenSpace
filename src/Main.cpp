//#include <mimalloc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <chrono>
#include <vector>

#include <spdlog/spdlog.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <entt/entt.hpp>

#include "Core.hpp"
#include "Profiler.hpp"
#include "Scene.hpp"

#include "Renderer.hpp"
#include "WindowSettings.hpp"

// - Engine -------------------------------------------------------------------

struct TMeshInstance {
    glm::mat4 WorldMatrix;
};

// - Renderer -----------------------------------------------------------------

using TGpuMeshId = TId<struct GpuMeshId>;
using TGpuSamplerId = TId<struct GpuSamplerId>;
using TGpuMaterialId = TId<struct GpuMaterialId>;

TIdGenerator<TGpuMeshId> g_gpuMeshCounter = {};

// - Game ---------------------------------------------------------------------

float g_cameraSpeed = 4.0f;

entt::registry g_gameRegistry = {};

// - Application --------------------------------------------------------------

constexpr ImVec2 g_imvec2UnitX = ImVec2(1, 0);
constexpr ImVec2 g_imvec2UnitY = ImVec2(0, 1);

GLFWwindow* g_window = nullptr;

bool g_sleepWhenWindowHasNoFocus = true;
bool g_windowHasFocus = false;
bool g_cursorJustEntered = false;

glm::dvec2 g_cursorPosition = {};

// - Implementation -----------------------------------------------------------

auto OnWindowKey(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t key,
    [[maybe_unused]] int32_t scancode,
    const int32_t action,
    [[maybe_unused]] int32_t mods) -> void {

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        RendererToggleEditorMode();
    }
}

auto OnWindowCursorEntered(
    [[maybe_unused]] GLFWwindow* window,
    int entered) -> void {

    if (entered) {
        g_cursorJustEntered = true;
    }

    g_windowHasFocus = entered == GLFW_TRUE;
}

auto OnWindowCursorPosition(
    [[maybe_unused]] GLFWwindow* window,
    double cursorPositionX,
    double cursorPositionY) -> void {

    if (g_cursorJustEntered)
    {
        g_cursorPosition = {cursorPositionX, cursorPositionY};
        g_cursorJustEntered = false;
    }        

    g_cursorFrameOffset += glm::dvec2{cursorPositionX - g_cursorPosition.x, g_cursorPosition.y - cursorPositionY};
    g_cursorPosition = {cursorPositionX, cursorPositionY};
}

auto OnWindowFramebufferSizeChanged(
    [[maybe_unused]] GLFWwindow* window,
    const int width,
    const int height) -> void {

    RendererResizeWindowFramebuffer(width, height);
}

auto main(
    [[maybe_unused]] int32_t argc,
    [[maybe_unused]] char* argv[]) -> int32_t {

    PROFILER_ZONESCOPEDN("main");
    auto previousTimeInSeconds = glfwGetTime();

    TWindowSettings windowSettings = {
        .ResolutionWidth = 1920,
        .ResolutionHeight = 1080,
        .ResolutionScale = 1.0f,
        .WindowStyle = TWindowStyle::Windowed,
        .IsDebug = true,
        .IsVSyncEnabled = true,
    };

    if (glfwInit() == GLFW_FALSE) {
        spdlog::error("Glfw: Unable to initialize");
        return 0;
    }

    /*
     * Setup Application
     */
    const auto isWindowWindowed = windowSettings.WindowStyle == TWindowStyle::Windowed;
    glfwWindowHint(GLFW_DECORATED, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (windowSettings.IsDebug) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    const auto primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        spdlog::error("Glfw: Unable to get the primary monitor");
        return 0;
    }

    const auto* primaryMonitorVideoMode = glfwGetVideoMode(primaryMonitor);
    const auto screenWidth = primaryMonitorVideoMode->width;
    const auto screenHeight = primaryMonitorVideoMode->height;

    const auto windowWidth = windowSettings.ResolutionWidth;
    const auto windowHeight = windowSettings.ResolutionHeight;

    auto monitor = windowSettings.WindowStyle == TWindowStyle::FullscreenExclusive
        ? primaryMonitor
        : nullptr;

    g_window = glfwCreateWindow(windowWidth, windowHeight, "How To Script", monitor, nullptr);
    if (g_window == nullptr) {
        spdlog::error("Glfw: Unable to create a window");
        return 0;
    }

    int32_t monitorLeft = 0;
    int32_t monitorTop = 0;
    glfwGetMonitorPos(primaryMonitor, &monitorLeft, &monitorTop);
    if (isWindowWindowed) {
        glfwSetWindowPos(g_window, screenWidth / 2 - windowWidth / 2 + monitorLeft, screenHeight / 2 - windowHeight / 2 + monitorTop);
    } else {
        glfwSetWindowPos(g_window, monitorLeft, monitorTop);
    }

    glfwSetKeyCallback(g_window, OnWindowKey);
    glfwSetCursorPosCallback(g_window, OnWindowCursorPosition);
    glfwSetCursorEnterCallback(g_window, OnWindowCursorEntered);    
    glfwSetFramebufferSizeCallback(g_window, OnWindowFramebufferSizeChanged);
    glfwSetInputMode(g_window, GLFW_CURSOR, true ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    int32_t windowFramebufferWidth = 0;
    int32_t windowFramebufferHeight = 0;
    glfwGetFramebufferSize(g_window, &windowFramebufferWidth, &windowFramebufferHeight);
    OnWindowFramebufferSizeChanged(g_window, windowFramebufferWidth, windowFramebufferHeight);

    glfwMakeContextCurrent(g_window);

    auto scaledFramebufferSize = glm::vec2(windowFramebufferWidth, windowFramebufferHeight) * windowSettings.ResolutionScale;

    if (!RendererInitialize(windowSettings, g_window, scaledFramebufferSize)) {
        spdlog::error("Renderer: Unable to initialize");
        return 0;
    }

    if (!SceneLoad()) {
        spdlog::error("Scene: Unable to load");
        return 0;
    }

    std::vector<float> frameTimes(512, 60.0f);

    /*
    uint32_t g_gridTexture;

    int32_t iconWidth = 0;
    int32_t iconHeight = 0;
    int32_t iconComponents = 0;

    auto gridTexturePixels = LoadImageFromFile("data/basic/T_Grid_1024_D.png", &iconWidth, &iconHeight, &iconComponents);
    if (gridTexturePixels != nullptr) {
        glCreateTextures(GL_TEXTURE_2D, 1, &g_gridTexture);
        auto mipLevels = static_cast<int32_t>(1 + glm::floor(glm::log2(glm::max(static_cast<float>(iconWidth), static_cast<float>(iconHeight)))));
        glTextureStorage2D(g_gridTexture, mipLevels, GL_SRGB8_ALPHA8, iconWidth, iconHeight);
        glTextureSubImage2D(g_gridTexture, 0, 0, 0, iconWidth, iconHeight, GL_RGBA, GL_UNSIGNED_BYTE, gridTexturePixels);
        glGenerateTextureMipmap(g_gridTexture);
        FreeImage(gridTexturePixels);
    }
    */
    
    /// Start Render Loop ///////

    auto accumulatedTimeInSeconds = 0.0;
    auto averageFramesPerSecond = 0.0f;
    auto updateInterval = 1.0f;

    TRenderContext renderContext = {};
    renderContext.Window = g_window;

    while (!glfwWindowShouldClose(g_window)) {

        PROFILER_ZONESCOPEDN("Frame");
        auto currentTimeInSeconds = glfwGetTime();
        auto deltaTimeInSeconds = currentTimeInSeconds - previousTimeInSeconds;
        auto framesPerSecond = 1.0f / deltaTimeInSeconds;
        accumulatedTimeInSeconds += deltaTimeInSeconds;

        frameTimes[renderContext.FrameCounter % frameTimes.size()] = framesPerSecond;
        if (accumulatedTimeInSeconds >= updateInterval) {
            auto sum = 0.0f;
            for (auto i = 0; i < frameTimes.size(); i++) {
                sum += frameTimes[i];
            }
            averageFramesPerSecond = sum / frameTimes.size();
            accumulatedTimeInSeconds = 0.0f;
        }

        renderContext.DeltaTimeInSeconds = static_cast<float>(deltaTimeInSeconds);
        renderContext.FramesPerSecond = static_cast<float>(framesPerSecond);
        renderContext.AverageFramesPerSecond = averageFramesPerSecond;

        auto isCursorActive = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE;
        glfwSetInputMode(g_window, GLFW_CURSOR, isCursorActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

        if (!isCursorActive) {
            glfwSetCursorPos(g_window, 0, 0);
            g_cursorPosition.x = 0;
            g_cursorPosition.y = 0;
        }

        auto& registry = SceneGetRegistry();
        SceneUpdate(renderContext, registry);
        RendererRender(renderContext, registry);

        g_cursorFrameOffset = {0.0, 0.0};

        {
            PROFILER_ZONESCOPEDN("SwapBuffers");
            glfwSwapBuffers(g_window);
            renderContext.FrameCounter++;
        }

#ifdef USE_PROFILER
        TracyGpuCollect
        FrameMark;
#endif

        glfwPollEvents();

        if (!g_windowHasFocus && g_sleepWhenWindowHasNoFocus) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        previousTimeInSeconds = currentTimeInSeconds;
    }

    /// Cleanup Resources //////

    SceneUnload();
    RendererUnload();

    return 0;
}
