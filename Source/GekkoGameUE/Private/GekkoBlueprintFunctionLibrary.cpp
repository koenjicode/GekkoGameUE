// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoBlueprintFunctionLibrary.h"
#include "Online.h"


bool UGekkoBlueprintFunctionLibrary::IsPlayInEditor()
{
#if WITH_EDITOR
	if (GEditor && GEditor->IsPlayingSessionInEditor())
	{
		return true;
	}
#endif
	return false;
}

int32 UGekkoBlueprintFunctionLibrary::GetPlayInEditorID()
{
#if WITH_EDITOR
	if (GEditor)
	{
		return UE::GetPlayInEditorID();
	}
#endif
	return 0;
}

FString UGekkoBlueprintFunctionLibrary::GetUniquePlayerIDAsString(int32 PlayerIndex)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface();

	if (Identity.IsValid())
	{
		TSharedPtr<const FUniqueNetId> NetId = Identity->GetUniquePlayerId(PlayerIndex);

		if (NetId.IsValid())
		{
			FString IdString = NetId->ToString();
			return IdString;
		}
	}
	
	return FString();
}

