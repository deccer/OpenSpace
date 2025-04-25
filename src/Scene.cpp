#include "Scene.hpp"
#include "Components.hpp"
#include "Profiler.hpp"
#include "Core.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Input.hpp"
#include "entt/entity/fwd.hpp"

#include <format>
#include <optional>

#include <glm/ext/scalar_constants.hpp>
#include "glm/gtc/quaternion.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    entt::entity parent,
    const std::string& name,
    glm::vec3 position,
    glm::vec3 orientation,
    glm::vec3 scale) -> entt::entity {

    auto entity = g_registry.create();
    auto& parentComponent = g_registry.get_or_emplace<TComponentParent>(parent);
    parentComponent.Children.push_back(entity);

    g_registry.emplace<TComponentName>(entity, name);
    g_registry.emplace<TComponentPosition>(entity, position);
    g_registry.emplace<TComponentOrientationEuler>(entity, TComponentOrientationEuler {
        .Pitch = orientation.x,
        .Yaw = orientation.y,
        .Roll = orientation.z
    });
    g_registry.emplace<TComponentScale>(entity, scale);
    g_registry.emplace<TComponentTransform>(entity, glm::mat4(1.0f));
    g_registry.emplace<TComponentChildOf>(entity, parent);

    return entity;
}

auto CreateEntityWithGraphics(
    entt::entity parent,
    const std::string& name,
    glm::vec3 position,
    glm::vec3 orientation,
    glm::vec3 scale,
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
    entt::entity parent,
    const std::string& name,
    glm::vec3 position,
    glm::vec3 orientation,
    glm::vec3 scale,
    const std::string& assetModelName,
    const std::string& assetMaterialNameOverride) -> entt::entity {

    using TVisitNodesDelegate = std::function<void(entt::entity, const Assets::TAssetModelNode&)>;
    TVisitNodesDelegate visitNodes = [&](entt::entity parentEntity, const Assets::TAssetModelNode& assetModelNode) -> void {

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
                    localPosition,
                    localRotationEuler,
                    localScale,
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
    auto entity = CreateEntity(parent, name, position, orientation, scale);
    for (const auto& assetModelNode : assetModel.Hierarchy) {
        visitNodes(entity, assetModelNode);
    }

    return entity;
}

auto Load() -> bool {

    /*
     * Load Assets
     */


    //Assets::AddAssetModelFromFile("axes", "data/default/SM_Deccer_Cubes_Textured_Complex.gltf");
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

    //Assets::AddAssetModelFromFile("axes", "data/scenes/survival_guitar_backpack.glb");
    //Assets::AddAssetModelFromFile("axes", "data/default/SM_Axis_Names.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/hintze-hall_-_vr_tour-1k.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/IntelSponzaCurtains/NewSponza_Curtains_glTF.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/IntelSponza/NewSponza_Main_glTF_002.gltf");
    //Assets::AddAssetModelFromFile("LessSillyShip", "/home/deccer/Code/scenes/GearboxAssy.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/Buggy.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/SM_Hierarchy_2_Empty.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/SM_Hierarchy_2_Empty_2.gltf");

    //Assets::AddAssetModelFromFile("axes", "data/scenes/SillyShip/SM_Ship6.gltf");
    Assets::AddAssetModelFromFile("LessSillyShip", "data/basic/SM_DemonShip.glb");
    Assets::AddAssetModelFromFile("SM_Cube_x1_y1_z1", "data/basic/SM_Cube_x1_y1_z1.glb");
    //Assets::AddAssetModelFromFile("SM_Cube_x1_y1_z2", "data/scenes/SM_Cube_x1_y1_z2.glb");

    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Storage/Resources/Models/UnitySponza/UnitySponza.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/OrientationTest.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/SM_UnitCube_NoYUp.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/frigate.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/helghast_rifleman.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/illegal_in_67_countries.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/l_m_mule_model_to_3_d_printer.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/low_poly_hero_ship.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/low_poly_humanoid_robot.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/low-poly_ship_practice.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/mechanical_shapes.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/monigote_hombre__base_male_humanoid.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/ring_ship.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/sponza_textured_separated.glb"); // draco
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Storage/Resources/Models/_Random/SM_Cube_OneMaterialPerFace.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/free_3d_modular_low_poly_assets_for_prototyping.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/Cornel_holes_white.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/abstract_shapes.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/calacona_santa_muerte.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/hover_bike_-_the_rocket.glb");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/SM_Cube_TwoPrimitives_2.gltf");
    //Assets::AddAssetModelFromFile("axes", "/home/deccer/Code/scenes/structure.glb");
    //Assets::AddAssetModelFromFile("SM_Tower", "data/scenes/Tower/scene.gltf");

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

    g_landingPadEntity = CreateAssetEntity(g_rootEntity, "Landing Pad", glm::vec3{-50.0f, -5.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{50.0f, 1.0f, 50.0f}, "SM_Cube_x1_y1_z1", "");
    g_shipEntity = CreateAssetEntity(g_rootEntity, "SillyShip", glm::vec3{-70.0f, 0.0f, -10.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}, "LessSillyShip", "");

    auto& rootChildren = g_registry.get_or_emplace<TComponentParent>(g_rootEntity);
    rootChildren.Children.push_back(g_playerEntity);

    return true;
}

auto Unload() -> void {

}

auto PlayerControlShip(
    TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    auto& playerCamera = registry.get<TComponentCamera>(g_playerEntity);
    auto& playerCameraOrientationEuler = registry.get<TComponentOrientationEuler>(g_playerEntity);
    auto& playerCameraPosition = registry.get<TComponentPosition>(g_playerEntity);

    auto& shipPosition = registry.get<TComponentPosition>(g_shipEntity);
    auto& shipOrientationEuler = registry.get<TComponentOrientationEuler>(g_shipEntity);
    glm::quat shipOrientation = glm::eulerAngleYX(shipOrientationEuler.Yaw, shipOrientationEuler.Pitch);
    glm::quat playerCameraOrientation = glm::eulerAngleYX(playerCameraOrientationEuler.Yaw, playerCameraOrientationEuler.Pitch);

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
    TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    auto& playerCamera = registry.get<TComponentCamera>(g_playerEntity);
    auto& playerCameraOrientationEuler = registry.get<TComponentOrientationEuler>(g_playerEntity);
    auto& playerCameraPosition = registry.get<TComponentPosition>(g_playerEntity);

    glm::quat playerCameraOrientation = glm::eulerAngleYX(playerCameraOrientationEuler.Yaw, playerCameraOrientationEuler.Pitch);

    glm::vec3 forward = playerCameraOrientation * -g_unitZ;
    forward.y = 0;
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(playerCameraOrientation * g_unitX);
    glm::vec3 up = glm::normalize(playerCameraOrientation * g_unitY);

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

        playerCameraPosition += forward;
    }
    if (controlState.MoveBackward.IsDown) {

        playerCameraPosition -= forward;
    }
    if (controlState.MoveRight.IsDown) {

        playerCameraPosition += right;
    }
    if (controlState.MoveLeft.IsDown) {

        playerCameraPosition -= right;
    }
    if (controlState.MoveUp.IsDown) {

        playerCameraPosition += up;
    }
    if (controlState.MoveDown.IsDown) {

        playerCameraPosition -= up;
    }

    if (controlState.FreeLook) {
        playerCameraOrientationEuler.Yaw -= controlState.CursorDelta.x * playerCamera.Sensitivity;
        playerCameraOrientationEuler.Pitch -= controlState.CursorDelta.y * playerCamera.Sensitivity;
        playerCameraOrientationEuler.Pitch = glm::clamp(playerCameraOrientationEuler.Pitch, -3.1f/2.0f, 3.1f/2.0f);
    }
}

