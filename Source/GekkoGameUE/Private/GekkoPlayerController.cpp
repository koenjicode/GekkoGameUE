#include "GekkoPlayerController.h"
#include "GekkoGameMode.h"
#include "GekkoGameState.h"
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
