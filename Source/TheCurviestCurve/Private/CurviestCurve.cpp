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
	CurveData.Add(FCurviestCurveData("Curviest.Curve_0", FLinearColor::MakeRandomColor()));
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



void UCurveCurviest::MakeCurveNameUnique( int CurveIdx )
{
	FCurviestCurveData &Curve = CurveData[CurveIdx];

	// Find Name Base
	FString BaseName = Curve.Name.ToString();
	int NewNameIdx = 0;
	FString NewName = BaseName;

	int UnderscoreIdx;
	if (BaseName.FindLastChar('_', UnderscoreIdx))
	{
		FString Left = BaseName.Left(UnderscoreIdx);
		FString Right = BaseName.RightChop(UnderscoreIdx + 1);
		if (Right.IsNumeric())
		{
			BaseName = Left;
			NewNameIdx = FCString::Atoi(*Right);
		}
	}

	// Collect Names
	TSet<FName> NameList;
	for (int i = 0; i < CurveData.Num(); i++)
	{
		if (i != CurveIdx)
		{
			NameList.Add(CurveData[i].Name);
		}
	}

	// Find Unique Name
	while (NameList.Contains(FName(*NewName)))
	{
		NewName = FString::Printf(TEXT("%s_%d"), *BaseName, ++NewNameIdx);
	}

	Curve.Name = FName(*NewName);
}

void UCurveCurviest::PreEditChange(class FEditPropertyChain& PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	OldCurveCount = CurveData.Num();

}

void UCurveCurviest::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& e)
{
	Super::PostEditChangeChainProperty(e);
	
	const FName PropName = e.GetPropertyName();
	int CurveIdx = e.GetArrayIndex("CurveData");
	
	switch (e.ChangeType)
	{
		case EPropertyChangeType::ArrayAdd:
			if (OldCurveCount < CurveData.Num())
			{
				CurveData[CurveIdx].Name = "Curviest.Curve_0";
				CurveData[CurveIdx].Color = FLinearColor::MakeRandomColor();
				MakeCurveNameUnique(CurveIdx);
			}
			break;

		case EPropertyChangeType::ValueSet:
		{
			if (0 < CurveIdx)
			{
				if (PropName == "Name")
				{
					MakeCurveNameUnique(CurveIdx);
				}
				if (PropName == "Color")
				{
					CurveData[CurveIdx].Color.A = 1.0f;
				}
			}
		}
			break;

		case EPropertyChangeType::ArrayClear:
			break;
		
	}

	OnCurveMapChanged.Broadcast( this );
}

#endif