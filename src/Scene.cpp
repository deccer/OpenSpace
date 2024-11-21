#include "Scene.hpp"
#include "Components.hpp"
#include "Profiler.hpp"
#include "Core.hpp"
#include "Assets.hpp"

#include "glm/ext/matrix_transform.hpp"

#include <format>
#include <optional>

entt::registry g_registry = {};

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

auto SceneLoad() -> bool {

    /*
     * Load Assets
     */

    //AddAssetFromFile("Test", "data/basic/fform_1.glb");
    Assets::AddAssetFromFile("fform1", "data/basic/fform_1.glb");
    Assets::AddAssetFromFile("fform2", "data/basic/fform_2.glb");
    Assets::AddAssetFromFile("fform3", "data/basic/fform_3.glb");
    Assets::AddAssetFromFile("fform4", "data/basic/fform_4.glb");
    Assets::AddAssetFromFile("fform5", "data/basic/fform_5.glb");
    Assets::AddAssetFromFile("fform6", "data/basic/fform_6.glb");
    Assets::AddAssetFromFile("fform7", "data/basic/fform_7.glb");
    Assets::AddAssetFromFile("fform8", "data/basic/fform_9.glb");
    Assets::AddAssetFromFile("fform9", "data/basic/fform_10.glb");

    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/Sponza/glTF/Sponza.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/_Random/SM_Cube_OneMaterialPerFace.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Downloads/modular_ruins_c/modular_ruins_c.glb");
    Assets::AddAssetFromFile("Test", "data/default/SM_Deccer_Cubes_Textured.glb");
    //LoadModelFromFile("SM_Tower", "data/scenes/Tower/scene.gltf");

    //LoadModelFromFile("SM_DeccerCube", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube1", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube_1by1by1", "data/basic/SM_Cube_1by1by1.glb");

    //LoadModelFromFile("SM_Cube_1by1_x5", "data/basic/SM_Cube_1by1_x5.glb");
    //LoadModelFromFile("SM_Cube_1by1_y5", "data/basic/SM_Cube_1by1_y5.glb");
    //LoadModelFromFile("SM_Cube_1by1_z5", "data/basic/SM_Cube_1by1_z5.glb");
    //LoadModelFromFile("Test", "data/basic/Cubes/SM_Cube_1by1_5y.gltf");
    //LoadModelFromFile("Test2", "data/basic/Cubes2/SM_Cube_1by1_5y.gltf");

    //LoadModelFromFile("SM_IntelSponza", "data/scenes/IntelSponza/NewSponza_Main_glTF_002.gltf");

    /// Setup Scene ////////////

    //SceneAddEntity(std::nullopt, "Test", glm::mat4(1.0f));

    SceneAddEntity(std::nullopt, "Test", glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -20.0f)), false, "");
    for (auto i = 1; i < 10; i++) {
        SceneAddEntity(std::nullopt, std::format("fform{}", i), glm::translate(glm::mat4(1.0f), glm::vec3(i * 5.0f, 0.0f, 0.0f)), true, "M_Default");
    }

    //AddEntity(std::nullopt, "Test2", glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -10.0f}));
    //AddEntity(std::nullopt, "SM_Tower", glm::scale(glm::mat4(1.0f), {0.01f, 0.01f, 0.01f}) * glm::translate(glm::mat4(1.0f), {-5.0f, -2.0f, -10.0f}));
    //AddEntity(std::nullopt, "SM_Cube1", glm::translate(glm::mat4(1.0f), {0.0f, 10.0f, 10.0f}));
    //AddEntity(std::nullopt, "SM_IntelSponza", glm::translate(glm::mat4(1.0f), {0.0f, -10.0f, 0.0f}));



    return true;
}

auto SceneUnload() -> void {

}

auto SceneGetRegistry() -> entt::registry& {
    return g_registry;
}
