// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
// Modifications by Skyler Clark to support UCurveCurviest editing

#include "CurviestCurveAssetEditor.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBorder.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "Curves/CurveBase.h"
#include "CurveAssetEditorModule.h"
#include "CurveEditor.h"
#include "SCurveEditorPanel.h"
#include "CurveEditorCommands.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SNumericDropDown.h"
#include "Curves/CurveLinearColor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/SFrameRatePicker.h"
#include "CommonFrameRates.h"
#include "Tree/ICurveEditorTreeItem.h"
#include "SCurviestCurveEditorTree.h"
#include "SCurviestCurveEditorTreePin.h"
#include "SCurviestCurveEditorTreeLock.h"

#include "CurviestCurve.h"

#define LOCTEXT_NAMESPACE "CurveAssetEditor"

const FName FCurviestCurveAssetEditor::CurveTabId( TEXT( "CurveAssetEditor_Curve" ) );
const FName FCurviestCurveAssetEditor::CurveDetailsTabId(TEXT("CurveAssetEditor_ColorCurveEditor"));

struct FCurviestCurveAssetEditorTreeParentItem : public ICurveEditorTreeItem
{
	FCurviestCurveAssetEditorTreeParentItem(FText InLabelName, FLinearColor InLabelColor)
	{
		LabelName = InLabelName;
		LabelColor = InLabelColor;
	}

	virtual TSharedPtr<SWidget> GenerateCurveEditorTreeWidget(const FName& InColumnName, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& TableRow) override
	{
		if (InColumnName == ColumnNames.Label)
		{
			return SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.Padding(FMargin(4.f))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(LabelName)
				.ColorAndOpacity(FSlateColor(LabelColor))
				];
		}

		else if (InColumnName == ColumnNames.PinHeader)
		{
			return SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCurviestCurveEditorTreePin, InCurveEditor, InTreeItemID, TableRow)
				]

			+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCurviestCurveEditorTreeLock, InCurveEditor, InTreeItemID, TableRow)
				];
		}

		return nullptr;
	}

	virtual void CreateCurveModels(TArray<TUniquePtr<FCurveModel>>& OutCurveModels) override
	{}

private:
	FText LabelName;
	FLinearColor LabelColor;
};

struct FCurviestCurveAssetEditorTreeItem : public ICurveEditorTreeItem
{
	FCurviestCurveAssetEditorTreeItem(FName InCurveName, TWeakObjectPtr<UCurveBase> InCurveOwner, const FRichCurveEditInfo& InEditInfo)
		: CurveOwner(InCurveOwner)
		, EditInfo(InEditInfo)
	{
		if (CurveOwner.IsValid())
		{
			CurveName = FText::FromName(InCurveName);
			CurveColor = CurveOwner->GetCurveColor(EditInfo);
		}
	}

	virtual TSharedPtr<SWidget> GenerateCurveEditorTreeWidget(const FName& InColumnName, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& TableRow) override
	{
		if (InColumnName == ColumnNames.Label)
		{
			return SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.Padding(FMargin(4.f))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(CurveName)
				.ColorAndOpacity(FSlateColor(CurveColor))
				];
		}
		else if (InColumnName == ColumnNames.PinHeader)
		{
			return SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCurviestCurveEditorTreePin, InCurveEditor, InTreeItemID, TableRow)
				]

			+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCurviestCurveEditorTreeLock, InCurveEditor, InTreeItemID, TableRow)
				];

		}

		return nullptr;
	}

	virtual void CreateCurveModels(TArray<TUniquePtr<FCurveModel>>& OutCurveModels) override
	{
		if (!CurveOwner.IsValid())
		{
			return;
		}

		TUniquePtr<FRichCurveEditorModelRaw> NewCurve = MakeUnique<FCurviestCurveModel>(static_cast<FRichCurve*>(EditInfo.CurveToEdit), CurveOwner.Get());
		NewCurve->SetShortDisplayName(CurveName);
		NewCurve->SetColor(CurveColor);
		OutCurveModels.Add(MoveTemp(NewCurve));
	}

