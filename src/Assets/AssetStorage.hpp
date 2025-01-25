#pragma once

#include "../Core/Types.hpp"
#include "AssetImporter.hpp"
#include "AssetProvider.hpp"

class TAssetStorage final : public IAssetProvider, public IAssetImporter {
public:
	TAssetStorage();
	~TAssetStorage();

	auto GetAssetModel(const std::string& assetName) -> TAssetModel& override;
	auto GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& override;

	auto ImportModel(
		const std::string& modelName,
		const std::filesystem::path& filePath) const -> std::expected<TAssetModel, std::string> override;

private:
	class Implementation;
	TScoped<Implementation> _implementation;
};

