// Copyright 2019 Skyler Clark. All Rights Reserved.

#include "AssetTypeActions_CurviestCurve.h"

#include "EditorFramework/AssetImportData.h"

#include "CurviestCurveAssetEditor.h"

void FAssetTypeActions_CurviestCurve::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Curve = Cast<UCurveBase>(*ObjIt);
		if (Curve != nullptr)
		{
			TSharedRef< FCurviestCurveAssetEditor > NewCurveAssetEditor(new FCurviestCurveAssetEditor());
			NewCurveAssetEditor->InitCurveAssetEditor(Mode, EditWithinLevelEditor, Curve);
		}
	}
}

void FAssetTypeActions_CurviestCurve::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto Curve = CastChecked<UCurveBase>(Asset);
		if (Curve->AssetImportData)
		{
			Curve->AssetImportData->ExtractFilenames(OutSourceFilePaths);
		}
	}
}