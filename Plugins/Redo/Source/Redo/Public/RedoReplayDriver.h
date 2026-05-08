// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedoReplayDriver.generated.h"

class URedoReplaySaveData;

UENUM()
enum class ERedoDriverType : uint8
{
	Recording,
	Playback,
};

UCLASS()
class REDO_API ARedoReplayDriver : public AActor
{
	GENERATED_BODY()

public:
	ARedoReplayDriver();
	
	void Init(int32 StoreInputSize, int32 NumPlayers);
	
	void RecordFrame(int32 Frame, void* Inputs) const;
	void GetInputsForFrame(int32 Frame, void& Inputs);

	void SetReplayData(URedoReplaySaveData* DataToUse);

	bool bIsRecording;
	
private:
	UPROPERTY()
	URedoReplaySaveData* ReplayData;
};
