// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "game.h"
#include "GekkoNetSimulationInterface.h"
#include "GekkoNetSubsystem.h"
#include "Containers/RingBuffer.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoGameState.generated.h"

class ARedoReplayManager;
/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoGameState : public AGameStateBase, public IGekkoNetSimulationInterface
{
	GENERATED_BODY()
	
public:
	AGekkoGameState();
	
	void Init();
	virtual void BeginPlay() override;
	
	void ShutdownGame();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void HandleTime();

	virtual void Tick(float DeltaSeconds) override;
	UFUNCTION(BlueprintImplementableEvent)
	void OnUnrealDraw();
	
	// Rewind/Fast Forward to a collected snapshot.
	UFUNCTION(BlueprintCallable)
	void RewindToSnapshot(int32 InFrame);
	// Rewind back a specified number of frames from the current frame.
	UFUNCTION(BlueprintCallable)
	void RewindBackFromCurrentFrame(int32 FramesToRewindBack);
	// Fast forward a specified number of frames from the current frame.
	UFUNCTION(BlueprintCallable)
	void FastForwardFromCurrentFrame(int32 FramesToFastForward);
	
	// Specify a player to take control of a Character whilst a replay is actively running.
	UFUNCTION(BlueprintCallable)
	void TakeoverReplay(int32 InTakeoverIndex);
	// Rewind to the snapshot that was chosen when replay takeover was called.
	UFUNCTION(BlueprintCallable)
	void RewindToTakeoverSnapshot();
	// Exit out of replay takeover mode.
	UFUNCTION(BlueprintCallable)
	void EndTakeover();
	
	void HandleBuffer();
	GekkoGame::Input PollLatestInput(int32 PlayerIndex) const;
	GekkoGame::Input PollInput(int32 PlayerIndex) const;
	
	void UpdateGame();
	void AdvanceGameState(GekkoGame::Input Inputs[], GekkoGameEvent* Event = nullptr);
	
	virtual void GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;
	virtual void GekkoAdvance(GekkoGameEvent* Event) override;
	
	UFUNCTION()
	virtual void OnPlayerDisconnected(int32 Handle);
	
	// Checks if the game is able to rewind at its current frame.
	UFUNCTION(BlueprintCallable)
	bool CanRewind();
	// Check if the game can pause at its current frame.
	UFUNCTION(BlueprintCallable)
	bool CanPause();
	UFUNCTION()
	bool ShouldPauseGame() const;
	// Set whether the game state is paused.
	UFUNCTION(BlueprintCallable)
	void SetGamePaused(bool bPaused);
	// Toggle whether the game state should be paused.
	UFUNCTION(BlueprintCallable)
	void TogglePause();
	
	// Get the replay manager.
	UFUNCTION(BlueprintPure)
	ARedoReplayManager* GetReplayManager() const { return ReplayManager; }
	
	// Get paddle position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetPaddlePosition(int32 index) const;
	// Get ball position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetBallPosition(int32 index) const;
	
	UPROPERTY(BlueprintReadOnly)
	int32 NetLocalPlayerID;
	UPROPERTY(BlueprintReadOnly)
	FGekkoSimpleNetworkStats NetStats;
	
	TRingBuffer<GekkoGame::Input> P1InputBuffer;
	TRingBuffer<GekkoGame::Input> P2InputBuffer;
	
	// Gameplay class that handles collecting inputs and saving them out to a replay.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ARedoReplayManager> ReplayManagerClass;
	// Fallback level on a discconect.
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DisconnectLevel;
	// Whether the Pong game state is paused or not.
	UPROPERTY(BlueprintReadWrite)
	bool bGamePaused = false;
	// Whether a player has taken control of a replay.
	UPROPERTY(BlueprintReadOnly)
	bool bReplayTakeoverEnabled = false;

private:
	int32 ReplayTakeoverIndex = 0;
	int32 ReplayTakeoverSnapshotFrame = 0;
	GekkoGame::Gamestate Gs = {};
	
	float ElapsedTime = 0;
	float ReplayTakeoverStartTimer = 0;
	int32 LocalFrame = 0;
	int32 RemoteFrame = 0;

	UPROPERTY()
	ARedoReplayManager* ReplayManager = nullptr;
	
};