private:
	TWeakObjectPtr<UCurveBase> CurveOwner;
	FRichCurveEditInfo EditInfo;
	FText CurveName;
	FLinearColor CurveColor;
};


void FCurviestCurveAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_CurveAssetEditor", "Curve Asset Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(CurveTabId, FOnSpawnTab::CreateSP(this, &FCurviestCurveAssetEditor::SpawnTab_CurveAsset))
		.SetDisplayName(LOCTEXT("CurveTab", "Curve"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.CurveBase"));
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.CurveBase"));
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 

	InTabManager->RegisterTabSpawner(CurveDetailsTabId, FOnSpawnTab::CreateSP(this, &FCurviestCurveAssetEditor::SpawnTab_CurveDetailsEditor))
		.SetDisplayName(LOCTEXT("CurveDetailsEditorTab", "Curve Details Editor"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.CurveBase"));
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.CurveBase"));
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 

}

void FCurviestCurveAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(CurveTabId);
	InTabManager->UnregisterTabSpawner(CurveDetailsTabId);
}

void FCurviestCurveAssetEditor::InitCurveAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveBase* CurveToEdit )
{

#if ENGINE_MAJOR_VERSION == 4
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_CurveAssetEditor_Layout_v1")
#else
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_CurveAssetEditor_Layout_v2")
#endif
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION == 4
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
#endif
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.9f)
				->SetHideTabWell(true)
				->AddTab(CurveTabId, ETabState::OpenedTab)
			)
		);

	// START Curviest
	UCurveCurviest* CurviestCurve = Cast<UCurveCurviest>(CurveToEdit);
	if (CurviestCurve)
	{
#if ENGINE_MAJOR_VERSION == 4
		StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_CurveAssetEditor_Layout_ColorCurvev2")
#else
		StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_CurveAssetEditor_Layout_ColorCurvev3")
#endif
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION == 4
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->SetHideTabWell(true)
					->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
				)
#endif
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.8f)
						->SetHideTabWell(true)
						->AddTab(CurveTabId, ETabState::OpenedTab)
					)

					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->SetHideTabWell(true)
						->AddTab(CurveDetailsTabId, ETabState::OpenedTab)
					)
				)
			);

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
#if ENGINE_MAJOR_VERSION == 4
		const FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);
#else
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
#endif
		CurveDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	}
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	const bool bToolbarFocusable = false;
	const bool bUseSmallIcons = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FName(TEXT("CurviestCurveAssetEditorApp")), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, CurveToEdit, bToolbarFocusable, bUseSmallIcons);

	FCurveAssetEditorModule& CurveAssetEditorModule = FModuleManager::LoadModuleChecked<FCurveAssetEditorModule>("CurveAssetEditor");
	AddMenuExtender(CurveAssetEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	AddToolbarExtender(GetToolbarExtender());

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( CurveTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	if (CurveEditor.IsValid())
	{
		RegenerateMenusAndToolbars();
	}

	if (CurviestCurve)
	{
		CurveDetailsView->SetObject(CurviestCurve);
	}
}

FName FCurviestCurveAssetEditor::GetToolkitFName() const
{
	return FName("CurviestCurveAssetEditor");
}

FText FCurviestCurveAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "Curviest Curve Asset Editor" );
}

FString FCurviestCurveAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "CurveAsset ").ToString();
}

FLinearColor FCurviestCurveAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}


