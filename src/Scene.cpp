#include "Scene.hpp"
#include "Components.hpp"
#include "Profiler.hpp"
#include "Core.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

#include <format>
#include <optional>

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/trigonometric.hpp>

#include <GLFW/glfw3.h>

namespace Scene {

entt::registry g_registry = {};

entt::entity g_playerEntity = {};
entt::entity g_entity_mars = {};

const glm::vec3 g_unitX = glm::vec3{1.0f, 0.0f, 0.0f};
const glm::vec3 g_unitY = glm::vec3{0.0f, 1.0f, 0.0f};
const glm::vec3 g_unitZ = glm::vec3{0.0f, 0.0f, 1.0f};

auto SceneAddEntity(
    std::optional<entt::entity> parent,
    const std::string& assetMeshName,
    const std::string& assetMaterialName,
    glm::mat4x4 initialTransform) -> entt::entity {

    PROFILER_ZONESCOPEDN(nameof(SceneAddEntity));

    auto entityId = g_registry.create();
    if (parent.has_value()) {
        auto parentComponent = g_registry.get_or_emplace<TComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);
        g_registry.emplace<TComponentChildOf>(entityId, parent.value());
    }

    g_registry.emplace<TComponentName>(entityId, assetMeshName);
    g_registry.emplace<TComponentMesh>(entityId, assetMeshName);
    g_registry.emplace<TComponentMaterial>(entityId, assetMaterialName);
    g_registry.emplace<TComponentTransform>(entityId, initialTransform);
    g_registry.emplace<TComponentCreateGpuResourcesNecessary>(entityId);

    return entityId;
}

auto SceneAddEntity(
    std::optional<entt::entity> parent,
    const std::string& assetName,
    glm::mat4 initialTransform,
    bool overrideMaterial,
    const std::string& materialName) -> std::optional<entt::entity> {

    PROFILER_ZONESCOPEDN(nameof(SceneAddEntity));

    if (!Assets::IsAssetLoaded(assetName)) {
        return std::nullopt;
    }

    auto entityId = g_registry.create();
    if (parent.has_value()) {
        auto& parentComponent = g_registry.get_or_emplace<TComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);

        g_registry.emplace<TComponentChildOf>(entityId, parent.value());
    }

    auto& asset = Assets::GetAsset(assetName);
    for(auto& assetInstanceData : asset.Instances) {

        auto assetMeshName = asset.Meshes[assetInstanceData.MeshIndex];
        auto assetMesh = Assets::GetAssetMeshData(assetMeshName);

        auto assetMaterialName = assetMesh.MaterialIndex.has_value()
            ? overrideMaterial ? materialName : asset.Materials[assetMesh.MaterialIndex.value()]
            : "M_Default";
        auto& worldMatrix = assetInstanceData.WorldMatrix;

        SceneAddEntity(entityId, assetMeshName, assetMaterialName, initialTransform * worldMatrix);
    }

    return entityId;
}

auto Load() -> bool {

    /*
     * Load Assets
     */

    Assets::AddAssetFromFile("Test", "data/default/SM_Deccer_Cubes_Textured.glb");
    Assets::AddAssetFromFile("fform1", "data/basic/fform_1.glb");
    Assets::AddAssetFromFile("fform2", "data/basic/fform_2.glb");
    Assets::AddAssetFromFile("fform3", "data/basic/fform_3.glb");
    Assets::AddAssetFromFile("fform4", "data/basic/fform_4.glb");
    Assets::AddAssetFromFile("fform5", "data/basic/fform_5.glb");
    Assets::AddAssetFromFile("fform6", "data/basic/fform_6.glb");
    Assets::AddAssetFromFile("fform7", "data/basic/fform_7.glb");
    Assets::AddAssetFromFile("fform8", "data/basic/fform_9.glb");
    Assets::AddAssetFromFile("fform9", "data/basic/fform_10.glb");

    //Assets::AddAssetFromFile("SM_Cube_x1_y1_z1", "data/scenes/SM_Cube_x1_y1_z1.glb");
    //Assets::AddAssetFromFile("SM_Cube_x1_y1_z2", "data/scenes/SM_Cube_x1_y1_z2.glb");

    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/Sponza/glTF/Sponza.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/_Random/SM_Cube_OneMaterialPerFace.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Downloads/modular_ruins_c/modular_ruins_c.glb");
    //LoadModelFromFile("SM_Tower", "data/scenes/Tower/scene.gltf");

    /// Setup Scene ////////////

    for (auto i = 1; i < 11; i++) {
        SceneAddEntity(std::nullopt, std::format("SM_Cuboid_x{}_y1_z1", i), glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, i * 2.0f, -10.0f)), false, "");
        SceneAddEntity(std::nullopt, std::format("SM_Cuboid_x1_y{}_z1", i), glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.0f, 0.0f, -5.0f)), false, "");
        SceneAddEntity(std::nullopt, std::format("SM_Cuboid_x1_y1_z{}", i), glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, i * 2.0f, -5.0f)), false, "");
    }

    SceneAddEntity(std::nullopt, "Test", glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -20.0f)), false, "");
    for (auto i = 1; i < 10; i++) {
        SceneAddEntity(std::nullopt, std::format("fform{}", i), glm::translate(glm::mat4(1.0f), glm::vec3(i * 5.0f, 0.0f, 0.0f)), true, "M_Default");
    }

    auto t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -6235072.0f, 0.0f));
    auto s = glm::scale(glm::mat4(1.0f), glm::vec3(6227558));
    g_entity_mars = SceneAddEntity(std::nullopt, "SM_Geodesic", "M_Mars", t * s);

    SceneAddEntity(std::nullopt, "SM_Cuboid_x50_y1_z50", glm::translate(glm::mat4(1.0f), glm::vec3{-50.0f, -5.0f, 0.0f}), false, "");

    g_playerEntity = g_registry.create();
    g_registry.emplace<TComponentCamera>(g_playerEntity, TComponentCamera {
        .Position = {-60.0f, -3.0f, 5.0f},
        .Orientation = glm::identity<glm::quat>(),//glm::angleAxis(glm::radians(-90.f), glm::vec3(0, 1, 0)),
        .FieldOfView = 70.0f,
        .CameraSpeed = 10.0f,
        .Sensitivity = 0.0025f,
    });

    return true;
}

