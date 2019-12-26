// Copyright 2019 Skyler Clark. All Rights Reserved.

#include "K2Node_CurviestGetCurveValues.h"

#include "Curves/CurveVector.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "CurviestCurve.h"
#include "KismetCompiler.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "GraphEditorSettings.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#if ENGINE_MINOR_VERSION >= 24
#include "ToolMenus.h"
#endif

#define LOCTEXT_NAMESPACE "K2Node_Curviest"

struct FGetPinName {

	static const FName& GetTargetPin() {
		static const FName PinName(TEXT("Target"));
		return PinName;
	}

	static const FName& GetInTimePin() {
		static const FName PinName(TEXT("InTime"));
		return PinName;
	}

	static const FName& GetOutputPin1() {
		static const FName OutputPinName(TEXT("Fred"));
		return OutputPinName;
	}

	static const FName& GetOutputPin2() {
		static const FName OutputPinName(TEXT("Carol"));
		return OutputPinName;
	}
};

void UK2Node_CurviestGetCurveValues::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	bool bIsDirty = false;
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("TemplateCurve"))
	{
		bIsDirty = true;
	}

	if (bIsDirty)
	{
		ReconstructNode();
		GetGraph()->NotifyGraphChanged();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_CurviestGetCurveValues::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create our pins

	// Input
	UEdGraphPin* InTargetPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UCurveBase::StaticClass(), FGetPinName::GetTargetPin());
	
	UEdGraphPin* InValuePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Float, FGetPinName::GetInTimePin());
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(InValuePin);
	
	// Output

	// Refresh
	if (TemplateCurve)
	{
		OutCurveNames.Empty();
		TArray<FRichCurveEditInfo> EditCurves = TemplateCurve->GetCurves();
		for (FRichCurveEditInfo Elem : EditCurves)
			OutCurveNames.Emplace(Elem.CurveName);
	}


	for (FName CurveName : OutCurveNames)
	{
		UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Float, CurveName);
		OutValidPin->bAdvancedView = true;
		K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);
	}

	if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Shown;
	}
}

FText UK2Node_CurviestGetCurveValues::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("K2Node_CurviestGetCurveValues_Title", "Get Curve Values");
}

FText UK2Node_CurviestGetCurveValues::GetTooltipText() const
{
	return LOCTEXT("K2Node_CurviestGetCurveValues_Tooltip", "Easily get float values out of any curve.");
}

FSlateIcon UK2Node_CurviestGetCurveValues::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

void UK2Node_CurviestGetCurveValues::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin == FindPin(FGetPinName::GetTargetPin()))
	{
		if (UCurveBase *Curve = Cast<UCurveBase>(Pin->DefaultObject))
		{
			TemplateCurve = Curve;
			PreloadObject(TemplateCurve);
			TemplateCurve->ConditionalPostLoad();

			RefreshTemplateCurve();
		}
	}
}

void UK2Node_CurviestGetCurveValues::PreloadRequiredAssets()
{
	Super::PreloadRequiredAssets();

	PreloadObject(TemplateCurve);
}


FText UK2Node_CurviestGetCurveValues::GetMenuCategory() const
{
	return LOCTEXT("K2Node_CurviestGetCurveValues_MenuCategory", "Curves");
}

void UK2Node_CurviestGetCurveValues::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UFunction* Func_GetValueFromCurve = UCurveCurviestBlueprintUtils::StaticClass()->FindFunctionByName(FName(TEXT("GetValueFromCurve")));
	
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	for (FName CurveName : OutCurveNames)
	{
		UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallFuncNode->SetFromFunction(Func_GetValueFromCurve);
		CallFuncNode->AllocateDefaultPins();

		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFuncNode, this);

		//Input
		CompilerContext.CopyPinLinksToIntermediate(*FindPin(FGetPinName::GetTargetPin()), *CallFuncNode->FindPin(TEXT("Curve")));

		UEdGraphPin* NamePin = CallFuncNode->FindPin(TEXT("Name"));
		K2Schema->SetPinAutogeneratedDefaultValue(NamePin, CurveName.ToString());

		CompilerContext.CopyPinLinksToIntermediate(*FindPin(FGetPinName::GetInTimePin()), *CallFuncNode->FindPin(TEXT("InTime")));

		//Output
		CompilerContext.MovePinLinksToIntermediate(*FindPin(CurveName), *CallFuncNode->GetReturnValuePin());
	}

	BreakAllNodeLinks();
}


//This method adds our node to the context menu
void UK2Node_CurviestGetCurveValues::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

#if ENGINE_MINOR_VERSION < 24
void UK2Node_CurviestGetCurveValues::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	Super::GetContextMenuActions(Context);

	Context.MenuBuilder->BeginSection("K2NodeVariableGet", LOCTEXT("K2Node_CurviestGetCurveValues_Header", "Curviest"));
	{
		Context.MenuBuilder->AddMenuEntry(
			LOCTEXT("RefreshNode", "Refresh Node"), 
			LOCTEXT("RefreshNodeTooltip", "Refresh this node."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(this, &UK2Node_CurviestGetCurveValues::RefreshTemplateCurve),
				FCanExecuteAction(),
				FIsActionChecked()
			)
		);
	}

	Context.MenuBuilder->EndSection();
}
#else
void UK2Node_CurviestGetCurveValues::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	FToolMenuSection& Section = Menu->AddSection("K2NodeCurviestGetCurveValues", LOCTEXT("CurviestGetCurveValuesHeader", "Curviest"));
	Section.AddMenuEntry(
		"RefreshNode",
		LOCTEXT("RefreshNode", "Refresh Node"),
		LOCTEXT("RefreshNodeTooltip", "Refresh this node."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateUObject(this, &UK2Node_CurviestGetCurveValues::RefreshTemplateCurve),
			FCanExecuteAction(),
			FIsActionChecked()
		)
	);
}
#endif

void UK2Node_CurviestGetCurveValues::RefreshTemplateCurve()
{
	if (TemplateCurve)
	{
		OutCurveNames.Empty();
		TArray<FRichCurveEditInfo> EditCurves = TemplateCurve->GetCurves();
		for (FRichCurveEditInfo Elem : EditCurves)
			OutCurveNames.Emplace(Elem.CurveName);
	}

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

#undef LOCTEXT_NAMESPACE