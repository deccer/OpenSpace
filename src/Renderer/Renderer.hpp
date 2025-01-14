#pragma once

struct TRenderContext {

};

class TScene;

class TRenderer {
public:
    TRenderer();
    virtual ~TRenderer();

    auto Load() -> bool;
    auto Render(TRenderContext& renderContext, TScene* scene) -> void;
    auto Unload() -> void;
};
