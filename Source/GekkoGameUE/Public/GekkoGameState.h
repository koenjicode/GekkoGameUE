// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "game.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoGameState.generated.h"

/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AGekkoGameState();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	
	void InitGame();
	
	void Update();
	UFUNCTION(BlueprintImplementableEvent)
	void OnUnrealDraw();
	
	// input gathering
	GekkoGame::Input PollInput(int32 ControllerIndex) const;
	
	// Get paddle position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetPaddlePosition(int32 index) const;
	// Get ball position in gekko game state.
	UFUNCTION(BlueprintPure)
	FVector GetBallPosition(int32 index) const;
	
	// Forces a Local Play mode that can be controlled by 2 players.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	bool bLocalPlayEnabled;

private:
	float ElapsedTime;

	GekkoGame::Gamestate gs = {};
	
};
