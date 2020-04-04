// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "CurveEditorTypes.h"
#include "Tree/CurveEditorTreeTraits.h"

class FCurveEditor;
class ITableRow;

class SCurviestCurveEditorTreeLock : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCurviestCurveEditorTreeLock) {}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& InTableRow);

private:

	FReply ToggleLocked();

	const FSlateBrush* GetPinBrush() const;

	bool IsLockedRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const;

	void LockRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const;
	void UnlockRecursive(FCurveEditorTreeItemID InTreeItem, FCurveEditor* CurveEditor) const;

	EVisibility GetPinVisibility() const;

private:

	TWeakPtr<ITableRow> WeakTableRow;
	TWeakPtr<FCurveEditor> WeakCurveEditor;
	FCurveEditorTreeItemID TreeItemID;
};