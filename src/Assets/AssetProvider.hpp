#pragma once

#include "AssetModel.hpp"

struct IAssetProvider {

	virtual auto GetAssetModel(const std::string& modelName) -> std::shared_ptr<TAssetModel> = 0;
	virtual auto GetAssetMesh(const std::string& assetMeshName) -> std::shared_ptr<void> = 0;
};
