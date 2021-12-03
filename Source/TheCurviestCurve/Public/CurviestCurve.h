// Copyright 2019 Skyler Clark. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/RichCurve.h"
#include "Curves/CurveBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "CurviestCurve.generated.h"

UCLASS()
class THECURVIESTCURVE_API UCurveCurviestBlueprintUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math|Curves", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static float GetValueFromCurve(UCurveBase *Curve, FName Name, float InTime);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math|Curves", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static float GetValueFromTaggedCurve(UCurveCurviest *Curve, FGameplayTag Tag, float InTime, bool bAllowParamLookup = true);

};

USTRUCT(BlueprintType)
struct FCurviestCurveData
{
	GENERATED_BODY()

public:
	// Identifying Curve Name
	UPROPERTY(EditAnywhere, Category = "Curviest")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Curviest")
	FGameplayTag IdentifierTag;

	UPROPERTY(EditAnywhere, Category = "Curviest")
	FLinearColor Color;

	UPROPERTY()
	FRichCurve Curve;

	FCurviestCurveData() 
	{
		this->Color = FLinearColor::White;
	}
	FCurviestCurveData(FName Name, FLinearColor Color)
	{
		this->Name = Name;
		this->Color = Color;
	}
};

USTRUCT(BlueprintType)
struct FCurviestCurveFloatParam
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Curviest")
	FGameplayTag IdentifierTag;

	UPROPERTY(EditAnywhere, Category = "Curviest")
	float Value = 0.0f;
};

UCLASS(BlueprintType, collapsecategories, hidecategories = (FilePath))
class THECURVIESTCURVE_API UCurveCurviest : public UCurveBase
{
	GENERATED_BODY()

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCurveMapChanged, UCurveBase*);

public:

	UCurveCurviest();
	~UCurveCurviest();

	/*UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	TArray<FName> GetCurveNames() const {
		return CurveNames.Array();
	}*/

	/** Evaluate this float curve at the specified time */
	UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	float GetFloatValue(FName Name, float InTime) const;

	UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	bool GetFloatValueFromNamedCurve(FName Name, float InTime, float &ValueOut) const;

	UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	bool GetFloatValueFromTaggedCurve(FGameplayTag IdentifierTag, float InTime, float &ValueOut, bool bAllowParamLookup = true) const;

	UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	bool GetFloatValueFromTaggedParam(FGameplayTag IdentifierTag, float &ValueOut) const;

	// Begin FCurveOwnerInterface
	virtual TArray<FRichCurveEditInfoConst> GetCurves() const override;
	virtual TArray<FRichCurveEditInfo> GetCurves() override;

	/** @return Color for this curve */
	virtual FLinearColor GetCurveColor(FRichCurveEditInfo CurveInfo) const override;

	/** Determine if Curve is the same */
	bool operator == (const UCurveCurviest& Curve) const;

	virtual bool IsValidCurve(FRichCurveEditInfo CurveInfo) override;

#if WITH_EDITOR
	void MakeCurveNameUnique(int CurveIdx);

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

	virtual void PreEditChange(class FEditPropertyChain& e) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& e) override;
	//virtual void OnCurveChanged(const TArray<FRichCurveEditInfo>& ChangedCurveEditInfos) override;
#endif

#if WITH_EDITORONLY_DATA
	FOnCurveMapChanged OnCurveMapChanged;
#endif

	UPROPERTY(EditAnywhere, Category = "Curviest")
	UCurveCurviest *Parent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Curviest", meta = (NoResetToDefault))
	TArray<FCurviestCurveData> CurveData;

	UPROPERTY(EditAnywhere, Category = "Curviest", meta = (NoResetToDefault))
	TArray<FCurviestCurveFloatParam> Params;

	void RebuildLookupMaps();

	bool bLookupsNeedRebuild = true;
	TMap<FName, int> CurveLookupByName;
	TMap<FGameplayTag, int> CurveLookupByTag;
	TMap<FGameplayTag, int> ParamLookupByTag;

protected:
	int OldCurveCount;


};
