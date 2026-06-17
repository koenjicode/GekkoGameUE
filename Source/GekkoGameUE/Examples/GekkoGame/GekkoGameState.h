// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "game.h"
#include "GekkoNetSimulationInterface.h"
#include "GekkoNetSubsystem.h"
#include "Containers/RingBuffer.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoGameState.generated.h"

class UGekkoGameInstance;
class AGekkoPlayerState;
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
	bool ShouldSpawnReplayManager() const;

	void CreateInputBuffers();
	void PrepareReplay();
	
protected:
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void HandleReplayTakeoverTimer();

	virtual void Tick(float DeltaSeconds) override;
	UFUNCTION(BlueprintImplementableEvent)
	void OnUnrealDraw();
	
	FName GetOnlineSubsystemName() const;
	bool IsListenConnectedMatch() const;
	int32 GetPlayerID();
	int32 GetOpponentID();
	
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
	
	UFUNCTION(BlueprintPure)
	bool IsPlayingOffline() const;
	
	void UpdateOffline();
	void UpdateOnline();
	void UpdateReplay();
	void UpdateGame();
	void AdvanceGameState(GekkoGame::Input InInputs[], GekkoGameEvent* Event = nullptr);
	
	virtual void GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;
	virtual void GekkoAdvance(GekkoGameEvent* Event) override;
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PlayerDisconnected(int32 Handle);
	
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
	UFUNCTION(BlueprintPure)
	uint8 GetScore(int32 index) const;
	
	UFUNCTION()
	AGekkoPlayerState* GetOpponentState() const;
	UFUNCTION()
	virtual void StartGekkoSession(uint8 InIndex);
	
	UPROPERTY(BlueprintReadOnly)
	int32 NetLocalPlayerID;
	UPROPERTY(BlueprintReadOnly)
	FGekkoNetworkStats NetStats;
	
	TRingBuffer<GekkoGame::Input> P1InputBuffer;
	TRingBuffer<GekkoGame::Input> P2InputBuffer;
	
	// Gameplay class that handles collecting inputs and saving them out to a replay.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ARedoReplayManager> ReplayManagerClass;
	// The transport method currently used.
	UPROPERTY(EditDefaultsOnly)
	EGekkoTransportType GekkoTransportMethod;
	// Fallback level on a discconect.
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DisconnectLevel;
	// Whether the Pong game state is paused or not.
	UPROPERTY(BlueprintReadWrite)
	bool bGamePaused = false;
	// Whether a player has taken control of a replay.
	UPROPERTY(BlueprintReadOnly)
	bool bReplayTakeoverEnabled = false;
	UPROPERTY(VisibleInstanceOnly, Replicated)
	bool bMatchStarted = false;
	UPROPERTY()
	bool bGekkoSessionStarted = false;
	
	UPROPERTY(BlueprintReadOnly)
	ARedoReplayManager* ReplayManager = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UGekkoGameInstance* GekkoGameInstance = nullptr;

private:
	virtual bool HasMatchStarted() const override;
	FString GetOpponentAddress() const;
	FString GetHostAddress();

	GekkoGame::Gamestate Gs = {};
	
	GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS];
		
	UPROPERTY()
	TArray<FGekkoEndpoint> NullEndpoints;
	UPROPERTY(VisibleInstanceOnly)
	int32 ReplayTakeoverIndex = 0;
	UPROPERTY(VisibleInstanceOnly)
	int32 ReplayTakeoverSnapshotFrame = 0;
	
	UPROPERTY(VisibleInstanceOnly)
	float ElapsedTime = 0;
	UPROPERTY(VisibleInstanceOnly)
	float ReplayTakeoverStartTimer = 0;
	UPROPERTY(VisibleInstanceOnly)
	int32 NetworkStatsTimer = 0;
	UPROPERTY(VisibleInstanceOnly)
	int32 LocalFrame = 0;
	UPROPERTY(VisibleInstanceOnly)
	int32 RemoteFrame = 0;
	
};
