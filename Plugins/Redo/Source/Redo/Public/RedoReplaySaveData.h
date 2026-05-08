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

	virtual void FillHeader(int32 InputSize, int32 NumOfPlayers);
	void SaveInputs(int32 Frame, void* Inputs);
	void GetInputs(int32 Frame, void& OutInputs);

	// header

	uint32 InputSizePerPlayer;
	uint32 PlayerCount;

	// stored data

	TArray<uint8> Data;
};
