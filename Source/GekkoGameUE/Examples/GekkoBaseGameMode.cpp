// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoBaseGameMode.h"
#include "GekkoPlayerController.h"
#include "GekkoGame/GekkoGameState.h"
#include "GekkoGameUE/GekkoGameLog.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"
#include "Kismet/GameplayStatics.h"

AGekkoBaseGameMode::AGekkoBaseGameMode()
{
	DefaultPawnClass = nullptr;
	GameStateClass = AGekkoGameState::StaticClass();
	
	SetActorTickInterval(1.f);
}

void AGekkoBaseGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	GekkoGameInstance = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	GekkoGameState = Cast<AGekkoGameState>(GetWorld()->GetGameState());
}

bool AGekkoBaseGameMode::IsLocalPlay() const
{
	return GekkoGameState->IsOffline();
}

void AGekkoBaseGameMode::ReadyPlayer(int32 PlayerId)
{
	PlayersReady.Add(PlayerId);
}

int32 AGekkoBaseGameMode::GetNumPlayers()
{
	return PlayersReady.Num();
}

bool AGekkoBaseGameMode::CanStartMatch()
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
	if (GetNumPlayers() == RequiredNumberOfPlayers + GekkoGameInstance->HostConfig.MaxSpectators)
	{
		UE_LOG(LogGekkoGame, Log, TEXT("Starting a networked match via RPC."));
		return true;
	}
	
	return false;
}

FGekkoConfig AGekkoBaseGameMode::MakeConfig()
{
	return GekkoGameInstance->HostConfig;
}

bool AGekkoBaseGameMode::HasMatchStarted() const
{
	bool bCanStart;
	
	if (IsNetMode(NM_Standalone))
	{
		bCanStart = bMatchStarted;
	}
	else
	{
		bCanStart = bMatchStarted && bGekkoSessionStarted;
	}
	
	if (bCanStart)
	{
		return Super::HasMatchStarted();
	}
	
	return false;
}

void AGekkoBaseGameMode::StartMatch()
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
		GekkoGameInstance->HostConfig = MakeConfig();
		
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

void AGekkoBaseGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!GekkoGameInstance || !GekkoGameState)
	{
		return;
	}
	
	StartMatch();
}
