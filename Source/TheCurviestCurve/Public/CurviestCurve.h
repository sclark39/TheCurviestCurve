// Copyright 2019 Skyler Clark. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/RichCurve.h"
#include "Curves/CurveBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CurviestCurve.generated.h"

UCLASS()
class THECURVIESTCURVE_API UCurveCurviestBlueprintUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math|Curves", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static float GetValueFromCurve(UCurveBase *Curve, FName Name, float InTime);
};

USTRUCT()
struct FCurviestCurveData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Name;

	UPROPERTY()
	FLinearColor Color;

	UPROPERTY()
	FRichCurve Curve;

	FCurviestCurveData() {}
	FCurviestCurveData(FName Name, FLinearColor Color) 
	{
		this->Name = Name;
		this->Color = Color;
	}
};

UCLASS(BlueprintType, collapsecategories, hidecategories = (FilePath))
class THECURVIESTCURVE_API UCurveCurviest : public UCurveBase
{
	GENERATED_BODY()

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCurveMapChanged, UCurveBase*);

public:

	UCurveCurviest();
	~UCurveCurviest();

	UPROPERTY(EditAnywhere, Category = "Curviest")
	TSet<FName> CurveNames;

	UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	TArray<FName> GetCurveNames() const {
		return CurveNames.Array();
	}

	/** Evaluate this float curve at the specified time */
	UFUNCTION(BlueprintCallable, Category = "Math|Curves")
	float GetFloatValue(FName Name, float InTime) const;

	// Begin FCurveOwnerInterface
	virtual TArray<FRichCurveEditInfoConst> GetCurves() const override;
	virtual TArray<FRichCurveEditInfo> GetCurves() override;

	/** @return Color for this curve */
	virtual FLinearColor GetCurveColor(FRichCurveEditInfo CurveInfo) const override;

	/** Determine if Curve is the same */
	bool operator == (const UCurveCurviest& Curve) const;

	virtual bool IsValidCurve(FRichCurveEditInfo CurveInfo) override;

#if WITH_EDITOR
	virtual void PreEditChange(class FEditPropertyChain& PropertyAboutToChange) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedChainEvent) override;
	//virtual void OnCurveChanged(const TArray<FRichCurveEditInfo>& ChangedCurveEditInfos) override;
#endif

#if WITH_EDITORONLY_DATA
	FOnCurveMapChanged OnCurveMapChanged;
#endif

protected:
	TSet<FName> OldCurveNames;

	UPROPERTY()
	TArray<FCurviestCurveData> CurveData;

};
