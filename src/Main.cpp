#include "Profiler.hpp"
#include "Scene.hpp"
#include "Renderer.hpp"
#include "WindowSettings.hpp"
#include "Input.hpp"
#include "Controls.hpp"

//#include <mimalloc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <entt/entt.hpp>

// - Game ---------------------------------------------------------------------

entt::registry g_gameRegistry = {};

// - Application --------------------------------------------------------------

TInputState g_inputState = {};
GLFWwindow* g_window = nullptr;

bool g_sleepWhenWindowHasNoFocus = true;
bool g_windowHasFocus = false;

// - Implementation -----------------------------------------------------------

auto OnWindowKey(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t key,
    [[maybe_unused]] int32_t scancode,
    const int32_t action,
    [[maybe_unused]] int32_t mods) -> void {

    if (action == GLFW_REPEAT) { 
        return;
    }
    if (action == GLFW_PRESS)
    {
        g_inputState.Keys[key].JustPressed = true;
        g_inputState.Keys[key].IsDown = true;
    }
    else if (action == GLFW_RELEASE) {
        g_inputState.Keys[key].JustReleased = true;
        g_inputState.Keys[key].IsDown = false;
    }

    if (g_inputState.Keys[INPUT_KEY_ESCAPE].JustPressed) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (g_inputState.Keys[INPUT_KEY_F1].JustPressed) {
        Renderer::ToggleEditorMode();
    }
}

auto OnWindowCursorEntered(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t entered) -> void {

    g_windowHasFocus = entered == GLFW_TRUE;
}

auto OnWindowMouseMove(
    [[maybe_unused]] GLFWwindow* window,
    const double mousePositionX,
    const double mousePositionY) -> void {

    const glm::vec2 mousePosition = {mousePositionX, mousePositionY};
    const auto mousePositionDelta = mousePosition - g_inputState.MousePosition;
   
    g_inputState.MousePositionDelta += mousePositionDelta;
    g_inputState.MousePosition = mousePosition;
}

auto OnWindowMouseClick(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t button,
    const int32_t action,
    [[maybe_unused]] int32_t modifier) {

    if (action == GLFW_PRESS)
    {
        g_inputState.MouseButtons[button].JustPressed = true;
        g_inputState.MouseButtons[button].IsDown = true;
    }
    else if (action == GLFW_RELEASE)
    {
        g_inputState.MouseButtons[button].JustReleased = true;
        g_inputState.MouseButtons[button].IsDown = false;
    }
}

auto OnWindowMouseScroll(
    [[maybe_unused]] GLFWwindow* window,
    const double offsetX,
    const double offsetY) {

    g_inputState.ScrollDelta += glm::vec2(offsetX, offsetY);
}

auto OnWindowFramebufferSizeChanged(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t width,
    const int32_t height) -> void {

    Renderer::ResizeWindowFramebuffer(width, height);
}

auto ResetInputState() -> void {

    for (auto& key : g_inputState.Keys)
    {
        key.JustPressed = false;
        key.JustReleased = false;
    }
    for (auto& btn : g_inputState.MouseButtons)
    {
        btn.JustPressed = false;
        btn.JustReleased = false;
    }
    g_inputState.MousePositionDelta = glm::vec2(0.0f);
    g_inputState.ScrollDelta = glm::vec2(0.0f);
}

