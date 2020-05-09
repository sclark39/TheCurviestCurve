// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCurviestCurveEditorTreeLock.h"
#include "CurviestCurveAssetEditor.h"
#include "CurveEditor.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableRow.h"

#include "EditorStyleSet.h"


void SCurviestCurveEditorTreeLock::Construct(const FArguments& InArgs, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& InTableRow)
{
	WeakCurveEditor = InCurveEditor;
	WeakTableRow = InTableRow;
	TreeItemID = InTreeItemID;

	ChildSlot
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.Visibility(this, &SCurviestCurveEditorTreeLock::GetPinVisibility)
		.OnClicked(this, &SCurviestCurveEditorTreeLock::ToggleLocked)
		[
			SNew(SImage)
			.Image(this, &SCurviestCurveEditorTreeLock::GetPinBrush)
		]
		];
}

FReply SCurviestCurveEditorTreeLock::ToggleLocked()
{
	TSharedPtr<FCurveEditor> CurveEditor = WeakCurveEditor.Pin();
	if (CurveEditor)
	{
		if (IsLockedRecursive(TreeItemID, CurveEditor.Get()))
		{
			UnlockRecursive(TreeItemID, CurveEditor.Get());
		}
		else
		{
			LockRecursive(TreeItemID, CurveEditor.Get());
		}
	}
	return FReply::Handled();
}

void SCurviestCurveEditorTreeLock::LockRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	FCurveEditorTreeItem* Item = CurveEditor->FindTreeItem(InTreeItem);
	if (!ensureMsgf(Item != nullptr, TEXT("Can't find curve editor tree item. Ignoring lock request.")))
		return;

	for (FCurveModelID CurveID : Item->GetOrCreateCurves(CurveEditor))
	{
		FCurveModel *CurveModel = CurveEditor->FindCurve(CurveID);
		if (FCurviestCurveModel *CurviestCurveModel = static_cast<FCurviestCurveModel*>(CurveModel))
			CurviestCurveModel->SetLocked(true);
	}

	for (FCurveEditorTreeItemID Child : Item->GetChildren())
	{
		LockRecursive(Child, CurveEditor);
	}
}

void SCurviestCurveEditorTreeLock::UnlockRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
{
	const bool bIsSelected = CurveEditor->GetTreeSelectionState(InTreeItem) == ECurveEditorTreeSelectionState::Explicit;

	FCurveEditorTreeItem* Item = CurveEditor->FindTreeItem(InTreeItem);
	if (!ensureMsgf(Item != nullptr, TEXT("Can't find curve editor tree item. Ignoring unlock request.")))
		return;

	for (FCurveModelID CurveID : Item->GetCurves())
	{
		FCurveModel *CurveModel = CurveEditor->FindCurve(CurveID);
		if (FCurviestCurveModel *CurviestCurveModel = static_cast<FCurviestCurveModel*>(CurveModel))
			CurviestCurveModel->SetLocked(false);
	}

	for (FCurveEditorTreeItemID Child : Item->GetChildren())
	{
		UnlockRecursive(Child, CurveEditor);
	}
}

bool SCurviestCurveEditorTreeLock::IsLockedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const
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
				return this->IsLockedRecursive(In, CurveEditor);
			});
			return Children.Num() > 0 && bAnyChildren;
		}
		else
		{
			const bool bAllChildren = Algo::AllOf(Children, [this, CurveEditor](FCurveEditorTreeItemID In)
			{
				return this->IsLockedRecursive(In, CurveEditor);
			});
			const bool bAllCurves = Algo::AllOf(Curves, [CurveEditor](FCurveModelID In)
			{
				return CurveEditor->FindCurve(In)->IsReadOnly();
			});
			return bAllChildren && bAllCurves;
		}
	}

	return false;
}

EVisibility SCurviestCurveEditorTreeLock::GetPinVisibility() const
{
	return EVisibility::Visible;
}

const FSlateBrush* SCurviestCurveEditorTreeLock::GetPinBrush() const
{
	TSharedPtr<FCurveEditor> CurveEditor = WeakCurveEditor.Pin();

	if (CurveEditor)
	{
		if (IsLockedRecursive(TreeItemID, CurveEditor.Get()))
		{
			return FEditorStyle::GetBrush("Level.LockedIcon16x");
		}

	}

	return FEditorStyle::GetBrush("Level.UnlockedIcon16x");
	
}