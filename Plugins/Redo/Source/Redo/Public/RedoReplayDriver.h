// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedoReplayDriver.generated.h"

class URedoReplaySaveData;

UENUM()
enum class ERedoDriverType : uint8
{
	None,
	Recording,
	Playback,
};

UCLASS()
class REDO_API ARedoReplayDriver : public AActor
{
	GENERATED_BODY()

public:
	ARedoReplayDriver();
	
	void Init(int32 StoreInputSize, int32 NumPlayers, URedoReplaySaveData* DataToUse);
	void Init(int32 StoreInputSize, int32 NumPlayers);
	void AdvanceLocalFrame();
	
	// record a frame.
	virtual void RecordInputs(void* InInputs);
	// read a specified frame
	virtual void ReciteInputs(int32 Frame, void* OutInputs);
	// read the current localreplayframe.
	virtual void ReciteInputs(void* OutInputs);
	// reads a specified frame for a particular player index.
	virtual void ReciteInputsForPlayer(int32 Frame, int32 ForPlayerIndex, void* OutInputs);
	// reads a specified frame for a particular player index.
	virtual void ReciteInputsForPlayer(int32 ForPlayerIndex, void* OutInputs);
	
	virtual URedoReplaySaveData* FindReplay();
	virtual void SaveReplay();

	void SetReplayData(URedoReplaySaveData* DataToUse);
	void SetLocalFrame(int32 NewLocalFrame);
	
	bool IsRecording() const { return CurrentDriverState == ERedoDriverType::Recording; }
	ERedoDriverType GetDriverState() const { return CurrentDriverState; }
	
private:
	ERedoDriverType CurrentDriverState = ERedoDriverType::None;
	int32 LocalReplayFrame;

	int32 InputSizePerPlayer;
	int32 NumOfPlayers;
	
	int32 CurrentFrame;
	
	TArray<uint8> DataBuffer;
	
	UPROPERTY()
	URedoReplaySaveData* PlaybackReplayData;
};