TSharedRef<SDockTab> FCurviestCurveAssetEditor::SpawnTab_CurveAsset(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == CurveTabId);


	CurveEditor = MakeShared<FCurveEditor>();
	FCurveEditorInitParams InitParams;
	CurveEditor->InitCurveEditor(InitParams);
	CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}");

	// Initialize our bounds at slightly larger than default to avoid clipping the tabs on the color widget.
	TUniquePtr<ICurveEditorBounds> EditorBounds = MakeUnique<FStaticCurveEditorBounds>();
	EditorBounds->SetInputBounds(-1.05, 1.05);
	CurveEditor->SetBounds(MoveTemp(EditorBounds));
	
	CurveEditorTree = SNew(SCurviestCurveEditorTree, CurveEditor);
	CurveEditorPanel = SNew(SCurveEditorPanel, CurveEditor.ToSharedRef())
		.TreeContent()[CurveEditorTree.ToSharedRef()];

	UCurveBase* Curve = Cast<UCurveBase>(GetEditingObject());
	if (Curve)
	{
		if (UCurveCurviest *Curviest = Cast<UCurveCurviest>(Curve))
		{
			AddCurvesToCurveEditor();
			Curviest->OnCurveMapChanged.AddSP(this, &FCurviestCurveAssetEditor::RefreshTab_CurveAsset);
		}
	}

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
#if ENGINE_MAJOR_VERSION == 4
		.Icon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"))
#endif // #if UE_MAJOR_VERSION == 5
		.Label(FText::Format(LOCTEXT("CurveAssetEditorTitle", "{0} Curve Asset"), FText::FromString(GetTabPrefix())))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
		.Padding(0.0f)
		[
			CurveEditorPanel.ToSharedRef()
		]
		];
#if ENGINE_MAJOR_VERSION == 5
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
	NewDockTab->SetTabIcon(FAppStyle::GetBrush("CurveAssetEditor.Tabs.Properties"));
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	NewDockTab->SetTabIcon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"));
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
#endif // #if UE_MAJOR_VERSION == 5
	
	return NewDockTab;
}

void FCurviestCurveAssetEditor::RefreshTab_CurveAsset(UCurveBase *Curve)
{
	UCurveBase* CurveOwner = Cast<UCurveBase>(GetEditingObject());
	if (CurveOwner && Curve == CurveOwner)
	{
		// Clear out the old
		TArray<FCurveEditorTreeItemID> TreeIds;
		CurveEditor->GetTree()->GetAllItems().GetKeys(TreeIds);
		for (FCurveEditorTreeItemID TreeId : TreeIds)
			CurveEditor->RemoveTreeItem(TreeId);

		TreeItemIdMaps.Empty();
		
		AddCurvesToCurveEditor();
	}
}

