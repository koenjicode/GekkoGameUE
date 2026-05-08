// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedoReplayDriver.generated.h"

UENUM()
enum class ERedoReplayState : uint8
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
	
	void InitReplayDriver();
	
	void RecordFrame();
	void PlayFrame();
	void RewindFrame();
	
private:
	// stored in the replay file
	
	int32 InputSize;
	
	// allocated during playback
	
	ERedoReplayState ReplayState;
	int32 SnapshotsPerFrame;
	int32 StateSize;
};
