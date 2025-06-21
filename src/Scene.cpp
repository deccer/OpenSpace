#include "Scene.hpp"
#include "Components.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>

constexpr auto g_unitX = glm::vec3{1.0f, 0.0f, 0.0f};
constexpr auto g_unitY = glm::vec3{0.0f, 1.0f, 0.0f};
constexpr auto g_unitZ = glm::vec3{0.0f, 0.0f, 1.0f};

auto TScene::PlayerControlShip(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) const -> void {

    const auto& playerCamera = registry.get<TComponentCamera>(_playerEntity);
    auto& playerCameraOrientationEuler = registry.get<TComponentOrientationEuler>(_playerEntity);

    auto& shipPosition = registry.get<TComponentPosition>(_shipEntity);
    auto& [Pitch, Yaw, Roll] = registry.get<TComponentOrientationEuler>(_shipEntity);
    const glm::quat shipOrientation = glm::eulerAngleYXZ(Yaw, Pitch, Roll);

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
        Pitch -= controlState.CursorDelta.y * playerCamera.Sensitivity;
        Yaw -= controlState.CursorDelta.x * playerCamera.Sensitivity;
    }
}

auto TScene::PlayerControlPlayer(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    auto& playerCamera = registry.get<TComponentCamera>(_playerEntity);
    auto& playerOrientation = registry.get<TComponentOrientationEuler>(_playerEntity);
    auto& playerPosition = registry.get<TComponentPosition>(_playerEntity);

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

auto TScene::Load() -> bool {

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
    //Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/SM_Capital_001.glb");
    //Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/DamagedHelmet/DamagedHelmet.gltf");
    //Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/DamagedHelmetReExported/DamagedHelmetReExported.glb");
    //Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/Sponza/Sponza.gltf");
    //Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/FlightHelmet/FlightHelmet.gltf");
    //Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/cutlass/cutlass.gltf");
    Assets::AddAssetModelFromFile("SM_Capital_001", "data/basic/RefPbr/scene.gltf");
    Assets::AddAssetModelFromFile("SM_Cube_x1_y1_z1", "data/basic/SM_Plane_007.glb");

    /// Setup Scene ////////////
    _rootEntity = CreateEmpty("Root");
    auto sun1Light = CreateGlobalLight(
        "Sun Purple Light (N)",
        0.0f,
        glm::radians(45.0f),
        glm::vec3{1.0f, 0.0f, 0.5f},
        0.5f);
    SetParent(sun1Light, _rootEntity);
    auto sun2Light = CreateGlobalLight(
        "Sun Blue Light (S)",
        glm::radians(180.0f),
        glm::radians(45.0f),
        glm::vec3{0.0f, 0.0f, 1.0f},
        0.5f);
    SetParent(sun2Light, _rootEntity);

    auto sun3Light = CreateGlobalLight(
        "Sun Green Light (E)",
        glm::radians(90.0f),
        glm::radians(45.0f),
        glm::vec3{0.0f, 1.0f, 0.0f},
        0.5f);
    SetParent(sun3Light, _rootEntity);

    auto sun4Light = CreateGlobalLight(
        "Sun Yellow Light (W)",
        glm::radians(270.0f),
        glm::radians(45.0f),
        glm::vec3{1.0f, 1.0f, 0.0f},
        0.5f);
    SetParent(sun4Light, _rootEntity);

    CreateModel("Axes", "axes");

    _playerEntity = CreateEmpty("Player");
    SetParent(_playerEntity, _rootEntity);
    SetPosition(_playerEntity, glm::vec3{-30.0f, 0.0f, -10.0f});
    SetOrientation(_playerEntity, 0.0f, glm::radians(-90.0f), 0.0f);
    AddComponent<TComponentPositionBackup>(_playerEntity, glm::vec3{0.0f});
    AddComponent<TComponentCamera>(_playerEntity, TComponentCamera {
        .FieldOfView = 90.0f,
        .CameraSpeed = 2.0f,
        .Sensitivity = 0.0025f,
    });

    _landingPadEntity = CreateModel("Landing Pad", "SM_Cube_x1_y1_z1");
    SetParent(_landingPadEntity, _rootEntity);
    SetPosition(_landingPadEntity, glm::vec3{-50.0f, -5.0f, 0.0f});

    _shipEntity = CreateModel("SillyShip", "LessSillyShip");
    SetParent(_shipEntity, _rootEntity);
    SetPosition(_shipEntity, glm::vec3{-70.0f, 0.0f, -10.0f});

    const auto capitalEntity = CreateModel("Capital 001", "SM_Capital_001");
    SetParent(capitalEntity, _rootEntity);
    SetScale(capitalEntity, glm::vec3{90.0f});
    SetPosition(capitalEntity, glm::vec3{0.0f, 30.0f, -200.0f});

    return true;
}

auto TScene::Unload() -> void {

}

auto TScene::Update(
    Renderer::TRenderContext& renderContext,
    entt::registry& registry,
    const TControlState& controlState) -> void {

    if (controlState.ToggleMount.JustPressed) {
        _isPlayerMounted = !_isPlayerMounted;

        if (_isPlayerMounted) {

            SetParent(_playerEntity, _shipEntity);

            auto& playerPosition = registry.get<TComponentPosition>(_playerEntity);
            registry.replace<TComponentPositionBackup>(_playerEntity, playerPosition);

            playerPosition = glm::vec3{0.0f};
            auto& playerOrientation = registry.get<TComponentOrientationEuler>(_playerEntity);
            playerOrientation.Pitch = 0.0f;
            playerOrientation.Yaw = glm::radians(0.0f);
            playerOrientation.Roll = 0.0f;

        } else {

            SetParent(_playerEntity, _rootEntity);

            const auto& playerPositionBackup = registry.get<TComponentPositionBackup>(_playerEntity);
            auto& playerPosition = registry.get<TComponentPosition>(_playerEntity);

            playerPosition = playerPositionBackup;
        }
    }

    if (_isPlayerMounted) {
        PlayerControlShip(renderContext, registry, controlState);
    } else {
        PlayerControlPlayer(renderContext, registry, controlState);
    }
}

auto TScene::GetRegistry() -> entt::registry& {
    return _registry;
}

auto TScene::CreateEmpty(const std::string& name) -> entt::entity {
    const auto entity = _registry.create();
    _registry.emplace<TComponentName>(entity, name);
    _registry.emplace<TComponentHierarchy>(entity);
    _registry.emplace<TComponentPosition>(entity);
    _registry.emplace<TComponentOrientationEuler>(entity);
    _registry.emplace<TComponentScale>(entity, glm::vec3(1.0f));
    _registry.emplace<TComponentTransform>(entity);
    _registry.emplace<TComponentRenderTransform>(entity);

    return entity;
}

auto TScene::CreateGlobalLight(
    const std::string& name,
    const float& azimuth,
    const float& elevation,
    const glm::vec3& color,
    float intensity,
    float canCastShadows) -> entt::entity {

    const auto entity = CreateEmpty(name);
    _registry.emplace<TComponentGlobalLight>(entity, azimuth, elevation, color, intensity, true, canCastShadows, true);
    return entity;
}

auto TScene::CreateCamera(const std::string& name) -> entt::entity {
    const auto entity = CreateEmpty(name);
    _registry.emplace<TComponentCamera>(entity);
    return entity;
}

auto TScene::CreateMesh(
        const std::string& name,
        const std::string& assetMeshName,
        const std::string& assetMaterialName) -> entt::entity {

    const auto entity = CreateEmpty(name);
    _registry.emplace<TComponentMesh>(entity, assetMeshName);
    _registry.emplace<TComponentMaterial>(entity, assetMaterialName);
    _registry.emplace<TComponentCreateGpuResourcesNecessary>(entity);
    return entity;
}

auto TScene::CreateModel(
    const std::string& name,
    const std::string_view assetModelName) -> entt::entity {

    using TVisitNodesDelegate = std::function<void(entt::entity, const Assets::TAssetModelNode&)>;
    TVisitNodesDelegate visitNodes = [&](
        const entt::entity parentEntity,
        const Assets::TAssetModelNode& assetModelNode) -> void {

        const auto& nodeName = assetModelNode.Name;
        const auto& localPosition = assetModelNode.LocalPosition;
        const auto localRotationEuler = glm::eulerAngles(assetModelNode.LocalRotation);
        const auto& localScale = assetModelNode.LocalScale;

        entt::entity childEntity = entt::null;
        if (assetModelNode.MeshName.has_value()) {
            childEntity = CreateEmpty(nodeName);
            SetParent(childEntity, parentEntity);
            SetPosition(childEntity, localPosition);
            SetOrientation(childEntity, localRotationEuler.x, localRotationEuler.y, localRotationEuler.z);
            SetScale(childEntity, localScale);

            const auto& assetMesh = Assets::GetAssetMesh(assetModelNode.MeshName.value());
            for (const auto& assetPrimitive : assetMesh.Primitives) {
                const auto meshEntity = CreateMesh(
                    assetPrimitive.Name,
                    assetPrimitive.Name,
                    assetPrimitive.MaterialName.value_or("M_Default"));
                SetParent(meshEntity, childEntity);
            }

        } else {
            childEntity = CreateEmpty(nodeName);
            SetParent(childEntity, parentEntity);
            SetPosition(childEntity, localPosition);
            SetOrientation(childEntity, localRotationEuler.x, localRotationEuler.y, localRotationEuler.z);
            SetScale(childEntity, localScale);
        }

        for (const auto& childAssetModelNode : assetModelNode.Children) {
            visitNodes(childEntity, childAssetModelNode);
        }
    };

    const auto& assetModel = Assets::GetAssetModel(assetModelName);
    const auto entity = CreateEmpty(name);

    for (const auto& assetModelNode : assetModel.Hierarchy) {
        visitNodes(entity, assetModelNode);
    }

    return entity;
}

auto TScene::SetParent(
    const entt::entity entity,
    const entt::entity parent) -> void {

    auto& [Parent, Children] = _registry.get_or_emplace<TComponentHierarchy>(entity);
    if (Parent != entt::null) {
        if (auto* oldParentHierarchy = _registry.try_get<TComponentHierarchy>(Parent)) {
            oldParentHierarchy->RemoveChild(entity);
        }
    }

    Parent = parent;

    if (parent != entt::null) {
        auto& parentHierarchy = _registry.get_or_emplace<TComponentHierarchy>(parent);
        parentHierarchy.AddChild(entity);
    }
}

auto TScene::SetPosition(
    const entt::entity entity,
    const glm::vec3& position) -> void {

    if (auto* positionComponent = _registry.try_get<TComponentPosition>(entity)) {
        *positionComponent = position;
    }
}

auto TScene::SetOrientation(
    const entt::entity entity,
    const float pitch,
    const float yaw,
    const float roll) -> void {

    if (auto* orientationComponent = _registry.try_get<TComponentOrientationEuler>(entity)) {
        *orientationComponent = { pitch, yaw, roll };
    }
}

auto TScene::SetScale(
    const entt::entity entity,
    const glm::vec3& scale) -> void {

    if (auto* scaleComponent = _registry.try_get<TComponentScale>(entity)) {
        *scaleComponent = scale;
    }
}
