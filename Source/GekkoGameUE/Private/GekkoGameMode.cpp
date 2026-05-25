// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameMode.h"

#include "GekkoGameState.h"
#include "GameFramework/PlayerState.h"

AGekkoGameMode::AGekkoGameMode()
{
	DefaultPawnClass = nullptr;
	GameStateClass = AGekkoGameState::StaticClass();
}

void AGekkoGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	if (GetNumPlayers() == 2)
	{
		AGekkoGameState* GS = GetGameState<AGekkoGameState>();
		
		FString HostAddr = GS->PlayerArray[0]->GetUniqueId().ToString();
		FString ClientAddr = GS->PlayerArray[1]->GetUniqueId().ToString();
	}
}
