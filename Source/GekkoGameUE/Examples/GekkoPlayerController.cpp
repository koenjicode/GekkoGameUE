#include "GekkoPlayerController.h"
#include "GekkoBaseGameMode.h"
#include "GameFramework/PlayerState.h"
#include "GekkoGame/GekkoGameState.h"
#include "GekkoGameUE/GekkoGameLog.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"

void AGekkoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	GekkoGameState = Cast<AGekkoGameState>(GetWorld()->GetGameState());
	GekkoGameInstance = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
}

void AGekkoPlayerController::ReadyClient()
{
	if (!PlayerState || bClientReady)
	{
		return;
	}
	
	if (PlayerState->GetPlayerId() == 0)
	{
		return;
	}
	
	Server_ClientReady(PlayerState->GetPlayerId());
	bClientReady = true;
}

void AGekkoPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsNetMode(NM_Standalone) && IsLocalController())
	{
		ReadyClient();
	}
}

void AGekkoPlayerController::Client_SendGekkoData_Implementation(const TArray<uint8>& Packet)
{
	GekkoMessages.Enqueue(Packet);
	UE_LOG(LogGekkoGame, Verbose, TEXT("Packets received from Host."));
}

void AGekkoPlayerController::Server_SendGekkoDataDirect_Implementation(const TArray<uint8>& Packet)
{
	const auto PC = Cast<AGekkoPlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (!PC)
	{
		return;
	}
	
	PC->GekkoMessages.Enqueue(Packet);
	UE_LOG(LogGekkoGame, Verbose, TEXT("Packets received from Client."));
}

bool AGekkoPlayerController::IsClient() const
{
	return IsNetMode(NM_Client);
}

void AGekkoPlayerController::SendGekkoData(GekkoNetAddress* Addr, const char* Data, int Length)
{
	if (OpponentAddressBuffer.IsEmpty())
	{
		OpponentAddressBuffer.AddUninitialized(Addr->size);
		FMemory::Memcpy(OpponentAddressBuffer.GetData(), Addr->data, Addr->size);
	}
	
	TArray<uint8> Packet;
	Packet.Append((uint8*)Data, Length);
	
	if (!IsClient())
	{
		if (!GekkoGameState->GetOpponentState())
		{
			return;
		}
		if (auto PC = Cast<AGekkoPlayerController>(GekkoGameState->GetOpponentState()->GetPlayerController()))
		{
			PC->Client_SendGekkoData(Packet);
			UE_LOG(LogGekkoGame, VeryVerbose, TEXT("Sending packets to client."));
		}
	}
	else
	{
		Server_SendGekkoDataDirect(Packet);
		UE_LOG(LogGekkoGame, VeryVerbose, TEXT("Sending packets to server."));
	}
}

GekkoNetResult** AGekkoPlayerController::ReceiveGekkoData(int* Length)
{
	GekkoResults.Reset();
	TArray<uint8> Packet;

	while (GekkoMessages.Dequeue(Packet))
	{
		GekkoNetResult* Res = static_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));
		FMemory::Memzero(Res, sizeof(GekkoNetResult));
		
		Res->addr.data = FMemory::Malloc(OpponentAddressBuffer.Num());
		Res->addr.size = OpponentAddressBuffer.Num();
		FMemory::Memcpy(Res->addr.data, OpponentAddressBuffer.GetData(), Res->addr.size);

		Res->data_len = Packet.Num();
		Res->data = FMemory::Malloc(Res->data_len);
		
		FMemory::Memcpy(Res->data, Packet.GetData(), Res->data_len);
		
		GekkoResults.Add(Res);
	}

	*Length = GekkoResults.Num();
	
	return GekkoResults.GetData();
}

FString AGekkoPlayerController::GetOpponentAddressAsString()
{
	if (OpponentAddressBuffer.IsEmpty())
	{
		return "NONE";
	}
	
	return FString(UTF8_TO_TCHAR(OpponentAddressBuffer.GetData()));
}

void AGekkoPlayerController::Server_ClientReady_Implementation(int32 PlayerId)
{
	Cast<AGekkoBaseGameMode>(GetWorld()->GetAuthGameMode())->ReadyPlayer(PlayerId);
}

void AGekkoPlayerController::Client_StartGekkoSession_Implementation(FGekkoConfig HostConfig)
{
	if (!GekkoGameState)
	{
		return;
	}
	if (!GekkoGameInstance)
	{
		return;
	}
	GekkoGameInstance->HostConfig = HostConfig;
	int32 PID = GetNetMode() == NM_Client ? 1 : 0;
	GekkoGameState->StartGekkoSession(PID);
}
