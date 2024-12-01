#include "Scene.hpp"
#include "Components.hpp"
#include "Profiler.hpp"
#include "Core.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Input.hpp"
#include "entt/entity/fwd.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include <format>
#include <optional>

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>


#include <GLFW/glfw3.h>

namespace Scene {

entt::registry g_registry = {};

entt::entity g_rootEntity = {};
entt::entity g_playerEntity = {};
entt::entity g_entity_mars = {};
std::optional<entt::entity> g_ship = {};

bool g_playerMounted = false;

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

    g_registry.emplace<TComponentName>(entityId, assetName);

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

auto CreateEntity(
    entt::entity parent,
    const std::string& name,
    glm::vec3 position,
    glm::quat orientation,
    glm::vec3 scale) -> entt::entity {

    auto entity = g_registry.create();
    auto& parentComponent = g_registry.get_or_emplace<TComponentParent>(parent);
    parentComponent.Children.push_back(entity);

    g_registry.emplace<TComponentChildOf>(entity, parent);
    g_registry.emplace<TComponentName>(entity, name);
    g_registry.emplace<TComponentPosition>(entity, position);
    g_registry.emplace<TComponentOrientation>(entity, orientation);
    g_registry.emplace<TComponentScale>(entity, scale);
    g_registry.emplace<TComponentTransform>(entity, glm::mat4(1.0f));

    return entity;
}

auto CreateEntityWithGraphics(
    entt::entity parent,
    const std::string& name,
    glm::vec3 position,
    glm::quat orientation,
    glm::vec3 scale,
    const std::string& assetMeshName,
    const std::string& assetMaterialName) -> entt::entity {

    auto entity = CreateEntity(parent, name, position, orientation, scale);

    g_registry.emplace<TComponentMesh>(entity, assetMeshName);
    g_registry.emplace<TComponentMaterial>(entity, assetMaterialName);
    g_registry.emplace<TComponentCreateGpuResourcesNecessary>(entity);

    return entity;
}

auto CreateAssetEntity(
    entt::entity parent,
    const std::string& name,
    glm::vec3 position,
    glm::quat orientation,
    glm::vec3 scale,
    const std::string& assetName,
    const std::string& assetMaterialNameOverride) -> entt::entity {

    auto entity = CreateEntity(parent, name, position, orientation, scale);
    auto& asset = Assets::GetAsset(assetName);

    for(auto& assetInstanceData : asset.Instances) {

        auto assetMeshName = asset.Meshes[assetInstanceData.MeshIndex];
        auto assetMesh = Assets::GetAssetMeshData(assetMeshName);

        std::string assetMaterialName;
        if (!assetMaterialNameOverride.empty()) {
            assetMaterialName = assetMaterialNameOverride;
        } else {
            assetMaterialName = !assetMesh.MaterialIndex
                ? "M_Default"
                : asset.Materials[*assetMesh.MaterialIndex];
        }

        auto& worldMatrix = assetInstanceData.WorldMatrix;

        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(worldMatrix, scale, orientation, translation, skew, perspective);

        CreateEntityWithGraphics(entity, assetMeshName, translation, orientation, scale, assetMeshName, assetMaterialName);
    }

    return entity;
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

    Assets::AddAssetFromFile("SM_SillyShip", "data/scenes/SillyShip/SM_Ship6.gltf");

    //Assets::AddAssetFromFile("SM_Cube_x1_y1_z1", "data/scenes/SM_Cube_x1_y1_z1.glb");
    //Assets::AddAssetFromFile("SM_Cube_x1_y1_z2", "data/scenes/SM_Cube_x1_y1_z2.glb");

    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/Sponza/glTF/Sponza.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/_Random/SM_Cube_OneMaterialPerFace.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Downloads/modular_ruins_c/modular_ruins_c.glb");
    //LoadModelFromFile("SM_Tower", "data/scenes/Tower/scene.gltf");

    /// Setup Scene ////////////

    g_rootEntity = g_registry.create();
    g_registry.emplace<TComponentTransform>(g_rootEntity, glm::mat4(1.0f));

/*
    for (auto i = 1; i < 11; i++) {
        SceneAddEntity(g_rootEntity, std::format("SM_Cuboid_x{}_y1_z1", i), glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, i * 2.0f, -10.0f)), false, "");
        SceneAddEntity(g_rootEntity, std::format("SM_Cuboid_x1_y{}_z1", i), glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.0f, 0.0f, -5.0f)), false, "");
        SceneAddEntity(g_rootEntity, std::format("SM_Cuboid_x1_y1_z{}", i), glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, i * 2.0f, -5.0f)), false, "");
    }
*/

/*
    SceneAddEntity(g_rootEntity, "Test", glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -20.0f)), false, "");
    for (auto i = 1; i < 10; i++) {
        SceneAddEntity(g_rootEntity, std::format("fform{}", i), glm::translate(glm::mat4(1.0f), glm::vec3(i * 5.0f, 0.0f, 0.0f)), true, "M_Default");
    }
*/
    Assets::TAsset marsAsset;
    marsAsset.Meshes.push_back("SM_Geodesic");
    marsAsset.Materials.push_back("M_Mars");
    marsAsset.Instances.push_back(Assets::TAssetInstanceData {
        .WorldMatrix = glm::mat4(1.0f),
        .MeshIndex = 0,
    });
    Assets::AddAsset("Mars", marsAsset);

    auto marsPosition = glm::vec3(0.0f, -6235072.0f, 0.0f);
    auto marsScale = glm::vec3(6227558);
    auto t = glm::translate(glm::mat4(1.0f), marsPosition);
    auto s = glm::scale(glm::mat4(1.0f), marsScale);
    //g_entity_mars = SceneAddEntity(g_rootEntity, "SM_Geodesic", "M_Mars", t * s);
    g_entity_mars = CreateAssetEntity(g_rootEntity, "Mars", marsPosition, glm::identity<glm::quat>(), marsScale, "Mars", "M_Mars");

    SceneAddEntity(g_rootEntity, "SM_Cuboid_x50_y1_z50", glm::translate(glm::mat4(1.0f), glm::vec3{-50.0f, -5.0f, 0.0f}), false, "");
    g_ship = SceneAddEntity(g_rootEntity, "SM_SillyShip", glm::translate(glm::mat4(1.0f), glm::vec3{-70.0f, -4.0f, -10.0f}), false, "");

    g_playerEntity = g_registry.create();
    g_registry.emplace<TComponentCamera>(g_playerEntity, TComponentCamera {
        .Position = {-60.0f, -3.0f, 5.0f},
        .Orientation = glm::angleAxis(glm::radians(-90.f), glm::vec3(0, 1, 0)),
        .FieldOfView = 70.0f,
        .CameraSpeed = 2.0f,
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

    auto& playerCamera = registry.get<TComponentCamera>(g_playerEntity);

    glm::vec3 forward = playerCamera.Orientation * glm::vec3(0, 0, -1);
    forward.y = 0;
    forward = glm::normalize(forward);

    glm::vec3 right = playerCamera.Orientation * glm::vec3(1, 0, 0);
    right = glm::normalize(right);

    auto tempCameraSpeed = playerCamera.CameraSpeed;
    if (inputState.Keys[INPUT_KEY_LEFT_SHIFT].IsDown) {
        tempCameraSpeed *= 10.0f;
    }
    if (inputState.Keys[INPUT_KEY_LEFT_ALT].IsDown) {
        tempCameraSpeed *= 4000.0f;
    }
    if (inputState.Keys[INPUT_KEY_LEFT_CONTROL].IsDown) {
        tempCameraSpeed *= 0.125f;
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
        playerCamera.Pitch = glm::clamp(playerCamera.Pitch, -3.1f/2.0f, 3.1f/2.0f);

        auto pitch = glm::angleAxis(playerCamera.Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        auto yaw = glm::angleAxis(playerCamera.Yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        playerCamera.Orientation = yaw * pitch;
    }

    if (inputState.Keys[INPUT_KEY_M].IsDown) {
        g_playerMounted = !g_playerMounted;
    }

    if (g_playerMounted) {
        auto& parent = registry.get<TComponentParent>(*g_ship);
        auto& child = parent.Children.front();
        auto& shipTransform = registry.get<TComponentTransform>(child);
        glm::vec3 shipScale;
        glm::vec3 shipTranslation;
        glm::quat shipOrientation;
        glm::vec3 shipSkew;
        glm::vec4 perspective;
        glm::decompose(shipTransform, shipScale, shipOrientation, shipTranslation, shipSkew, perspective);
        playerCamera.Position = shipTranslation + glm::vec3(0.0f, 0.0f, -2.5f);
    }

/*
    auto& marsTransform = registry.get<TComponentTransform>(g_entity_mars);
    auto mars_t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -6235072.0f, 0.0f));
    auto mars_r = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(renderContext.FrameCounter)) * 0.01f, glm::vec3(0.2f, 0.7f, 0.2f));
    auto mars_s = glm::scale(glm::mat4(1.0f), glm::vec3(6227558));
    marsTransform = mars_t * mars_r * mars_s;
*/
    auto& marsRotation = registry.get<TComponentOrientation>(g_entity_mars);
    marsRotation = glm::quat_cast(glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(renderContext.FrameCounter)) * 0.01f, glm::vec3(0.2f, 0.7f, 0.2f)));
}

auto GetRegistry() -> entt::registry& {
    return g_registry;
}

}
