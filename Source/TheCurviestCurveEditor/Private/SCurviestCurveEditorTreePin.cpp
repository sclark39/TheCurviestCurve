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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 	
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 		
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
	FCurveEditorTreeItem* Item = CurveEditor->FindTreeItem(InTreeItem);
	if (!ensureMsgf(Item != nullptr, TEXT("Can't find curve editor tree item. Ignoring pinning request.")))
		return;

	for (FCurveModelID CurveID : Item->GetOrCreateCurves(CurveEditor))
	{
		CurveEditor->PinCurve(CurveID);
	}

	for (FCurveEditorTreeItemID Child : Item->GetChildren())
	{
		PinRecursive(Child, CurveEditor);
	}
}

void SCurviestCurveEditorTreePin::UnpinRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const bool bIsSelected = CurveEditor->GetTreeSelectionState(InTreeItem) == ECurveEditorTreeSelectionState::Explicit;

	FCurveEditorTreeItem* Item = CurveEditor->FindTreeItem(InTreeItem);
	if (!ensureMsgf(Item != nullptr, TEXT("Can't find curve editor tree item. Ignoring unpinning request.")))
		return;

	for (FCurveModelID CurveID : Item->GetCurves())
	{
		if (bIsSelected)
		{
			CurveEditor->UnpinCurve(CurveID);
		}
		else
		{
			Item->DestroyCurves(CurveEditor);
		}
	}

	for (FCurveEditorTreeItemID Child : Item->GetChildren())
	{
		UnpinRecursive(Child, CurveEditor);
	}
}

bool SCurviestCurveEditorTreePin::IsSelectedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const FCurveEditorTreeItem *Item = CurveEditor->FindTreeItem(InTreeItem);
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

bool SCurviestCurveEditorTreePin::IsChildSelectedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const FCurveEditorTreeItem *Item = CurveEditor->FindTreeItem(InTreeItem);
	if (Item)
	{
		
		const bool bIsSelected = CurveEditor->GetTreeSelectionState(InTreeItem) == ECurveEditorTreeSelectionState::Explicit;
		if (bIsSelected)
			return true;
		
		for (auto ChildId : Item->GetChildren())
			if (IsChildSelectedRecursive(ChildId, CurveEditor))
				return true;
	}
	return false;
}

bool SCurviestCurveEditorTreePin::IsPinnedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const FCurveEditorTreeItem *Item = CurveEditor->FindTreeItem(InTreeItem);

	if (Item)
	{
		TArrayView<const FCurveModelID>          Curves = Item->GetCurves();
		TArrayView<const FCurveEditorTreeItemID> Children = Item->GetChildren();

		if (Curves.Num() == 0)
		{
			const bool bAnyChildren = Algo::AllOf(Children, [this, CurveEditor](FCurveEditorTreeItemID In)
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


bool SCurviestCurveEditorTreePin::IsChildPinnedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const FCurveEditorTreeItem *Item = CurveEditor->FindTreeItem(InTreeItem);

	if (Item)
	{
		TArrayView<const FCurveModelID>          Curves = Item->GetCurves();
		TArrayView<const FCurveEditorTreeItemID> Children = Item->GetChildren();

		if (Curves.Num() == 0)
		{
			const bool bAnyChildren = Algo::AnyOf(Children, [this, CurveEditor](FCurveEditorTreeItemID In)
			{
				return this->IsChildPinnedRecursive(In, CurveEditor);
			});
			return Children.Num() > 0 && bAnyChildren;
		}
		else
		{
			const bool bAllChildren = Algo::AllOf(Children, [this, CurveEditor](FCurveEditorTreeItemID In)
			{
				return this->IsChildPinnedRecursive(In, CurveEditor);
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
}

const FSlateBrush* SCurviestCurveEditorTreePin::GetPinBrush() const
{
	TSharedPtr<FCurveEditor> CurveEditor = WeakCurveEditor.Pin();

	if (CurveEditor)
	{
		if (IsPinnedRecursive(TreeItemID, CurveEditor.Get()))
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 	
			return FAppStyle::GetBrush("Level.VisibleIcon16x");
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
			return FEditorStyle::GetBrush("Level.VisibleIcon16x");
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 	
			//return FEditorStyle::GetBrush("GenericCurveEditor.Pin_Active");
		}

		if (IsSelectedRecursive(TreeItemID, CurveEditor.Get())
			|| IsChildSelectedRecursive(TreeItemID, CurveEditor.Get())
			|| IsChildPinnedRecursive(TreeItemID, CurveEditor.Get())
		)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 		
			return FAppStyle::GetBrush("Level.VisibleIcon16x");
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
			return FEditorStyle::GetBrush("Level.VisibleIcon16x");
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 	
	return FAppStyle::GetBrush("Level.NotVisibleHighlightIcon16x");
#else // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 
	return FEditorStyle::GetBrush("Level.NotVisibleHighlightIcon16x");
#endif // #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 	
	//return FEditorStyle::GetBrush("GenericCurveEditor.Pin_Inactive");
}