FCurveEditorTreeItemID FCurviestCurveAssetEditor::GetTreeItemId(FName Breadcrumb)
{	
	FCurveEditorTreeItemID *FoundTreeId = TreeItemIdMaps.Find(Breadcrumb);
	if (FoundTreeId)
		return *FoundTreeId;

	FCurveEditorTreeItemID ParentId = FCurveEditorTreeItemID::Invalid();

	FString ParentTrail;
	FString Heading = Breadcrumb.ToString();
	if (Breadcrumb.ToString().Split(".", &ParentTrail, &Heading, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		ParentId = GetTreeItemId(FName(*ParentTrail));
	}

	TSharedPtr<FCurviestCurveAssetEditorTreeParentItem> TreeDisplayItem = MakeShared<FCurviestCurveAssetEditorTreeParentItem>(
		FText::FromString(Heading),
		FColor::White);
	FCurveEditorTreeItem* TreeItem = CurveEditor->AddTreeItem(ParentId);
	TreeItem->SetStrongItem(TreeDisplayItem);

	FCurveEditorTreeItemID TreeId = TreeItem->GetID();
	TreeItemIdMaps.Add(Breadcrumb, TreeId);

	return TreeId;
}


void FCurviestCurveAssetEditor::AddCurvesToCurveEditor()
{
	UCurveBase* Curve = Cast<UCurveBase>(GetEditingObject());
	if (!Curve) 
		return;

	// Add back the new
	for (const FRichCurveEditInfo& CurveData : Curve->GetCurves())
	{
		FCurveEditorTreeItemID ParentTreeId = FCurveEditorTreeItemID::Invalid();

		FString CurveName = CurveData.CurveName.ToString();
		FString ParentTrail;
		FString Heading = CurveName;
		if (CurveName.Split(".", &ParentTrail, &Heading, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			ParentTreeId = GetTreeItemId(FName(*ParentTrail));
		}

		TSharedPtr<FCurviestCurveAssetEditorTreeItem> TreeItem = MakeShared<FCurviestCurveAssetEditorTreeItem>(FName(*Heading), Curve, CurveData);

		// Add the channel to the tree-item and let it manage the lifecycle of the tree item.
		FCurveEditorTreeItem* NewItem = CurveEditor->AddTreeItem(ParentTreeId);
		NewItem->SetStrongItem(TreeItem);
		
		// Expand all folders so everything is visible when you open the editor.
		CurveEditorTree->SetItemExpansion(ParentTreeId, true);

		// Pin all of the curves so everything is visible when you open the editor.
		for (const FCurveModelID CurveModel : NewItem->GetOrCreateCurves(CurveEditor.Get()))
		{
			CurveEditor->PinCurve(CurveModel);			
		}
	}
}


TSharedRef<SDockTab> FCurviestCurveAssetEditor::SpawnTab_CurveDetailsEditor(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == CurveDetailsTabId);

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
#if ENGINE_MAJOR_VERSION == 4
		.Icon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"))
#endif // #if UE_MAJOR_VERSION == 5
		.Label(LOCTEXT("CurveDetailsEditor", "Curve Details Editor"))
		.TabColorScale(GetTabColorScale())
		[
			CurveDetailsView.ToSharedRef()
		];
#if ENGINE_MAJOR_VERSION == 5
	NewDockTab->SetTabIcon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"));
#endif // #if UE_MAJOR_VERSION == 5

	return NewDockTab;
}


TSharedPtr<FExtender> FCurviestCurveAssetEditor::GetToolbarExtender()
{
	// Use the Curve Editor Panel's extenders which already has all of the icons listed in the right order.
	return CurveEditorPanel->GetToolbarExtender();
}

EOrientation FCurviestCurveAssetEditor::GetSnapLabelOrientation() const
{
	return FMultiBoxSettings::UseSmallToolBarIcons.Get()
		? EOrientation::Orient_Horizontal
		: EOrientation::Orient_Vertical;
}

TSharedRef<SWidget> FCurviestCurveAssetEditor::MakeCurveEditorCurveOptionsMenu()
{
	struct FExtrapolationMenus
	{
		static void MakePreInfinityExtrapSubMenu(FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection("Pre-Infinity Extrapolation", LOCTEXT("CurveEditorMenuPreInfinityExtrapHeader", "Extrapolation"));
			{
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPreInfinityExtrapCycle);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPreInfinityExtrapCycleWithOffset);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPreInfinityExtrapOscillate);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPreInfinityExtrapLinear);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPreInfinityExtrapConstant);
			}
			MenuBuilder.EndSection();
		}

		static void MakePostInfinityExtrapSubMenu(FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection("Post-Infinity Extrapolation", LOCTEXT("CurveEditorMenuPostInfinityExtrapHeader", "Extrapolation"));
			{
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPostInfinityExtrapCycle);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPostInfinityExtrapCycleWithOffset);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPostInfinityExtrapOscillate);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPostInfinityExtrapLinear);
				MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetPostInfinityExtrapConstant);
			}
			MenuBuilder.EndSection();
		}
	};

	FMenuBuilder MenuBuilder(true, CurveEditor->GetCommands());

	MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().BakeCurve);
	MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().ReduceCurve);

	MenuBuilder.AddSubMenu(
		LOCTEXT("PreInfinitySubMenu", "Pre-Infinity"),
		LOCTEXT("PreInfinitySubMenuToolTip", "Pre-Infinity Extrapolation"),
		FNewMenuDelegate::CreateStatic(&FExtrapolationMenus::MakePreInfinityExtrapSubMenu));

	MenuBuilder.AddSubMenu(
		LOCTEXT("PostInfinitySubMenu", "Post-Infinity"),
		LOCTEXT("PostInfinitySubMenuToolTip", "Post-Infinity Extrapolation"),
		FNewMenuDelegate::CreateStatic(&FExtrapolationMenus::MakePostInfinityExtrapSubMenu));

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
