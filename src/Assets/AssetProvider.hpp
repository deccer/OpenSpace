#pragma once

#include "AssetModel.hpp"
#include "AssetMesh.hpp"

struct IAssetProvider {

	virtual auto GetAssetModel(const std::string& modelName) -> TAssetModel& = 0;
	virtual auto GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& = 0;
};
