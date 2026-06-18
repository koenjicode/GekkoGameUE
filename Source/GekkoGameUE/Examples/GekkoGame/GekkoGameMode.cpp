// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameMode.h"
#include "game.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"

FGekkoConfig AGekkoGameMode::MakeConfig()
{
	auto HostConfig = GekkoGameInstance->HostConfig;
	
	HostConfig.NumPlayers = 2;
	HostConfig.StateSize = sizeof(GekkoGame::Gamestate::state);
	HostConfig.InputSize = sizeof(GekkoGame::Input);
	
	return HostConfig;
}
