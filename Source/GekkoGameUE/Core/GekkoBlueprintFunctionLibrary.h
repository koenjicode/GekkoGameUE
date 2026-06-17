// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GekkoBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API UGekkoBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure)
	static bool IsPlayInEditor();
	
	UFUNCTION(BlueprintPure)
	static int32 GetPlayInEditorID();
	
	UFUNCTION(BlueprintPure)
	static FString GetUniquePlayerIDAsString(int32 PlayerIndex);
};
