// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "game.h"
#include "GekkoNetSimulationInterface.h"
#include "Containers/RingBuffer.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoGameState.generated.h"

class ARedoReplayDriver;
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
	
	UFUNCTION(BlueprintCallable)
	void RewindToSnapshot(int32 InFrame);
	UFUNCTION(BlueprintCallable)
	void RewindBackFromCurrentFrame(int32 FramesToRewindBack);
	UFUNCTION(BlueprintCallable)
	void FastForwardFromCurrentFrame(int32 FramesToFastForward);
	
	UFUNCTION(BlueprintCallable)
	void TakeoverReplay(int32 InTakeoverIndex);
	UFUNCTION(BlueprintCallable)
	void RewindToTakeoverSnapshot();
	UFUNCTION(BlueprintCallable)
	void EndTakeover();
	
	// input gathering
	void HandleBuffer();
	GekkoGame::Input PollLatestInput(int32 PlayerIndex) const;
	GekkoGame::Input PollInput(int32 PlayerIndex) const;
	
	void UpdateGame();
	void AdvanceGameState(GekkoGame::Input Inputs[], GekkoGameEvent* Event = nullptr);
	
	virtual void GekkoGetLocalInputs(void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;
	virtual void GekkoAdvance(GekkoGameEvent* Event, bool Render) override;
	virtual void GekkoDisconnect(GekkoSessionEvent* Event) override;
	
	UFUNCTION(BlueprintCallable)
	bool CanRewind();
	UFUNCTION(BlueprintCallable)
	bool CanPause();
	UFUNCTION()
	bool ShouldPauseGame() const;
	UFUNCTION(BlueprintCallable)
	void SetGamePaused(bool bPaused);
	UFUNCTION(BlueprintCallable)
	void TogglePause();
	
	UFUNCTION(BlueprintPure)
	ARedoReplayDriver* GetReplayDriver() const { return ReplayDriver; }
	
	// Get paddle position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetPaddlePosition(int32 index) const;
	// Get ball position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetBallPosition(int32 index) const;
	
	TRingBuffer<GekkoGame::Input> P1InputBuffer;
	TRingBuffer<GekkoGame::Input> P2InputBuffer;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DisconnectLevel;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ARedoReplayDriver> ReplayDriverClass;
	UPROPERTY(BlueprintReadWrite)
	bool bGamePaused = false;
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
	ARedoReplayDriver* ReplayDriver = nullptr;
	
};
