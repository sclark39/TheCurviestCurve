// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
// Modifications by Skyler Clark to support UCurveCurviest editing

#include "CurviestCurveAssetEditor.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBorder.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "Curves/CurveBase.h"
#include "CurveAssetEditor/Public/CurveAssetEditorModule.h"

#include "SCurveEditor.h"
#include "CurveEditor/Public/CurveEditorCommands.h"
//#include "Toolkits/IToolkitHost.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SNumericDropDown.h"
#include "CurviestCurve.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "CurveAssetEditor"

const FName FCurviestCurveAssetEditor::CurveTabId( TEXT( "CurveAssetEditor_Curve" ) );
const FName FCurviestCurveAssetEditor::ColorCurveEditorTabId(TEXT("CurveAssetEditor_ColorCurveEditor"));

void FCurviestCurveAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_CurveAssetEditor", "Curve Asset Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner( CurveTabId, FOnSpawnTab::CreateSP(this, &FCurviestCurveAssetEditor::SpawnTab_CurveAsset) )
		.SetDisplayName( LOCTEXT("CurveTab", "Curve") )
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.CurveBase"));

	if (ColorCurveDetailsView)
	{
		InTabManager->RegisterTabSpawner(ColorCurveEditorTabId, FOnSpawnTab::CreateSP(this, &FCurviestCurveAssetEditor::SpawnTab_ColorCurveEditor))
			.SetDisplayName(LOCTEXT("ColorCurveEditorTab", "Color Curve Editor"))
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.CurveBase"));
	}
}

void FCurviestCurveAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner( CurveTabId );
	InTabManager->UnregisterTabSpawner(ColorCurveEditorTabId);
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
	UCurveCurviest* ColorCurve = Cast<UCurveCurviest>(CurveToEdit);
	// END Curviest
	if (ColorCurve)
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
						->AddTab(ColorCurveEditorTabId, ETabState::OpenedTab)
					)
				)
			);

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		const FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);
		ColorCurveDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	}
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FName(TEXT("CurveAssetEditorApp")), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, CurveToEdit );
	
	FCurveAssetEditorModule& CurveAssetEditorModule = FModuleManager::LoadModuleChecked<FCurveAssetEditorModule>( "CurveAssetEditor" );
	AddMenuExtender(CurveAssetEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	AddToolbarExtender(GetToolbarExtender());

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( CurveTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	if (TrackWidget.IsValid())
	{
		RegenerateMenusAndToolbars();
	}

	if (ColorCurve)
	{
		ColorCurveDetailsView->SetObject(ColorCurve);
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

TSharedRef<SDockTab> FCurviestCurveAssetEditor::SpawnTab_CurveAsset( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == CurveTabId );

	ViewMinInput=0.f;
	ViewMaxInput=5.f;

	InputSnap = 0.1f;
	OutputSnap = 0.05f;

	UCurveBase* Curve = Cast<UCurveBase>(GetEditingObject());

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties") )
		.Label( FText::Format(LOCTEXT("CurveAssetEditorTitle", "{0} Curve Asset"), FText::FromString(GetTabPrefix())))
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				SAssignNew(TrackWidget, SCurveEditor)
				.ViewMinInput(this, &FCurviestCurveAssetEditor::GetViewMinInput)
				.ViewMaxInput(this, &FCurviestCurveAssetEditor::GetViewMaxInput)
				.InputSnap(this, &FCurviestCurveAssetEditor::GetInputSnap)
				.OutputSnap(this, &FCurviestCurveAssetEditor::GetOutputSnap)
				.TimelineLength(this, &FCurviestCurveAssetEditor::GetTimelineLength)
				.OnSetInputViewRange(this, &FCurviestCurveAssetEditor::SetInputViewRange)
				.HideUI(false)
				.AlwaysDisplayColorCurves(true)
				.ShowZoomButtons(false)
			]
		];



	FCurveOwnerInterface* CurveOwner = Curve;
	
	if (CurveOwner != NULL)
	{
		check(TrackWidget.IsValid());
		// Set this curve as the SCurveEditor's selected curve
		TrackWidget->SetCurveOwner(CurveOwner);

		// START Curviest
		if (UCurveCurviest *Curviest = Cast<UCurveCurviest>(Curve))
		{
			Curviest->OnCurveMapChanged.AddSP(this, &FCurviestCurveAssetEditor::RefreshTab_CurveAsset);
		}
		// END Curviest

	}

	return NewDockTab;
}

