#include "Scene.hpp"
#include "Components.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>

namespace Scene {

entt::registry g_registry = {};

entt::entity g_rootEntity = entt::null;
entt::entity g_landingPadEntity = entt::null;
entt::entity g_playerEntity = entt::null;
entt::entity g_marsEntity = entt::null;
entt::entity g_shipEntity = entt::null;

entt::entity g_celestialBodySun = entt::null;
entt::entity g_celestialBodyPlanet = entt::null;
entt::entity g_celestialBodyMoon = entt::null;

bool g_playerMounted = false;

constexpr glm::vec3 g_unitX = glm::vec3{1.0f, 0.0f, 0.0f};
constexpr glm::vec3 g_unitY = glm::vec3{0.0f, 1.0f, 0.0f};
constexpr glm::vec3 g_unitZ = glm::vec3{0.0f, 0.0f, 1.0f};

auto CreateEntity(
    const entt::entity parent,
    const std::string& name,
    const glm::vec3 position,
    const glm::vec3 orientation,
    const glm::vec3 scale) -> entt::entity {

    const auto entity = g_registry.create();
    g_registry.emplace<TComponentHierarchy>(entity);

    g_registry.emplace<TComponentName>(entity, name);
    g_registry.emplace<TComponentPosition>(entity, position);
    g_registry.emplace<TComponentOrientationEuler>(entity, TComponentOrientationEuler {
        .Pitch = orientation.x,
        .Yaw = orientation.y,
        .Roll = orientation.z
    });
    g_registry.emplace<TComponentScale>(entity, scale);
    g_registry.emplace<TComponentTransform>(entity, glm::mat4(1.0f));
    g_registry.emplace<TComponentRenderTransform>(entity, glm::mat4(1.0f));

    EntityChangeParent(g_registry, entity, parent);

    return entity;
}

auto CreateEntityWithGraphics(
    const entt::entity parent,
    const std::string& name,
    const glm::vec3 position,
    const glm::vec3 orientation,
    const glm::vec3 scale,
    const std::string& assetPrimitiveName,
    std::optional<std::string_view> overrideAssetMaterialName) -> entt::entity {

    const auto entity = CreateEntity(parent, name, position, orientation, scale);

    const auto& assetPrimitive = Assets::GetAssetPrimitive(assetPrimitiveName);
    const auto& assetMaterialName = overrideAssetMaterialName.has_value()
        ? overrideAssetMaterialName.value().data()
        : assetPrimitive.MaterialName.has_value()
            ? assetPrimitive.MaterialName.value()
            : "T-Default";

    g_registry.emplace<TComponentMesh>(entity, assetPrimitiveName);
    g_registry.emplace<TComponentMaterial>(entity, assetMaterialName);
    g_registry.emplace<TComponentCreateGpuResourcesNecessary>(entity);

    return entity;
}

auto CreateAssetEntity(
    const entt::entity parent,
    const std::string& name,
    const glm::vec3 position,
    const glm::vec3 orientation,
    const glm::vec3 scale,
    const std::string& assetModelName,
    const std::string& assetMaterialNameOverride) -> entt::entity {

    using TVisitNodesDelegate = std::function<void(entt::entity, const Assets::TAssetModelNode&)>;
    TVisitNodesDelegate visitNodes = [&](const entt::entity parentEntity, const Assets::TAssetModelNode& assetModelNode) -> void {

        const auto& nodeName = assetModelNode.Name;
        const auto& localPosition = assetModelNode.LocalPosition;
        const auto localRotationEuler = glm::eulerAngles(assetModelNode.LocalRotation);
        const auto& localScale = assetModelNode.LocalScale;

        entt::entity childEntity = entt::null;
        if (assetModelNode.MeshName.has_value()) {
            childEntity = CreateEntity(parentEntity, nodeName, localPosition, localRotationEuler, localScale);
            const auto& assetMesh = Assets::GetAssetMesh(assetModelNode.MeshName.value());
            for (const auto& assetPrimitive : assetMesh.Primitives) {
                CreateEntityWithGraphics(
                    childEntity,
                    assetPrimitive.Name,
                    glm::vec3{0.0f},//localPosition,
                    glm::vec3{0.0f},//localRotationEuler,
                    glm::vec3{1.0f},//localScale,
                    assetPrimitive.Name, assetPrimitive.MaterialName.value_or("M_Default"));
            }

        } else {
            childEntity = CreateEntity(
                parentEntity,
                nodeName,
                localPosition,
                localRotationEuler,
                localScale);
        }

        for (const auto& childAssetModelNode : assetModelNode.Children) {
            visitNodes(childEntity, childAssetModelNode);
        }
    };

    const auto& assetModel = Assets::GetAssetModel(assetModelName);
    const auto entity = CreateEntity(parent, name, position, orientation, scale);
    for (const auto& assetModelNode : assetModel.Hierarchy) {
        visitNodes(entity, assetModelNode);
    }

    return entity;
}

auto Load() -> bool {

    /*
     * Load Assets
     */

    /*
    Assets::AddAssetModelFromFile("fform1", "data/basic/fform_1.glb");
    Assets::AddAssetModelFromFile("fform2", "data/basic/fform_2.glb");
    Assets::AddAssetModelFromFile("fform3", "data/basic/fform_3.glb");
    Assets::AddAssetModelFromFile("fform4", "data/basic/fform_4.glb");
    Assets::AddAssetModelFromFile("fform5", "data/basic/fform_5.glb");
    Assets::AddAssetModelFromFile("fform6", "data/basic/fform_6.glb");
    Assets::AddAssetModelFromFile("fform7", "data/basic/fform_7.glb");
    Assets::AddAssetModelFromFile("fform8", "data/basic/fform_9.glb");
    Assets::AddAssetModelFromFile("fform9", "data/basic/fform_10.glb");
    */

    //Assets::AddAssetModelFromFile("axes", "data/scenes/SillyShip/SM_Ship6.gltf");
    Assets::AddAssetModelFromFile("LessSillyShip", "data/basic/SM_DemonShip.glb");
    Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/SM_Capital_001.glb");
    Assets::AddAssetModelFromFile("SM_Cube_x1_y1_z1", "data/basic/SM_Plane_007.glb");


    /// Setup Scene ////////////
    g_rootEntity = CreateEntity(entt::null, "Root", glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f});

