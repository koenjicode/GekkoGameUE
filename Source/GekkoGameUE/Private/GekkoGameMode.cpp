// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameMode.h"

#include "GekkoGameState.h"

AGekkoGameMode::AGekkoGameMode()
{
	DefaultPawnClass = nullptr;
	GameStateClass = AGekkoGameState::StaticClass();
}
