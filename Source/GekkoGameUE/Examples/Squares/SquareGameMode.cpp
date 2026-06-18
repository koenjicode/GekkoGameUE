// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareGameMode.h"
#include "SquareGameState.h"
#include "SquareTypes.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"

ASquareGameMode::ASquareGameMode(const FObjectInitializer& ObjectInitializer)
{
	DefaultPawnClass = nullptr;
	GameStateClass = ASquareGameState::StaticClass();
	PlayerStateClass = AGekkoPlayerState::StaticClass();
}

FGekkoConfig ASquareGameMode::MakeConfig()
{
	auto HostConfig = GekkoGameInstance->HostConfig;
	
	HostConfig.NumPlayers = 2;
	HostConfig.StateSize = 0;
	HostConfig.InputSize = sizeof(FSquareInputs);
	
	return HostConfig;
}