    CreateAssetEntity(
        g_rootEntity,
        "Axes",
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f), "axes", "");

    g_playerEntity = CreateEntity(g_rootEntity, "Player", glm::vec3{-60.0f, -3.0f, 5.0f}, glm::vec3{0.0f, glm::radians(-90.0f), 0.0f}, glm::vec3{1.0f});
    g_registry.emplace<TComponentPositionBackup>(g_playerEntity, glm::vec3{0.0f});
    g_registry.emplace<TComponentCamera>(g_playerEntity, TComponentCamera {
        .FieldOfView = 90.0f,
        .CameraSpeed = 2.0f,
        .Sensitivity = 0.0025f,
    });

    g_landingPadEntity = CreateAssetEntity(g_rootEntity, "Landing Pad", glm::vec3{-50.0f, -5.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 1.0f, 1.0f}, "SM_Cube_x1_y1_z1", "");
    g_shipEntity = CreateAssetEntity(g_rootEntity, "SillyShip", glm::vec3{-70.0f, 0.0f, -10.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}, "LessSillyShip", "");
    CreateAssetEntity(g_rootEntity, "Capital 001", glm::vec3{0.0f, 30.0f, -200.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}, "SM_Capital_001", "M_Orange");

    return true;
}

auto Unload() -> void {

}

auto PlayerControlShip(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    auto& playerCamera = registry.get<TComponentCamera>(g_playerEntity);
    auto& playerCameraOrientationEuler = registry.get<TComponentOrientationEuler>(g_playerEntity);

    auto& shipPosition = registry.get<TComponentPosition>(g_shipEntity);
    auto& shipOrientationEuler = registry.get<TComponentOrientationEuler>(g_shipEntity);
    const glm::quat shipOrientation = glm::eulerAngleYXZ(shipOrientationEuler.Yaw, shipOrientationEuler.Pitch, shipOrientationEuler.Roll);

    glm::vec3 forward = glm::normalize(shipOrientation * -g_unitZ);
    glm::vec3 right = glm::normalize(shipOrientation * g_unitX);
    glm::vec3 up = glm::normalize(shipOrientation * g_unitY);

    auto tempCameraSpeed = playerCamera.CameraSpeed;
    if (controlState.Fast.IsDown) {
        tempCameraSpeed *= 10.0f;
    }
    if (controlState.Faster.IsDown) {
        tempCameraSpeed *= 4000.0f;
    }
    if (controlState.Slow.IsDown) {
        tempCameraSpeed *= 0.125f;
    }

    forward *= tempCameraSpeed;
    right *= tempCameraSpeed;
    up *= tempCameraSpeed;

    if (controlState.MoveForward.IsDown) {

        shipPosition += forward;
    }
    if (controlState.MoveBackward.IsDown) {

        shipPosition -= forward;
    }
    if (controlState.MoveRight.IsDown) {

        shipPosition += right;
    }
    if (controlState.MoveLeft.IsDown) {

        shipPosition -= right;
    }
    if (controlState.MoveUp.IsDown) {

        shipPosition += up;
    }
    if (controlState.MoveDown.IsDown) {

        shipPosition -= up;
    }

    if (controlState.CursorMode.IsDown) {
        playerCameraOrientationEuler.Yaw -= controlState.CursorDelta.x * playerCamera.Sensitivity;
        playerCameraOrientationEuler.Pitch -= controlState.CursorDelta.y * playerCamera.Sensitivity;
        playerCameraOrientationEuler.Pitch = glm::clamp(playerCameraOrientationEuler.Pitch, -3.1f/2.0f, 3.1f/2.0f);
    } else {
        shipOrientationEuler.Pitch -= controlState.CursorDelta.y * playerCamera.Sensitivity;
        shipOrientationEuler.Yaw -= controlState.CursorDelta.x * playerCamera.Sensitivity;
    }
}

