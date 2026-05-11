// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedoReplayManager.generated.h"

class URedoReplaySaveData;

UENUM()
enum class ERedoReplayMode : uint8
{
	None,
	Recording,
	Playback,
};

UCLASS()
class REDO_API ARedoReplayManager : public AActor
{
	GENERATED_BODY()

public:
	ARedoReplayManager();
	// Initialize replay manager.
	void Init(int32 InStateSize, int32 InInputSize, int32 InPlayerNum, URedoReplaySaveData* DataToUse = nullptr, int32 SnapshotFrameInterval = 1);
	
	// Record an input structure that will be eventually saved into a replay.
	virtual void RecordInputsForReplay(void* InInputs);
	// Retrieves inputs from a specified frame in the current replay.
	virtual void RetrieveInputsFromReplay(int32 Frame, void* OutInputs);
	// Retrieves inputs from the current frame in the replay.
	virtual void RetrieveInputsFromReplay(void* OutInputs);
	
	// Creates a snapshot based on a specified state.
	virtual void AddSnapshot(void* InSnapshot);
	// Rewinds to a specified snapshot based on a given frame.
	virtual bool RewindToSnapshot(int32 InFrame, void* OutState);
	
	// Find a replay based on a search string.
	virtual URedoReplaySaveData* FindReplay(FString SearchString, int32 Index = 0);
	// Save the replay to disk.
	virtual void SaveReplay();
	
	// Set the replay data to new data.
	void SetReplayData(URedoReplaySaveData* DataToUse);
	// Set the local frame to a new frame.
	void SetLocalFrame(int32 InLocalFrame);
	
	// Advances the Replay Manager by one frame, this should be called after your game state update!
	void AdvanceLocalFrame();
	
	// Checks if the replay manager is currently recording.
	UFUNCTION(BlueprintPure)
	bool IsRecording() const { return CurrentReplayBehaviour == ERedoReplayMode::Recording; }
	// Checks if the replay manager is playing back a replay.
	UFUNCTION(BlueprintPure)
	bool IsPlayingBackReplay() const { return CurrentReplayBehaviour == ERedoReplayMode::Playback; }
	// Retrieve the local replay frame.
	UFUNCTION(BlueprintPure)
	int32 GetReplayFrame() const { return LocalReplayFrame; }
	// Retrieve the replay managers current behavior.
	UFUNCTION(BlueprintPure)
	ERedoReplayMode GetManagerBehaviour() const { return CurrentReplayBehaviour; }
	// Get the current playback data.
	UFUNCTION(BlueprintPure)
	URedoReplaySaveData* GetReplayData() const { return PlaybackData; }
	// Get the replay length in frames.
	UFUNCTION(BlueprintPure)
	int32 GetReplayLengthInFrames() const;
	// Get the replay length.
	UFUNCTION(BlueprintPure)
	float GetReplayLength() const;
	
	// Name of the replay file that gets saved.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString ReplayName = "REPLAY";

private:
	
	ERedoReplayMode CurrentReplayBehaviour = ERedoReplayMode::None;
	int32 LocalReplayFrame;
	
	int32 InputSizePerPlayer;
	int32 NumPlayers;
	
	int32 StateSize;
	TArray<uint8> StateSnapshotBuffer;
	
	TArray<uint8> DataBuffer;
	
	UPROPERTY()
	URedoReplaySaveData* PlaybackData;
};
