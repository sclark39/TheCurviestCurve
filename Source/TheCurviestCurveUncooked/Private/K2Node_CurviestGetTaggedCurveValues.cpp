// Copyright 2019 Skyler Clark. All Rights Reserved.

#include "K2Node_CurviestGetTaggedCurveValues.h"

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

struct FTaggedCurveValuesGetPinName {

	static const FName& GetTargetPin() {
		static const FName PinName(TEXT("Target"));
		return PinName;
	}

	static const FName& GetInTimePin() {
		static const FName PinName(TEXT("InTime"));
		return PinName;
	}

	static const FName& GetTagsPin() {
		static const FName PinName(TEXT("Tags"));
		return PinName;
	}

	static const FName& GetAllowParamLookupPin() {
		static const FName PinName(TEXT("bAllowParamLookup"));
		return PinName;
	}
};

void UK2Node_CurviestGetTaggedCurveValues::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	bool bIsDirty = false;
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("Tags") || PropertyName == TEXT("bAllowParamLookup"))
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

void UK2Node_CurviestGetTaggedCurveValues::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create our pins

	// Input
	UEdGraphPin* InTargetPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UCurveBase::StaticClass(), FTaggedCurveValuesGetPinName::GetTargetPin());
	
#if ENGINE_MAJOR_VERSION >= 5
	UEdGraphPin* InValuePin = CreatePin(EGPD_Input,  UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Double, FTaggedCurveValuesGetPinName::GetInTimePin());
#else
	UEdGraphPin* InValuePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Float, FTaggedCurveValuesGetPinName::GetInTimePin());
#endif 
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(InValuePin);

	UEdGraphPin* TagsPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FGameplayTagContainer::StaticStruct(), FTaggedCurveValuesGetPinName::GetTagsPin());
	K2Schema->SetPinAutogeneratedDefaultValue(TagsPin, Tags.ToString());
	TagsPin->bAdvancedView = true;

	UEdGraphPin* AllowParamLookupPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, FTaggedCurveValuesGetPinName::GetAllowParamLookupPin());
	K2Schema->SetPinAutogeneratedDefaultValue(AllowParamLookupPin, bAllowParamLookup ? TEXT("true") : TEXT("false"));
	AllowParamLookupPin->bAdvancedView = true;

	// Output

	// Refresh
	for (FGameplayTag Tag : Tags)
	{
#if ENGINE_MAJOR_VERSION >= 5
		UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Double, Tag.GetTagName());
#else
		UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Float, Tag.GetTagName());
#endif 
		//OutValidPin->bAdvancedView = true;
		K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);
	}

	if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Shown;
	}
}

FText UK2Node_CurviestGetTaggedCurveValues::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("K2Node_CurviestGetTaggedCurveValues_Title", "Get Tagged Curve Values");
}

FText UK2Node_CurviestGetTaggedCurveValues::GetTooltipText() const
{
	return LOCTEXT("K2Node_CurviestGetCurveValues_Tooltip", "Easily get float values out of any curve.");
}

FSlateIcon UK2Node_CurviestGetTaggedCurveValues::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

void UK2Node_CurviestGetTaggedCurveValues::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin == FindPin(FTaggedCurveValuesGetPinName::GetTagsPin()))
	{
		Tags.FromExportString(Pin->DefaultValue);

		ReconstructNode();
		GetGraph()->NotifyGraphChanged();
	}
	else if (Pin == FindPin(FTaggedCurveValuesGetPinName::GetAllowParamLookupPin()))
	{
		bAllowParamLookup = Pin->DefaultValue == TEXT("true");
	}
}

void UK2Node_CurviestGetTaggedCurveValues::PreloadRequiredAssets()
{
	Super::PreloadRequiredAssets();
}


FText UK2Node_CurviestGetTaggedCurveValues::GetMenuCategory() const
{
	return LOCTEXT("K2Node_CurviestGetCurveValues_MenuCategory", "Curves");
}

void UK2Node_CurviestGetTaggedCurveValues::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UFunction* Func_GetValueFromCurve = UCurveCurviestBlueprintUtils::StaticClass()->FindFunctionByName(FName(TEXT("GetValueFromTaggedCurve")));
	
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	for (FGameplayTag Tag : Tags)
	{
		FName CurveName = Tag.GetTagName();

		UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallFuncNode->SetFromFunction(Func_GetValueFromCurve);
		CallFuncNode->AllocateDefaultPins();

		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFuncNode, this);

		//Input
		CompilerContext.CopyPinLinksToIntermediate(*FindPin(FTaggedCurveValuesGetPinName::GetTargetPin()), *CallFuncNode->FindPin(TEXT("Curve")));

		UEdGraphPin* TagPin = CallFuncNode->FindPin(TEXT("Tag"));		
		K2Schema->SetPinAutogeneratedDefaultValue(TagPin, CurveName.ToString());

		UEdGraphPin* AllowParamLookupPin = CallFuncNode->FindPin(TEXT("bAllowParamLookup"));
		K2Schema->SetPinAutogeneratedDefaultValue(AllowParamLookupPin, bAllowParamLookup ? "true" : "false" );

		CompilerContext.CopyPinLinksToIntermediate(*FindPin(FTaggedCurveValuesGetPinName::GetInTimePin()), *CallFuncNode->FindPin(TEXT("InTime")));

		//Output
		CompilerContext.MovePinLinksToIntermediate(*FindPin(CurveName), *CallFuncNode->GetReturnValuePin());
	}

	BreakAllNodeLinks();
}


//This method adds our node to the context menu
void UK2Node_CurviestGetTaggedCurveValues::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UK2Node_CurviestGetTaggedCurveValues::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	/*
	FToolMenuSection& Section = Menu->AddSection("K2NodeCurviestGetCurveValues", LOCTEXT("CurviestGetCurveValuesHeader", "Curviest"));
	Section.AddMenuEntry(
		"RefreshNode",
		LOCTEXT("RefreshNode", "Refresh Node"),
		LOCTEXT("RefreshNodeTooltip", "Refresh this node."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateUObject(const_cast<UK2Node_CurviestGetTaggedCurveValues*>(this), &UK2Node_CurviestGetTaggedCurveValues::RefreshTemplateCurve),
			FCanExecuteAction(),
			FIsActionChecked()
		)
	);*/
}


#undef LOCTEXT_NAMESPACE