#pragma once

#include "../Core/Types.hpp"

struct TGameContext;
struct TRenderContext;

class TScene;

struct ImGuiContext;

class TRenderer {
public:
    TRenderer();
    virtual ~TRenderer();

    auto Load(
        void* window,
        void* windowContext) -> bool;
    auto Render(
        TGameContext* gameContext,
        TRenderContext* renderContext,
        TReference<TScene> scene) -> void;
    auto Unload() -> void;

private:
    ImGuiContext* _imguiContext;
};
