// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedoReplayDriver.generated.h"

class URedoReplaySaveData;

UENUM()
enum class ERedoReplayMode : uint8
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
	
	void Init(int32 InStateSize, int32 InInputSize, int32 InPlayerNum, URedoReplaySaveData* DataToUse);
	void AdvanceLocalFrame();
	
	// TODO: Implement core code functions in these updates to make the library easier to use in a plug and play fashion.
	virtual void UpdateRecording(void* InInputs, bool bAdvanceReplayFrame = true);
	virtual void UpdatePlayback(void* OutInputs, bool bAdvanceReplayFrame = true);
	virtual void UpdatePlaybackTakeover(int32 PlayerIndex);
	
	virtual void AddSnapshot(void* InSnapshot);
	virtual bool RewindToSnapshot(int32 InFrame, void* OutState);
	
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
	
	bool IsRecording() const { return CurrentDriverState == ERedoReplayMode::Recording; }
	ERedoReplayMode GetDriverState() const { return CurrentDriverState; }
	URedoReplaySaveData* GetReplayData() const { return PlaybackData; }
	
	
private:
	ERedoReplayMode CurrentDriverState = ERedoReplayMode::None;
	int32 LocalReplayFrame;
	
	int32 InputSizePerPlayer;
	int32 NumPlayers;
	
	int32 StateSize;
	TArray<uint8> StateSnapshotBuffer;
	
	int32 CurrentFrame;
	
	TArray<uint8> DataBuffer;
	
	UPROPERTY()
	URedoReplaySaveData* PlaybackData;
};
