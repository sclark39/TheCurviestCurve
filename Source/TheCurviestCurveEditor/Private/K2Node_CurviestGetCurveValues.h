// Copyright 2019 Skyler Clark. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "K2Node.h"
#include "Curves/CurveBase.h"
#include "Curves/RichCurve.h"
#include "K2Node_CurviestGetCurveValues.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CurviestGetCurveValues : public UK2Node
{
	GENERATED_BODY()
public:

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

	// UEdGraphNode implementation
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PreloadRequiredAssets() override;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;
	// End of UEdGraphNode implementation

	// K2Node implementation 
	virtual FText GetMenuCategory() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsNodePure() const override { return true; }
	// End of K2Node implementation 

	UFUNCTION()
	void RefreshTemplateCurve();

	UPROPERTY(EditAnywhere, Category = CurviestOptions)
	UCurveBase* TemplateCurve;

	UPROPERTY()
	TArray<FName> OutCurveNames;

};