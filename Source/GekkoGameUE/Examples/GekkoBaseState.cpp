// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoBaseState.h"

#include "GekkoNetSubsystem.h"
#include "GekkoPlayerState.h"
#include "GekkoGameUE/GekkoGameLog.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"

void AGekkoBaseState::BeginPlay()
{
	Super::BeginPlay();
	
	GekkoGameInstance = GetWorld()->GetGameInstance<UGekkoGameInstance>();
}

AGekkoPlayerState* AGekkoBaseState::GetOpponentState() const
{
	auto PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}
	auto LocalState = PC->PlayerState;
	if (!LocalState)
	{
		return nullptr;
	}
	for (int i = 0; i < PlayerArray.Num(); i++)
	{
		if (LocalState->GetPlayerId() != PlayerArray[i]->GetPlayerId())
		{
			return Cast<AGekkoPlayerState>(PlayerArray[i]);
		}
	}
	return nullptr;
}

FString AGekkoBaseState::GetOpponentAddress() const
{
	if (GekkoGameInstance->bDirectMode)
	{
		if (GekkoGameInstance->OpponentAddresses.Num() > 0)
		{
			return GekkoGameInstance->OpponentAddresses[0];
		}
	}
	
	return GetOpponentState()->GetUniqueId().ToString();
}

FString AGekkoBaseState::GetHostAddress() const
{
	auto PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->HasAuthority())
	{
		return PC->PlayerState->GetUniqueId().ToString();
	}
	
	if (PlayerArray.Num() > 0)
	{
		for (int i = 0; i < PlayerArray.Num(); i++)
		{
			if (PC->PlayerState->GetUniqueId().ToString() != PlayerArray[i]->GetUniqueId().ToString())
			{
				return PlayerArray[i]->GetUniqueId().ToString();
			}
		}
	}
	
	return "NO_HOST";
}

void AGekkoBaseState::StartGekkoSession(uint8 InIndex)
{
	UGekkoNetSubsystem* GekkoNet = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();

	if (!GekkoNet)
	{
		return;
	}
	
	GekkoNet->SetSimulationHost(this);
	GekkoNet->bUseAsioTransport = GekkoGameInstance->bUsingAsio;
	GekkoNet->bUseDirectAdapterIfAvailable = GekkoGameInstance->bDirectMode;

	if (GekkoNet->bUseDirectAdapterIfAvailable)
	{
		GekkoNet->SetLocalPort(GekkoGameInstance->LocalPort);
	}
	
	auto& HostConfig = GekkoGameInstance->HostConfig;
	bool& bIsSpectator = GekkoGameInstance->bIsSpectating;
	
	GekkoNet->StartSession(HostConfig, bIsSpectator);
	GekkoNet->SetRunahead(GekkoGameInstance->RunaheadAmount);
	
	FString RemoteAddress;
	if (!bIsSpectator)
	{
		RemoteAddress = GetOpponentAddress();
	}
	else
	{
		RemoteAddress = GetHostAddress();
	}
	
	if (InIndex == 0)
	{
		NetLocalPlayerID = GekkoNet->AddActor();
		GekkoNet->AddActor(EGekkoPlayerType::RemotePlayer, RemoteAddress);
	}
	else
	{
		GekkoNet->AddActor(EGekkoPlayerType::RemotePlayer, RemoteAddress);
		NetLocalPlayerID = GekkoNet->AddActor();
	}
	
	GekkoNet->SetLocalDelay(GekkoGameInstance->LocalDelayAmount, NetLocalPlayerID, false);
	
	GekkoNet->OnPlayerDisconnected.AddUniqueDynamic(this, &AGekkoBaseState::PlayerDisconnected);
	
	bGekkoSessionStarted = true;
	
	UE_LOG(LogGekkoGame, Log, TEXT("Started session as Player %d"), InIndex + 1);
}

void AGekkoBaseState::GekkoAdvance(GekkoGameEvent* Event)
{
}

void AGekkoBaseState::GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData)
{
}

void AGekkoBaseState::GekkoLoad(GekkoGameEvent* Event)
{
}

void AGekkoBaseState::GekkoSave(GekkoGameEvent* Event)
{
}
