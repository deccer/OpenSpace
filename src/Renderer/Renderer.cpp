#include "Renderer.hpp"
#include "../Game/Scene.hpp"

TRenderer::TRenderer() {
}

TRenderer::~TRenderer() {

}

auto TRenderer::Load() -> bool {
    return true;
}

auto TRenderer::Unload() -> void {

}

auto TRenderer::Render(TRenderContext& renderContext, TScene* scene) -> void {

}
