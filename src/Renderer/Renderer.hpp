#pragma once

#include "../Core/Types.hpp"

struct TGameContext;
struct TRenderContext;
struct ImGuiContext;

class TScene;

class TRenderer final {
public:
    TRenderer();
    ~TRenderer();

    auto Load(
        void* window,
        void* windowContext) -> bool;
    auto Render(
        const TGameContext* gameContext,
        const TRenderContext* renderContext,
        const TScene* scene) -> void;
    auto Unload() -> void;

private:
    void* _window = nullptr;
    void* _windowContext = nullptr;
    ImGuiContext* _imguiContext = nullptr;

    auto RenderImGui(
        const TGameContext* gameContext,
        const TRenderContext* renderContext,
        const TScene* scene) -> void;
    auto LoadImGui() -> bool;
    auto UnloadImGui() -> void;
};