// START Curviest
void FCurviestCurveAssetEditor::RefreshTab_CurveAsset(UCurveBase *Curve)
{
	UCurveBase* CurveOwner = Cast<UCurveBase>(GetEditingObject());
	if (CurveOwner && Curve == CurveOwner)
	{
		check(TrackWidget.IsValid());
		// Set this curve as the SCurveEditor's selected curve
		TrackWidget->SetCurveOwner(Curve);
	}
}
// END Curviest

TSharedRef<SDockTab> FCurviestCurveAssetEditor::SpawnTab_ColorCurveEditor(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == ColorCurveEditorTabId);


	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties"))
		.Label(LOCTEXT("ColorCurveEditor", "Color Curve Editor"))
		.TabColorScale(GetTabColorScale())
		[
			ColorCurveDetailsView.ToSharedRef()
		];

	return NewDockTab;
}

float FCurviestCurveAssetEditor::GetInputSnap() const
{
	return InputSnap;
}

void FCurviestCurveAssetEditor::SetInputSnap(float value)
{
	InputSnap = value;
}

float FCurviestCurveAssetEditor::GetOutputSnap() const
{
	return OutputSnap;
}

void FCurviestCurveAssetEditor::SetOutputSnap(float value)
{
	OutputSnap = value;
}

float FCurviestCurveAssetEditor::GetTimelineLength() const
{
	return 0.f;
}

void FCurviestCurveAssetEditor::SetInputViewRange(float InViewMinInput, float InViewMaxInput)
{
	ViewMaxInput = InViewMaxInput;
	ViewMinInput = InViewMinInput;
}

