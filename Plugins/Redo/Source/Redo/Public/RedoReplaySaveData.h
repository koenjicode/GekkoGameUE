// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RedoReplaySaveData.generated.h"

/**
 * 
 */
UCLASS()
class REDO_API URedoReplaySaveData : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly)  
	FDateTime Timestamp;
	UPROPERTY(BlueprintReadOnly)
	int32 ReplayLengthInFrames;
	UPROPERTY(BlueprintReadOnly)  
	int32 InputSizePerPlayer;
	UPROPERTY(BlueprintReadOnly)  
	int32 PlayerCount;
	UPROPERTY()
	TArray<uint8> Data;
};
