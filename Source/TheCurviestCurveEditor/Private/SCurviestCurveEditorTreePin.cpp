// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCurviestCurveEditorTreePin.h"
#include "CurveEditor.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableRow.h"

#include "EditorStyleSet.h"


void SCurviestCurveEditorTreePin::Construct(const FArguments& InArgs, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& InTableRow)
{
	WeakCurveEditor = InCurveEditor;
	WeakTableRow = InTableRow;
	TreeItemID = InTreeItemID;

	ChildSlot
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.Visibility(this, &SCurviestCurveEditorTreePin::GetPinVisibility)
		.OnClicked(this, &SCurviestCurveEditorTreePin::TogglePinned)
		[
			SNew(SImage)
			.Image(this, &SCurviestCurveEditorTreePin::GetPinBrush)
		]
		];
}

FReply SCurviestCurveEditorTreePin::TogglePinned()
{
	TSharedPtr<FCurveEditor> CurveEditor = WeakCurveEditor.Pin();
	if (CurveEditor)
	{
		if (IsPinnedRecursive(TreeItemID, CurveEditor.Get()))
		{
			UnpinRecursive(TreeItemID, CurveEditor.Get());
		}
		else
		{
			PinRecursive(TreeItemID, CurveEditor.Get());
		}
	}
	return FReply::Handled();
}

void SCurviestCurveEditorTreePin::PinRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	FCurveEditorTreeItem& Item = CurveEditor->GetTreeItem(InTreeItem);

	for (FCurveModelID CurveID : Item.GetOrCreateCurves(CurveEditor))
	{
		CurveEditor->PinCurve(CurveID);
	}

	for (FCurveEditorTreeItemID Child : Item.GetChildren())
	{
		PinRecursive(Child, CurveEditor);
	}
}

void SCurviestCurveEditorTreePin::UnpinRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const bool bIsSelected = CurveEditor->GetTreeSelectionState(InTreeItem) == ECurveEditorTreeSelectionState::Explicit;

	FCurveEditorTreeItem& Item = CurveEditor->GetTreeItem(InTreeItem);
	for (FCurveModelID CurveID : Item.GetCurves())
	{
		if (bIsSelected)
		{
			CurveEditor->UnpinCurve(CurveID);
		}
		else
		{
			Item.DestroyCurves(CurveEditor);
		}
	}

	for (FCurveEditorTreeItemID Child : Item.GetChildren())
	{
		UnpinRecursive(Child, CurveEditor);
	}
}

bool SCurviestCurveEditorTreePin::IsSelectedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const FCurveEditorTreeItem *Item = CurveEditor->GetTree()->FindItem(InTreeItem);
	if (Item)
	{
		const bool bIsSelected = CurveEditor->GetTreeSelectionState(InTreeItem) == ECurveEditorTreeSelectionState::Explicit;
		if (bIsSelected)
			return true;

		FCurveEditorTreeItemID ParentId = Item->GetParentID();
		return IsSelectedRecursive(ParentId, CurveEditor);
	}
	return false;
}

bool SCurviestCurveEditorTreePin::IsPinnedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const FCurveEditorTreeItem *Item = CurveEditor->GetTree()->FindItem(InTreeItem);	

	if (Item)
	{
		TArrayView<const FCurveModelID>          Curves = Item->GetCurves();
		TArrayView<const FCurveEditorTreeItemID> Children = Item->GetChildren();

		if (Curves.Num() == 0)
		{
			const bool bAnyChildren = Algo::AnyOf(Children, [this, CurveEditor](FCurveEditorTreeItemID In)
			{
				return this->IsPinnedRecursive(In, CurveEditor);
			});
			return Children.Num() > 0 && bAnyChildren;
		}
		else
		{
			const bool bAllChildren = Algo::AllOf(Children, [this, CurveEditor](FCurveEditorTreeItemID In)
			{
				return this->IsPinnedRecursive(In, CurveEditor);
			});
			const bool bAllCurves = Algo::AllOf(Curves, [CurveEditor](FCurveModelID In)
			{
				return CurveEditor->IsCurvePinned(In);
			});
			return bAllChildren && bAllCurves;
		}
	}

	return false;
}

EVisibility SCurviestCurveEditorTreePin::GetPinVisibility() const
{
	TSharedPtr<FCurveEditor> CurveEditor = WeakCurveEditor.Pin();
	TSharedPtr<ITableRow> Row = WeakTableRow.Pin();
	TSharedPtr<SWidget> RowWidget = Row ? TSharedPtr<SWidget>(Row->AsWidget()) : nullptr;

	return EVisibility::Visible;
	/*
	if (RowWidget && RowWidget->IsHovered())
	{
		return EVisibility::Visible;
	}
	else if (CurveEditor && IsPinnedRecursive(TreeItemID, CurveEditor.Get()))
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;*/
}

const FSlateBrush* SCurviestCurveEditorTreePin::GetPinBrush() const
{
	TSharedPtr<FCurveEditor> CurveEditor = WeakCurveEditor.Pin();

	if (CurveEditor)
	{
		if (IsPinnedRecursive(TreeItemID, CurveEditor.Get()))
		{
			return FEditorStyle::GetBrush("GenericCurveEditor.Pin_Active");
		}

		if (IsSelectedRecursive(TreeItemID, CurveEditor.Get()))
			return FEditorStyle::GetBrush("Level.VisibleIcon16x");
	}

	return FEditorStyle::GetBrush("Level.NotVisibleHighlightIcon16x");
	//return FEditorStyle::GetBrush("GenericCurveEditor.Pin_Inactive");
}