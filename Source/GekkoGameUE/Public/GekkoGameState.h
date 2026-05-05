// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "game.h"
#include "GekkoNetSimulationInterface.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoGameState.generated.h"

/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoGameState : public AGameStateBase, public IGekkoNetSimulationInterface
{
	GENERATED_BODY()
	
public:
	AGekkoGameState();
	
	void InitGame();
	virtual void BeginPlay() override;
	
	void ShutdownGame();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void Tick(float DeltaSeconds) override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnUnrealDraw();
	
	// input gathering
	GekkoGame::Input PollInput(int32 ControllerIndex) const;
	
	void UpdateGame();
	
	virtual void GekkoGetLocalInputs(void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;
	virtual void GekkoAdvance(GekkoGameEvent* Event, bool Render) override;
	virtual void GekkoDisconnect(GekkoSessionEvent* Event) override;
	
	// Get paddle position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetPaddlePosition(int32 index) const;
	// Get ball position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetBallPosition(int32 index) const;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DisconnectLevel;

private:
	float ElapsedTime;
	GekkoGame::Gamestate gs = {};
	
};
