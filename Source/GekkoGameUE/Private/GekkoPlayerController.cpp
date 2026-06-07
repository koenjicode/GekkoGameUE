#include "GekkoPlayerController.h"

#include "GekkoGameLog.h"
#include "GekkoGameMode.h"
#include "GekkoGameState.h"
#include "GekkoPlayerState.h"
#include "GameFramework/PlayerState.h"

void AGekkoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	GekkoGameState = Cast<AGekkoGameState>(GetWorld()->GetGameState());
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
	UE_LOG(LogGekkoGame, Log, TEXT("Packets received from Host."));
}

void AGekkoPlayerController::Server_SendGekkoDataDirect_Implementation(const TArray<uint8>& Packet)
{
	const auto PC = Cast<AGekkoPlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (!PC)
	{
		return;
	}
	
	PC->GekkoMessages.Enqueue(Packet);
	UE_LOG(LogGekkoGame, Log, TEXT("Packets received from Client."));
}

void AGekkoPlayerController::SendGekkoData(GekkoNetAddress* Addr, const char* Data, int Length)
{
	if (OpponentAddress.IsEmpty())
	{
		OpponentAddress = FString(UTF8_TO_TCHAR((const char*)Addr->data));
	}
	
	TArray<uint8> Packet;
	Packet.Append((uint8*)Data, Length);
	
	if (!IsNetMode(NM_Client))
	{
		if (auto PC = Cast<AGekkoPlayerController>(GekkoGameState->GetOpponentState()->GetPlayerController()))
		{
			PC->Client_SendGekkoData(Packet);
			UE_LOG(LogGekkoGame, Log, TEXT("Sending packets to client."));
		}
	}
	else
	{
		Server_SendGekkoDataDirect(Packet);
		UE_LOG(LogGekkoGame, Log, TEXT("Sending packets to server."));
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
		
		auto Convert = StringCast<UTF8CHAR>(*OpponentAddress);
		Res->addr.size = Convert.Length();
		Res->addr.data = FMemory::Malloc(Res->addr.size);
		FMemory::Memcpy(Res->addr.data, Convert.Get(), Res->addr.size);

		Res->data_len = Packet.Num();
		Res->data = FMemory::Malloc(Res->data_len);
		
		FMemory::Memcpy(Res->data, Packet.GetData(), Res->data_len);
		
		GekkoResults.Add(Res);
	}

	*Length = GekkoResults.Num();
	
	return GekkoResults.GetData();
}

void AGekkoPlayerController::Server_ClientReady_Implementation(int32 PlayerId)
{
	Cast<AGekkoGameMode>(GetWorld()->GetAuthGameMode())->ReadyPlayer(PlayerId);
}

void AGekkoPlayerController::Client_StartGekkoSession_Implementation()
{
	int32 PID = GetNetMode() == NM_Client ? 1 : 0;
	if (!GekkoGameState)
	{
		return;
	}
	GekkoGameState->StartGekkoSession(PID);
}
