// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameMode.h"
#include "GekkoGameState.h"
#include "GekkoPlayerController.h"
#include "GekkoGameUE/GekkoGameLog.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"
#include "Kismet/GameplayStatics.h"

AGekkoGameMode::AGekkoGameMode()
{
	DefaultPawnClass = nullptr;
	GameStateClass = AGekkoGameState::StaticClass();
	
	SetActorTickInterval(1.f);
}

void AGekkoGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	GekkoGameInstance = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	GekkoGameState = Cast<AGekkoGameState>(GetWorld()->GetGameState());
}

bool AGekkoGameMode::IsLocalPlay() const
{
	return GekkoGameState->IsPlayingOffline();
}

void AGekkoGameMode::ReadyPlayer(int32 PlayerId)
{
	PlayersReady.Add(PlayerId);
}

int32 AGekkoGameMode::GetNumPlayers()
{
	return PlayersReady.Num();
}

bool AGekkoGameMode::CanStartMatch()
{
	// Don't even bother to start if these are not valid.
	if (!GekkoGameInstance || !GekkoGameState)
	{
		return false;
	}
	
	// The match has already started, so we don't need to start it again.
	if (GekkoGameState->bMatchStarted)
	{
		return false;
	}
	
	// If we're running the game Locally or for direct connections, this should be true.
	if (IsLocalPlay())
	{
		UE_LOG(LogGekkoGame, Log, TEXT("Starting a Local Play match."));
		return true;
	}
	
	// If we're running as a server, we're not going to do anything, maybe try and get this to work later?
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	
	// If direct connect is enabled, we will start the match
	if (GekkoGameInstance->bDirectMode)
	{
		UE_LOG(LogGekkoGame, Log, TEXT("Starting a direct networked match."));
		return true;
	}
	
	// If two players are connected in the lobby, we can start the match.
	int32 RequiredNumberOfPlayers = 2;
	if (GetNumPlayers() == RequiredNumberOfPlayers + GekkoGameInstance->HostConfig.MaxSpectators)
	{
		UE_LOG(LogGekkoGame, Log, TEXT("Starting a networked match via RPC."));
		return true;
	}
	
	return false;
}

void AGekkoGameMode::StartMatch()
{
	if (!CanStartMatch())
	{
		return;
	}
	
	if (IsLocalPlay())
	{
		UGameplayStatics::CreatePlayer(GetWorld(), true);
	}
	else
	{
		auto& HostConfig = GekkoGameInstance->HostConfig;
		HostConfig.NumPlayers = 2;
		HostConfig.StateSize = sizeof(GekkoGame::Gamestate::state);
		HostConfig.InputSize = sizeof(GekkoGame::Input);
		
		if (GekkoGameInstance->bDirectMode)
		{
			GekkoGameState->StartGekkoSession(GekkoGameInstance->DirectPlayerId);
		}
		else
		{
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				AGekkoPlayerController* PC = Cast<AGekkoPlayerController>(It->Get());
				PC->Client_StartGekkoSession(GekkoGameInstance->HostConfig);
			}
		}
	}
	
	GekkoGameState->bMatchStarted = true;
}

void AGekkoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!GekkoGameInstance || !GekkoGameState)
	{
		return;
	}
	
	StartMatch();
}
