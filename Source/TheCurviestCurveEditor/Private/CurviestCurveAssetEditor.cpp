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
#include "RichCurveEditorModel.h"
#include "CurveEditorCommands.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SNumericDropDown.h"
#include "Curves/CurveLinearColor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/SFrameRatePicker.h"
#include "CommonFrameRates.h"
#include "Tree/ICurveEditorTreeItem.h"
#include "Tree/SCurveEditorTree.h"
#include "Tree/SCurveEditorTreePin.h"

#include "CurviestCurve.h"

#define LOCTEXT_NAMESPACE "CurveAssetEditor"

const FName FCurviestCurveAssetEditor::CurveTabId( TEXT( "CurveAssetEditor_Curve" ) );
const FName FCurviestCurveAssetEditor::CurveDetailsTabId(TEXT("CurveAssetEditor_ColorCurveEditor"));

struct FCurviestCurveAssetEditorTreeItem : public ICurveEditorTreeItem
{
	FCurviestCurveAssetEditorTreeItem(TWeakObjectPtr<UCurveBase> InCurveOwner, const FRichCurveEditInfo& InEditInfo)
		: CurveOwner(InCurveOwner)
		, EditInfo(InEditInfo)
	{
		if (CurveOwner.IsValid())
		{
			CurveName = FText::FromName(EditInfo.CurveName);
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
			return SNew(SCurveEditorTreePin, InCurveEditor, InTreeItemID, TableRow);
		}

		return nullptr;
	}

	virtual void CreateCurveModels(TArray<TUniquePtr<FCurveModel>>& OutCurveModels) override
	{
		if (!CurveOwner.IsValid())
		{
			return;
		}

		TUniquePtr<FRichCurveEditorModel> NewCurve = MakeUnique<FRichCurveEditorModel>(static_cast<FRichCurve*>(EditInfo.CurveToEdit), CurveOwner.Get());
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
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.CurveBase"));

	InTabManager->RegisterTabSpawner(CurveDetailsTabId, FOnSpawnTab::CreateSP(this, &FCurviestCurveAssetEditor::SpawnTab_CurveDetailsEditor))
		.SetDisplayName(LOCTEXT("CurveDetailsEditorTab", "Curve Details Editor"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.CurveBase"));

}

void FCurviestCurveAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(CurveTabId);
	InTabManager->UnregisterTabSpawner(CurveDetailsTabId);
}

void FCurviestCurveAssetEditor::InitCurveAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveBase* CurveToEdit )
{

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_CurveAssetEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
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
		StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_CurveAssetEditor_Layout_ColorCurvev2")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->SetHideTabWell(true)
					->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
				)
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
		const FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);
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

	CurveEditorPanel = SNew(SCurveEditorPanel, CurveEditor.ToSharedRef())
		.TreeContent()
		[
			SNew(SCurveEditorTree, CurveEditor)
		];

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
		.Icon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"))
		.Label(FText::Format(LOCTEXT("CurveAssetEditorTitle", "{0} Curve Asset"), FText::FromString(GetTabPrefix())))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0.0f)
		[
			CurveEditorPanel.ToSharedRef()
		]
		];
	
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

		TArray<FCurveModelID> CurveIds;
		CurveEditor->GetCurves().GetKeys(CurveIds);

		for (FCurveEditorTreeItemID TreeId : TreeIds)
			CurveEditor->RemoveTreeItem(TreeId);

		for (FCurveModelID CurveId : CurveIds)
			CurveEditor->RemoveCurve(CurveId);

		AddCurvesToCurveEditor();
		RegenerateMenusAndToolbars();
	}
}

void FCurviestCurveAssetEditor::AddCurvesToCurveEditor()
{
	UCurveBase* Curve = Cast<UCurveBase>(GetEditingObject());
	if (!Curve) 
		return;

	// Add back the new
	for (const FRichCurveEditInfo& CurveData : Curve->GetCurves())
	{
		TSharedPtr<FCurviestCurveAssetEditorTreeItem> TreeItem = MakeShared<FCurviestCurveAssetEditorTreeItem>(Curve, CurveData);

		// Add the channel to the tree-item and let it manage the lifecycle of the tree item.
		FCurveEditorTreeItem* NewItem = CurveEditor->AddTreeItem(FCurveEditorTreeItemID::Invalid());
		NewItem->SetStrongItem(TreeItem);

		// Pin all of the created curves by default for now so that they're visible when you open the
		// editor. Since there's only ever up to 4 channels we don't have to worry about overwhelming
		// amounts of curves.
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
		.Icon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"))
		.Label(LOCTEXT("CurveDetailsEditor", "Curve Details Editor"))
		.TabColorScale(GetTabColorScale())
		[
			CurveDetailsView.ToSharedRef()
		];

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
