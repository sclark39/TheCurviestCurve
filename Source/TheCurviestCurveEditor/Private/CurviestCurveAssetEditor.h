// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
// Modifications by Skyler Clark to support UCurveCurviest editing
// FCurveAssetEditor is in private :(

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Toolkits/IToolkitHost.h"
#include "ICurveAssetEditor.h"
#include "CurveEditorTypes.h"
#include "Containers/Map.h"

class FCurveEditor;
class UCurveBase;
class SCurveEditorPanel;

class FCurviestCurveAssetEditor :  public ICurveAssetEditor
{
public:

	FCurviestCurveAssetEditor()
	{}

	virtual ~FCurviestCurveAssetEditor() {}

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/**
	 * Edits the specified table
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	CurveToEdit				The curve to edit
	 */
	void InitCurveAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveBase* CurveToEdit );

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	TSharedPtr<FExtender> GetToolbarExtender();
	TSharedRef<SWidget> MakeCurveEditorCurveOptionsMenu();

private:
	
	/**	Spawns the tab with the curve asset inside */
	TSharedRef<SDockTab> SpawnTab_CurveAsset( const FSpawnTabArgs& Args );
	void RefreshTab_CurveAsset(UCurveBase *Curve);

	void AddCurvesToCurveEditor();

	/**	Spawns the details panel for the color curve */
	TSharedRef<SDockTab> SpawnTab_CurveDetailsEditor(const FSpawnTabArgs& Args);

	/** Get the orientation for the snap value controls. */
	EOrientation GetSnapLabelOrientation() const;

private:
	TSharedPtr<FCurveEditor> CurveEditor;
	TSharedPtr<SCurveEditorPanel> CurveEditorPanel;

	/**	The tab id for the curve asset tab */
	static const FName CurveTabId;
	/**	The tab id for the color curve editor tab */
	static const FName CurveDetailsTabId;

	/* Holds the details panel for the color curve */
	TSharedPtr<class IDetailsView> CurveDetailsView;

	FCurveEditorTreeItemID GetTreeItemId(FName Breadcrumb);
	TMap<FName, FCurveEditorTreeItemID> TreeItemIdMaps;
};
