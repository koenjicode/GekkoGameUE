// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoBlueprintFunctionLibrary.h"

bool UGekkoBlueprintFunctionLibrary::IsPlayInEditor()
{
	if (GEditor && GEditor->IsPlayingSessionInEditor())
	{
		return true;
	}
	return false;
}

int32 UGekkoBlueprintFunctionLibrary::GetPlayInEditorID()
{
	return UE::GetPlayInEditorID();
}
