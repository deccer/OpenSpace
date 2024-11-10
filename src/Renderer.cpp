#include "Renderer.hpp"
#include "RHI.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

auto RendererInitialize() -> bool {

    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        return false;
    }

    RhiInitialize();

    return true;
}

auto RendererUnload() -> void {

    RhiShutdown();
}

auto RendererRender(entt::registry& registry) -> void {

}