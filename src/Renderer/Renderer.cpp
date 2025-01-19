#include "Renderer.hpp"
#include "RenderContext.hpp"
#include "RHI/RHI.hpp"
#include "../Game/GameContext.hpp"
#include "../Game/Scene.hpp"
#include "../Core/Logger.hpp"

#include <glad/gl.h>
#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>

TRenderer::TRenderer() {
}

TRenderer::~TRenderer() {

}

auto TRenderer::Load(
    void* window,
    void* windowContext) -> bool {

    _window = window;
    _windowContext = windowContext;

    if (!LoadImGui()) {
        return false;
    }

    RhiInitialize();

    return true;
}

auto TRenderer::Unload() -> void {

    UnloadImGui();
    RhiShutdown();
}

auto TRenderer::Render(
    const TGameContext* gameContext,
    const TRenderContext* renderContext,
    const TScene* scene) -> void {

    if (gameContext->IsEditor || gameContext->IsDebugUI) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    // Render

    if (gameContext->IsDebugUI) {
        ImGui::SetNextWindowPos({32, 32});
        ImGui::SetNextWindowSize({168, 212});
        auto windowBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        windowBackgroundColor.w = 0.4f;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
        if (ImGui::Begin("#InGameStatistics", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoDecoration)) {

            //ImGui::PushFont(g_fontJetbrainsMonoRegular);
            ImGui::TextColored(ImVec4{0.9f, 0.7f, 0.0f, 1.0f}, "F1 to toggle editor");
            ImGui::SeparatorText("Frame Statistics");

            ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * gameContext->FramesPerSecond);
            ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * gameContext->FramesPerSecond));
            ImGui::Text("mfps: %.0f", gameContext->AverageFramesPerSecond);
            ImGui::Text("rfps: %.0f", gameContext->FramesPerSecond);
            ImGui::Text("rpms: %.0f", gameContext->FramesPerSecond * 60.0f);
            ImGui::Text("  ft: %.2f ms", gameContext->DeltaTime * 1000.0f);
            ImGui::Text("   f: %lu", gameContext->FrameCounter);
            //ImGui::PopFont();
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    if (gameContext->IsEditor) {
        ImGui::ShowDemoWindow();
    }

    if (gameContext->IsEditor || gameContext->IsDebugUI) {
        ImGui::Render();
        if (auto* imGuiDrawData = ImGui::GetDrawData(); imGuiDrawData != nullptr) {
            glDisable(GL_FRAMEBUFFER_SRGB);
            glViewport(0, 0, gameContext->FramebufferSize.x, gameContext->FramebufferSize.y);
            ImGui_ImplOpenGL3_RenderDrawData(imGuiDrawData);
        }
    }
}

auto TRenderer::RenderImGui(
    const TGameContext* gameContext,
    const TRenderContext* renderContext,
    const TScene* scene) -> void {

}

auto TRenderer::LoadImGui() -> bool {

    _imguiContext = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // this little shit doesn't do anything
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (!ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(_window), _windowContext)) {
        TLogger::Error("ImGui: Unable to initialize the SDL2 backend");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        TLogger::Error("ImGui: Unable to initialize the OpenGL backend");
        return false;
    }

    return true;
}

auto TRenderer::UnloadImGui() -> void {

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(_imguiContext);
}
