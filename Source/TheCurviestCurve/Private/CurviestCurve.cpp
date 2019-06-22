// Copyright 2019 Skyler Clark. All Rights Reserved.

#include "CurviestCurve.h"

float UCurveCurviestBlueprintUtils::GetValueFromCurve(UCurveBase *Curve, FName Name, float InTime)
{
	TArray<FRichCurveEditInfo> EditCurves = Curve->GetCurves();
	for (auto Elem : EditCurves)
	{
		if (Elem.CurveName == Name)
			return Elem.CurveToEdit->Eval(InTime);
	}

	return 0.0f;
}


UCurveCurviest::UCurveCurviest()
{
}

UCurveCurviest::~UCurveCurviest()
{
}


float UCurveCurviest::GetFloatValue(FName Key, float InTime) const
{
	for (const auto& Data : CurveData)
		if (Data.Name == Key)
			return Data.Curve.Eval(InTime);
	return 0;
}

TArray<FRichCurveEditInfoConst> UCurveCurviest::GetCurves() const
{
	TArray<FRichCurveEditInfoConst> CurveEditInfos;
	for (const auto& Data : CurveData)
		CurveEditInfos.Add(FRichCurveEditInfoConst(&Data.Curve, Data.Name));
	return CurveEditInfos;
}

TArray<FRichCurveEditInfo> UCurveCurviest::GetCurves()
{
	TArray<FRichCurveEditInfo> CurveEditInfos;
	for (auto& Data : CurveData)
		CurveEditInfos.Add(FRichCurveEditInfo(&Data.Curve, Data.Name));
	return CurveEditInfos;
}

bool UCurveCurviest::IsValidCurve(FRichCurveEditInfo CurveInfo)
{
	for (const auto& Data : CurveData)
		if (&Data.Curve == CurveInfo.CurveToEdit)
			return true;
	return false;
}

FLinearColor UCurveCurviest::GetCurveColor(FRichCurveEditInfo CurveInfo) const
{
	for (const auto& Data : CurveData)
		if (CurveInfo.CurveToEdit == &Data.Curve)
			return Data.Color;
	return FLinearColor::White;
}


bool UCurveCurviest::operator==(const UCurveCurviest& Curve) const
{
	for (int i = 0; i < CurveData.Num(); i++)
		if (!( CurveData[i].Curve == Curve.CurveData[i].Curve))
			return false;
	return true;
}


#if WITH_EDITOR

void UCurveCurviest::PreEditChange(class FEditPropertyChain& PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	OldCurveNames = CurveNames;
}

void UCurveCurviest::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& e)
{
	Super::PostEditChangeChainProperty(e);
	
	TArray<FName> Added = CurveNames.Difference(OldCurveNames).Array();
	TArray<FName> Removed = OldCurveNames.Difference(CurveNames).Array();

	switch (e.ChangeType)
	{
	case EPropertyChangeType::ArrayAdd:
		if (Added.Num())
		{
			CurveData.Add(FCurviestCurveData( Added[0], FLinearColor::MakeRandomColor() ));
			UE_LOG(LogTemp, Log, TEXT("Added new %s"), *Added[0].ToString());
		}
		break;

	case EPropertyChangeType::ArrayRemove:
		if (Removed.Num())
		{
			for (int Idx = CurveData.Num(); Idx-- > 0;)
			{
				if (CurveData[Idx].Name == Removed[0])
				{
					CurveData.RemoveAt(Idx);
					break;
				}
			}

			CurveNames.CompactStable();

			UE_LOG(LogTemp, Log, TEXT("Deleted %s"), *Removed[0].ToString());
		}
		break;

	case EPropertyChangeType::ValueSet:
		if (Added.Num() && Removed.Num())
		{
			for (auto& Data : CurveData)
				if (Data.Name == Removed[0])
				{
					Data.Name = Added[0];
					//Data.Color = FLinearColor::MakeRandomColor();
				}

			UE_LOG(LogTemp, Log, TEXT("Changed value from %s to %s"), *Added[0].ToString(), *Removed[0].ToString());
		}
		break;

	case EPropertyChangeType::ArrayClear:
		CurveData.Empty();
		UE_LOG(LogTemp, Log, TEXT("Cleared Array"));
		break;
	}
	

	OnCurveMapChanged.Broadcast( this );
}

#endif