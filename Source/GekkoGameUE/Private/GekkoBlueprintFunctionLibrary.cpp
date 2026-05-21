// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoBlueprintFunctionLibrary.h"


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