TSharedPtr<FExtender> FCurviestCurveAssetEditor::GetToolbarExtender()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TSharedRef<SWidget> InputSnapWidget, TSharedRef<SWidget> OutputSnapWidget, FCurviestCurveAssetEditor* CurveAssetEditor)
		{
			ToolbarBuilder.BeginSection("Curve");
			{
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().ZoomToFitHorizontal);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().ZoomToFitVertical);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().ZoomToFit);
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Interpolation");
			{
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().InterpolationCubicAuto);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().InterpolationCubicUser);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().InterpolationCubicBreak);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().InterpolationLinear);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().InterpolationConstant);
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.AddComboButton(
				FUIAction(),
				FOnGetContent::CreateSP( CurveAssetEditor, &FCurviestCurveAssetEditor::MakeCurveEditorCurveOptionsMenu ),
				LOCTEXT( "CurveEditorCurveOptions", "Curves Options" ),
				LOCTEXT( "CurveEditorCurveOptionsToolTip", "Curve Options" ),
				TAttribute<FSlateIcon>(),
				true );

			ToolbarBuilder.BeginSection("Snap");
			{
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().ToggleInputSnapping);
				ToolbarBuilder.AddWidget(InputSnapWidget);
				ToolbarBuilder.AddToolBarButton(FCurveEditorCommands::Get().ToggleOutputSnapping);
				ToolbarBuilder.AddWidget(OutputSnapWidget);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	TArray<SNumericDropDown<float>::FNamedValue> SnapValues;
	SnapValues.Add( SNumericDropDown<float>::FNamedValue( 0.001f, LOCTEXT( "Snap_OneThousandth", "0.001" ), LOCTEXT( "SnapDescription_OneThousandth", "Set snap to 1/1000th" ) ) );
	SnapValues.Add( SNumericDropDown<float>::FNamedValue( 0.01f, LOCTEXT( "Snap_OneHundredth", "0.01" ), LOCTEXT( "SnapDescription_OneHundredth", "Set snap to 1/100th" ) ) );
	SnapValues.Add( SNumericDropDown<float>::FNamedValue( 0.1f, LOCTEXT( "Snap_OneTenth", "0.1" ), LOCTEXT( "SnapDescription_OneTenth", "Set snap to 1/10th" ) ) );
	SnapValues.Add( SNumericDropDown<float>::FNamedValue( 1.0f, LOCTEXT( "Snap_One", "1" ), LOCTEXT( "SnapDescription_One", "Set snap to 1" ) ) );
	SnapValues.Add( SNumericDropDown<float>::FNamedValue( 10.0f, LOCTEXT( "Snap_Ten", "10" ), LOCTEXT( "SnapDescription_Ten", "Set snap to 10" ) ) );
	SnapValues.Add( SNumericDropDown<float>::FNamedValue( 100.0f, LOCTEXT( "Snap_OneHundred", "100" ), LOCTEXT( "SnapDescription_OneHundred", "Set snap to 100" ) ) );

	TSharedRef<SWidget> InputSnapWidget =
		SNew( SNumericDropDown<float> )
		.DropDownValues( SnapValues )
		.LabelText( LOCTEXT("InputSnapLabel", "Input Snap"))
		.Value( this, &FCurviestCurveAssetEditor::GetInputSnap )
		.OnValueChanged( this, &FCurviestCurveAssetEditor::SetInputSnap )
		.Orientation( this, &FCurviestCurveAssetEditor::GetSnapLabelOrientation );

	TSharedRef<SWidget> OutputSnapWidget =
		SNew( SNumericDropDown<float> )
		.DropDownValues( SnapValues )
		.LabelText( LOCTEXT( "OutputSnapLabel", "Output Snap" ) )
		.Value( this, &FCurviestCurveAssetEditor::GetOutputSnap )
		.OnValueChanged( this, &FCurviestCurveAssetEditor::SetOutputSnap )
		.Orientation( this, &FCurviestCurveAssetEditor::GetSnapLabelOrientation );

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		TrackWidget->GetCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, InputSnapWidget, OutputSnapWidget, this)
		);

	return ToolbarExtender;
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
			MenuBuilder.BeginSection( "Pre-Infinity Extrapolation", LOCTEXT( "CurveEditorMenuPreInfinityExtrapHeader", "Extrapolation" ) );
			{
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPreInfinityExtrapCycle);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPreInfinityExtrapCycleWithOffset);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPreInfinityExtrapOscillate);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPreInfinityExtrapLinear);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPreInfinityExtrapConstant);
			}
			MenuBuilder.EndSection();
		}

		static void MakePostInfinityExtrapSubMenu(FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection( "Post-Infinity Extrapolation", LOCTEXT( "CurveEditorMenuPostInfinityExtrapHeader", "Extrapolation" ) );
			{
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPostInfinityExtrapCycle);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPostInfinityExtrapCycleWithOffset);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPostInfinityExtrapOscillate);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPostInfinityExtrapLinear);
				MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().SetPostInfinityExtrapConstant);
			}
			MenuBuilder.EndSection();
		}
	};

	FMenuBuilder MenuBuilder( true, TrackWidget->GetCommands());

	MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().BakeCurve);
	MenuBuilder.AddMenuEntry( FCurveEditorCommands::Get().ReduceCurve);

	MenuBuilder.AddSubMenu(
		LOCTEXT( "PreInfinitySubMenu", "Pre-Infinity" ),
		LOCTEXT( "PreInfinitySubMenuToolTip", "Pre-Infinity Extrapolation" ),
		FNewMenuDelegate::CreateStatic( &FExtrapolationMenus::MakePreInfinityExtrapSubMenu ) );

	MenuBuilder.AddSubMenu(
		LOCTEXT( "PostInfinitySubMenu", "Post-Infinity" ),
		LOCTEXT( "PostInfinitySubMenuToolTip", "Post-Infinity Extrapolation" ),
		FNewMenuDelegate::CreateStatic( &FExtrapolationMenus::MakePostInfinityExtrapSubMenu ) );
	
	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
