// Copyright 2019 Skyler Clark. All Rights Reserved.

#include "CurviestCurve.h"

static FName NAME_CurveDefault(TEXT("Curve_0"));

float UCurveCurviestBlueprintUtils::GetValueFromCurve(UCurveBase *Curve, FName Name, float InTime)
{
	if (Curve)
	{
		TArray<FRichCurveEditInfo> EditCurves = Curve->GetCurves();
		for (auto Elem : EditCurves)
		{
			if (Elem.CurveName == Name)
				return Elem.CurveToEdit->Eval(InTime);
		}
	}
	return 0.0f;
}

float UCurveCurviestBlueprintUtils::GetValueFromTaggedCurve(UCurveCurviest *Curve, FGameplayTag Tag, float InTime, bool bAllowParamLookup)
{
	if (Curve)
	{
		float Value = 0.0f;
		Curve->GetFloatValueFromTaggedCurve(Tag, InTime, Value, bAllowParamLookup);
		return Value;
	}
	return 0.0f;
}

UCurveCurviest::UCurveCurviest()
{
	CurveData.Add(FCurviestCurveData(NAME_CurveDefault, FLinearColor::MakeRandomColor()));
	bLookupsNeedRebuild = true;
}

UCurveCurviest::~UCurveCurviest()
{
}

void UCurveCurviest::RebuildLookupMaps()
{
	if (bLookupsNeedRebuild)
	{
		CurveLookupByName.Reset();
		CurveLookupByTag.Reset();
		for (int i = 0; i < CurveData.Num(); i++)
		{
			auto &Data = CurveData[i];
			CurveLookupByName.Add(Data.Name, i);
			CurveLookupByTag.Add(Data.IdentifierTag, i);
		}

		ParamLookupByTag.Reset();
		for (int i = 0; i < Params.Num(); i++)
		{
			auto &Data = Params[i];
			ParamLookupByTag.Add(Data.IdentifierTag, i);
		}


		bLookupsNeedRebuild = false;
	}
}


float UCurveCurviest::GetFloatValue(FName Name, float InTime) const
{
	float ValueOut = 0.0f;
	GetFloatValueFromNamedCurve(Name, InTime, ValueOut);
	return ValueOut;
}


bool UCurveCurviest::GetFloatValueFromNamedCurve(FName Name, float InTime, float &ValueOut) const
{
	const_cast<UCurveCurviest*>(this)->RebuildLookupMaps();

	const int *CurveIdx = CurveLookupByName.Find(Name);
	if (CurveIdx)
	{
		ValueOut = CurveData[*CurveIdx].Curve.Eval(InTime);
		return true;
	}
	return false;
}


bool UCurveCurviest::GetFloatValueFromTaggedCurve(FGameplayTag IdentifierTag, float InTime, float &ValueOut, bool bAllowParamLookup) const
{
	const_cast<UCurveCurviest*>(this)->RebuildLookupMaps();

	const int *CurveIdx = CurveLookupByTag.Find(IdentifierTag);
	if (CurveIdx)
	{
		ValueOut = CurveData[*CurveIdx].Curve.Eval(InTime);
		return true;
	}

	if (bAllowParamLookup)
	{
		const int *ParamIdx = ParamLookupByTag.Find(IdentifierTag);
		if (ParamIdx)
		{
			ValueOut = Params[*ParamIdx].Value;
			return true;
		}
	}

	// Check for parent data
	if (Parent)
		return Parent->GetFloatValueFromTaggedCurve(IdentifierTag, InTime, ValueOut, bAllowParamLookup);

	return false;
}	


bool UCurveCurviest::GetFloatValueFromTaggedParam(FGameplayTag IdentifierTag, float &ValueOut) const
{
	const_cast<UCurveCurviest*>(this)->RebuildLookupMaps();

	const int *ParamIdx = ParamLookupByTag.Find(IdentifierTag);
	if (ParamIdx)
	{
		ValueOut = Params[*ParamIdx].Value;
		return true;
	}

	// Check for parent data
	if (Parent)
		return Parent->GetFloatValueFromTaggedParam(IdentifierTag, ValueOut);

	return false;

}


TArray<FGameplayTag> UCurveCurviest::GetAllCurveIdentifierTags( bool bAllowParamLookup ) const
{
	const_cast<UCurveCurviest*>(this)->RebuildLookupMaps();

	TSet<FGameplayTag> OutTags;
	CurveLookupByTag.GetKeys(OutTags);

	if ( bAllowParamLookup )
	{
		TSet<FGameplayTag> OutParamTags;
		ParamLookupByTag.GetKeys(OutParamTags);
		OutTags = OutTags.Union(OutParamTags);
	}

	return OutTags.Array();
}
	
TArray<FGameplayTag> UCurveCurviest::GetAllParamIdentifierTags() const
{
	const_cast<UCurveCurviest*>(this)->RebuildLookupMaps();

	TSet<FGameplayTag> OutTags;
	CurveLookupByTag.GetKeys(OutTags);

	return OutTags.Array();
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

	if (Curve.Name == NAME_None || Curve.Name == NAME_CurveDefault)
	{
		Curve.Name = NAME_CurveDefault;
		if (Curve.IdentifierTag != FGameplayTag::EmptyTag)
		{
			Curve.Name = Curve.IdentifierTag.GetTagName();
		}
	}

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

void UCurveCurviest::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	const FName PropName = e.GetPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(UCurveCurviest, Parent))
	{
		if (Parent == this)
			Parent = nullptr;
	}
}

void UCurveCurviest::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& e)
{
	Super::PostEditChangeChainProperty(e);

	const FName PropName = e.GetPropertyName();
	const FName ArrayName = e.PropertyChain.GetHead()->GetValue()->GetFName();

	if (ArrayName == GET_MEMBER_NAME_CHECKED(UCurveCurviest, CurveData))
	{
		int CurveIdx = e.GetArrayIndex(GET_MEMBER_NAME_STRING_CHECKED(UCurveCurviest, CurveData));
		switch (e.ChangeType)
		{
		case EPropertyChangeType::ArrayAdd:
			if (OldCurveCount < CurveData.Num())
			{
				CurveData[CurveIdx].Name = NAME_CurveDefault;
				CurveData[CurveIdx].Color = FLinearColor::MakeRandomColor();
				MakeCurveNameUnique(CurveIdx);
			}
			break;

		case EPropertyChangeType::Duplicate:
			// For whatever reason, duplicate adds the new item in the index before the selected
			// but we want to fix up the name on the later one, not the earlier...
			if (0 <= CurveIdx && CurveIdx + 1 < CurveData.Num())
			{
				MakeCurveNameUnique(CurveIdx + 1);
			}
			break;

		case EPropertyChangeType::ValueSet:
			if (0 <= CurveIdx)
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
			break;

		case EPropertyChangeType::ArrayClear:
			break;

		}

		OnCurveMapChanged.Broadcast(this);

		bLookupsNeedRebuild = true;

	}
	else if (ArrayName == GET_MEMBER_NAME_CHECKED(UCurveCurviest, Params))
	{
		bLookupsNeedRebuild = true;
	}
}

#endif