auto Update(
    TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    if (g_playerMounted) {
        PlayerControlShip(renderContext, registry, controlState);
    } else {
        PlayerControlPlayer(renderContext, registry, controlState);
    }

    if (controlState.ToggleMount.JustPressed) {
        g_playerMounted = !g_playerMounted;

        if (g_playerMounted) {

            EntityChangeParent(registry, g_playerEntity, g_shipEntity);

            auto& playerPosition = registry.get<TComponentPosition>(g_playerEntity);
            auto& playerPositionBackup = registry.get<TComponentPositionBackup>(g_playerEntity);
            playerPositionBackup = (TComponentPositionBackup)glm::vec3(playerPosition);

            //playerPosition = glm::vec3{0.0f, 0.4f, -4.66f};
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

    auto shipGlobal = EntityGetGlobalTransform(registry, g_shipEntity);
    glm::vec3 s_scale;
    glm::quat s_orientation;
    glm::vec3 s_translation;
    glm::vec3 s_skew;
    glm::vec4 s_perspective;
    glm::decompose(shipGlobal, s_scale, s_orientation, s_translation, s_skew, s_perspective);

    auto playerGlobal = EntityGetGlobalTransform(registry, g_playerEntity);
    glm::vec3 p_scale;
    glm::quat p_orientation;
    glm::vec3 p_translation;
    glm::vec3 p_skew;
    glm::vec4 p_perspective;
    glm::decompose(playerGlobal, p_scale, p_orientation, p_translation, p_skew, p_perspective);

    //std::printf("ShipGlobalPos: %0.2f, %0.2f, %0.2f\n", s_translation.x, s_translation.y, s_translation.z);
    //std::printf("PlyrGlobalPos: %0.2f, %0.2f, %0.2f\n", p_translation.x, p_translation.y, p_translation.z);
}

auto GetRegistry() -> entt::registry& {
    return g_registry;
}

}
