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
	
	virtual void AddSnapshot(void* InSnapshot);
	virtual bool RewindToSnapshot(int32 InFrame, void* OutState);
	
	// record a frame.
	virtual void RecordInputs(void* InInputs);
	// read a specified frame
	virtual void ReciteInputs(int32 Frame, void* OutInputs);
	// read the current localreplayframe.
	virtual void ReciteInputs(void* OutInputs);
	
	virtual URedoReplaySaveData* FindReplay();
	virtual void SaveReplay();

	void SetReplayData(URedoReplaySaveData* DataToUse);
	void SetLocalFrame(int32 NewLocalFrame);
	
	UFUNCTION(BlueprintPure)
	bool IsRecording() const { return CurrentDriverState == ERedoReplayMode::Recording; }
	UFUNCTION(BlueprintPure)
	bool IsPlayingBackReplay() const { return CurrentDriverState == ERedoReplayMode::Playback; }
	UFUNCTION(BlueprintPure)
	int32 GetReplayFrame() const { return LocalReplayFrame; }
	UFUNCTION(BlueprintPure)
	ERedoReplayMode GetDriverState() const { return CurrentDriverState; }
	UFUNCTION(BlueprintPure)
	URedoReplaySaveData* GetReplayData() const { return PlaybackData; }
	UFUNCTION(BlueprintPure)
	int32 GetReplayLengthInFrames() const;
	
	
private:
	ERedoReplayMode CurrentDriverState = ERedoReplayMode::None;
	int32 LocalReplayFrame;
	
	int32 InputSizePerPlayer;
	int32 NumPlayers;
	
	int32 StateSize;
	TArray<uint8> StateSnapshotBuffer;
	
	TArray<uint8> DataBuffer;
	
	UPROPERTY()
	URedoReplaySaveData* PlaybackData;
};
