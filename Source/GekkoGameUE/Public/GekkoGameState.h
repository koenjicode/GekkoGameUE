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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float ElapsedTime;

	GekkoGame::Gamestate gs = {};
	
};