auto Unload() -> void {

}

auto Update(
    TRenderContext& renderContext,
    entt::registry& registry,
    const TInputState& inputState) -> void {

/*
    const auto forward = cameraComponent.GetForwardDirection();
    const auto right = glm::normalize(glm::cross(forward, g_unitY));

    auto tempCameraSpeed = cameraComponent.CameraSpeed;
    if (glfwGetKey(renderContext.Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        tempCameraSpeed *= 4.0f;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        tempCameraSpeed *= 4000.0f;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        tempCameraSpeed *= 0.25f;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraComponent.Position += forward * renderContext.DeltaTimeInSeconds * tempCameraSpeed;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraComponent.Position -= forward * renderContext.DeltaTimeInSeconds * tempCameraSpeed;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraComponent.Position += right * renderContext.DeltaTimeInSeconds * tempCameraSpeed;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraComponent.Position -= right * renderContext.DeltaTimeInSeconds * tempCameraSpeed;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_Q) == GLFW_PRESS) {
        cameraComponent.Position.y -= renderContext.DeltaTimeInSeconds * tempCameraSpeed;
    }
    if (glfwGetKey(renderContext.Window, GLFW_KEY_E) == GLFW_PRESS) {
        cameraComponent.Position.y += renderContext.DeltaTimeInSeconds * tempCameraSpeed;
    }

    if (glfwGetMouseButton(renderContext.Window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {

        cameraComponent.Yaw += static_cast<float>(g_cursorFrameOffset.x * cameraComponent.Sensitivity);
        cameraComponent.Pitch += static_cast<float>(g_cursorFrameOffset.y * cameraComponent.Sensitivity);
        cameraComponent.Pitch = glm::clamp(cameraComponent.Pitch, -glm::half_pi<float>() + 1e-4f, glm::half_pi<float>() - 1e-4f);    
    }
*/
    auto& playerCamera = registry.get<TComponentCamera>(g_playerEntity);

    glm::vec3 forward = playerCamera.Orientation * glm::vec3(0, 0, -1);
    forward.y = 0;
    forward = glm::normalize(forward);

    glm::vec3 right = playerCamera.Orientation * glm::vec3(1, 0, 0);
    right = glm::normalize(right);

    auto tempCameraSpeed = playerCamera.CameraSpeed;
    if (inputState.Keys[INPUT_KEY_LEFT_SHIFT].IsDown) {
        tempCameraSpeed *= 4.0f;
    }
    if (inputState.Keys[INPUT_KEY_LEFT_ALT].IsDown) {
        tempCameraSpeed *= 4000.0f;
    }
    if (inputState.Keys[INPUT_KEY_LEFT_CONTROL].IsDown) {
        tempCameraSpeed *= 0.25f;
    }

    forward *= tempCameraSpeed;
    right *= tempCameraSpeed;
    glm::vec3 up = tempCameraSpeed * glm::vec3(0, 1, 0);

    if (inputState.Keys[INPUT_KEY_W].IsDown) {

        playerCamera.Position += forward;
    }
    if (inputState.Keys[INPUT_KEY_S].IsDown) {
    
        playerCamera.Position -= forward;
    }
    if (inputState.Keys[INPUT_KEY_D].IsDown) {
    
        playerCamera.Position += right;
    }
    if (inputState.Keys[INPUT_KEY_A].IsDown) {
    
        playerCamera.Position -= right;
    }
    if (inputState.Keys[INPUT_KEY_Q].IsDown) {
    
        playerCamera.Position += up;
    }
    if (inputState.Keys[INPUT_KEY_Z].IsDown) {
    
        playerCamera.Position -= up;
    }

    if (inputState.MouseButtons[INPUT_MOUSE_BUTTON_RIGHT].IsDown) {

        playerCamera.Yaw -= inputState.MousePositionDelta.x * playerCamera.Sensitivity;
        playerCamera.Pitch -= inputState.MousePositionDelta.y * playerCamera.Sensitivity;
        playerCamera.Pitch = glm::clamp(playerCamera.Pitch, glm::radians(-89.0f), glm::radians(89.0f));

        auto pitch = glm::angleAxis(playerCamera.Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        auto yaw = glm::angleAxis(playerCamera.Yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        playerCamera.Orientation = yaw * pitch;
    }

    auto& marsTransform = registry.get<TComponentTransform>(g_entity_mars);
    auto mars_t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -6235072.0f, 0.0f));
    auto mars_s = glm::scale(glm::mat4(1.0f), glm::vec3(6227558));
    marsTransform = glm::rotate(mars_t * mars_s, glm::radians(static_cast<float>(renderContext.FrameCounter)) * 0.01f, glm::vec3(0.2f, 0.7f, 0.2f));
}

auto GetRegistry() -> entt::registry& {
    return g_registry;
}

}