auto PlayerControlPlayer(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    auto& playerCamera = registry.get<TComponentCamera>(g_playerEntity);
    auto& playerOrientation = registry.get<TComponentOrientationEuler>(g_playerEntity);
    auto& playerPosition = registry.get<TComponentPosition>(g_playerEntity);

    const glm::quat playerCameraOrientation = glm::eulerAngleYXZ(playerOrientation.Yaw, playerOrientation.Pitch, playerOrientation.Roll);

    auto tempCameraSpeed = playerCamera.CameraSpeed;
    if (controlState.Fast.IsDown) {
        tempCameraSpeed *= 10.0f;
    }
    if (controlState.Faster.IsDown) {
        tempCameraSpeed *= 4000.0f;
    }
    if (controlState.Slow.IsDown) {
        tempCameraSpeed *= 0.125f;
    }

    glm::vec3 forward = playerCameraOrientation * -g_unitZ;
    forward.y = 0;
    forward = glm::normalize(forward) * tempCameraSpeed;

    const glm::vec3 right = glm::normalize(playerCameraOrientation * g_unitX) * tempCameraSpeed;
    const glm::vec3 up = glm::normalize(playerCameraOrientation * g_unitY) * tempCameraSpeed;

    if (controlState.MoveForward.IsDown) {
        playerPosition += forward;
    }
    if (controlState.MoveBackward.IsDown) {
        playerPosition -= forward;
    }
    if (controlState.MoveRight.IsDown) {
        playerPosition += right;
    }
    if (controlState.MoveLeft.IsDown) {
        playerPosition -= right;
    }
    if (controlState.MoveUp.IsDown) {
        playerPosition += up;
    }
    if (controlState.MoveDown.IsDown) {
        playerPosition -= up;
    }

    if (controlState.FreeLook) {
        playerOrientation.Yaw -= controlState.CursorDelta.x * playerCamera.Sensitivity;
        playerOrientation.Pitch -= controlState.CursorDelta.y * playerCamera.Sensitivity;
        playerOrientation.Pitch = glm::clamp(playerOrientation.Pitch, -3.1f/2.0f, 3.1f/2.0f);
    }
}

auto Update(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    if (controlState.ToggleMount.JustPressed) {
        g_playerMounted = !g_playerMounted;

        if (g_playerMounted) {

            EntityChangeParent(registry, g_playerEntity, g_shipEntity);

            auto& playerPosition = registry.get<TComponentPosition>(g_playerEntity);
            registry.replace<TComponentPositionBackup>(g_playerEntity, playerPosition);

            playerPosition = glm::vec3{0.0f};
            auto& playerOrientation = registry.get<TComponentOrientationEuler>(g_playerEntity);
            playerOrientation.Pitch = 0.0f;
            playerOrientation.Yaw = glm::radians(0.0f);
            playerOrientation.Roll = 0.0f;

        } else {

            EntityChangeParent(registry, g_playerEntity, g_rootEntity);

            auto& playerPositionBackup = registry.get<TComponentPositionBackup>(g_playerEntity);
            auto& playerPosition = registry.get<TComponentPosition>(g_playerEntity);

            playerPosition = playerPositionBackup;
        }
    }

    if (g_playerMounted) {
        PlayerControlShip(renderContext, registry, controlState);
    } else {
        PlayerControlPlayer(renderContext, registry, controlState);
    }
}

auto GetRegistry() -> entt::registry& {
    return g_registry;
}

}