auto MapInputStateToControlState(
    const TInputState& inputState,
    TControlState& controlState) -> void {

    controlState.Fast = inputState.Keys[INPUT_KEY_LEFT_SHIFT];
    controlState.Faster = inputState.Keys[INPUT_KEY_LEFT_ALT];
    controlState.Slow = inputState.Keys[INPUT_KEY_LEFT_CONTROL];

    controlState.MoveForward = inputState.Keys[INPUT_KEY_W];
    controlState.MoveBackward = inputState.Keys[INPUT_KEY_S];    
    controlState.MoveLeft = inputState.Keys[INPUT_KEY_A];
    controlState.MoveRight = inputState.Keys[INPUT_KEY_D];
    controlState.MoveUp = inputState.Keys[INPUT_KEY_SPACE];
    controlState.MoveDown = inputState.Keys[INPUT_KEY_C];

    controlState.CursorMode = inputState.Keys[INPUT_KEY_LEFT_SHIFT];
    controlState.CursorDelta = inputState.MousePositionDelta;
    controlState.ToggleMount = inputState.Keys[INPUT_KEY_M];

    controlState.FreeLook = inputState.MouseButtons[INPUT_MOUSE_BUTTON_RIGHT].IsDown;
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

    const auto isZink = std::getenv("GALLIUM_DRIVER");
    if (isZink == nullptr) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    } else {
        spdlog::info("Zink detected");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
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

    g_window = glfwCreateWindow(windowWidth, windowHeight, "OpenSpace", monitor, nullptr);
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
    glfwSetCursorEnterCallback(g_window, OnWindowCursorEntered);
    glfwSetCursorPosCallback(g_window, OnWindowMouseMove);
    glfwSetMouseButtonCallback(g_window, OnWindowMouseClick);
    glfwSetScrollCallback(g_window, OnWindowMouseScroll);
    glfwSetFramebufferSizeCallback(g_window, OnWindowFramebufferSizeChanged);

    glfwSetInputMode(g_window, GLFW_CURSOR, false ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(g_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    int32_t windowFramebufferWidth = 0;
    int32_t windowFramebufferHeight = 0;
    glfwGetFramebufferSize(g_window, &windowFramebufferWidth, &windowFramebufferHeight);
    OnWindowFramebufferSizeChanged(g_window, windowFramebufferWidth, windowFramebufferHeight);

    glfwMakeContextCurrent(g_window);

    auto scaledFramebufferSize = glm::vec2(windowFramebufferWidth, windowFramebufferHeight) * windowSettings.ResolutionScale;

    if (!Renderer::Initialize(windowSettings, g_window, scaledFramebufferSize)) {
        spdlog::error("Renderer: Unable to initialize");
        return 0;
    }

    if (!Scene::Load()) {
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
    auto updateIntervalInSeconds = 1.0f;

    Renderer::TRenderContext renderContext = {};
    renderContext.Window = g_window;

    TControlState controlState = {};

    while (!glfwWindowShouldClose(g_window)) {

        PROFILER_ZONESCOPEDN("Frame");

        glfwPollEvents();
        TInputState inputState = g_inputState;
        ResetInputState();
        MapInputStateToControlState(inputState, controlState);

        auto currentTimeInSeconds = glfwGetTime();
        auto deltaTimeInSeconds = currentTimeInSeconds - previousTimeInSeconds;
        auto framesPerSecond = 1.0f / deltaTimeInSeconds;
        accumulatedTimeInSeconds += deltaTimeInSeconds;

        frameTimes[renderContext.FrameCounter % frameTimes.size()] = framesPerSecond;
        if (accumulatedTimeInSeconds >= updateIntervalInSeconds) {
            auto summedFrameTime = 0.0f;
            for (auto i = 0; i < frameTimes.size(); i++) {
                summedFrameTime += frameTimes[i];
            }
            averageFramesPerSecond = summedFrameTime / frameTimes.size();
            accumulatedTimeInSeconds = 0.0f;
        }

        renderContext.DeltaTimeInSeconds = static_cast<float>(deltaTimeInSeconds);
        renderContext.FramesPerSecond = static_cast<float>(framesPerSecond);
        renderContext.AverageFramesPerSecond = averageFramesPerSecond;

        auto& registry = Scene::GetRegistry();
        Scene::Update(renderContext, registry, controlState);
        Renderer::Render(renderContext, registry);

        {
            PROFILER_ZONESCOPEDN("SwapBuffers");
            glfwSwapBuffers(g_window);
            renderContext.FrameCounter++;
        }

#ifdef USE_PROFILER
        TracyGpuCollect
        FrameMark;
#endif
        if (!g_windowHasFocus && g_sleepWhenWindowHasNoFocus) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        previousTimeInSeconds = currentTimeInSeconds;
    }

    /*
     * Cleanup Resources
     */

    Scene::Unload();
    Renderer::Unload();

    glfwSetKeyCallback(g_window, nullptr);
    glfwSetCursorEnterCallback(g_window, nullptr);
    glfwSetCursorPosCallback(g_window, nullptr);
    glfwSetMouseButtonCallback(g_window, nullptr);
    glfwSetScrollCallback(g_window, nullptr);
    glfwSetFramebufferSizeCallback(g_window, nullptr);

    glfwDestroyWindow(g_window);
    glfwTerminate();

    return 0;
}
