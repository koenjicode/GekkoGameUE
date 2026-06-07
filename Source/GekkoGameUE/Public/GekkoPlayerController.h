// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoNetRPCController.h"
#include "GameFramework/PlayerController.h"
#include "GekkoPlayerController.generated.h"

class AGekkoGameState;
class AGekkoPlayerState;
/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoPlayerController : public AGekkoNetRPCController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void ReadyClient();
	virtual void Tick(float DeltaSeconds) override;
public:
	UFUNCTION(BlueprintImplementableEvent)
	void GekkoControllerCheck();
	virtual void Client_SendGekkoData_Implementation(const TArray<uint8>& Packet) override;
	UFUNCTION(Server, Reliable)
	virtual void Server_SendGekkoDataDirect(const TArray<uint8>& Packet);
	virtual void SendGekkoData(GekkoNetAddress* Addr, const char* Data, int Length) override;
	virtual GekkoNetResult** ReceiveGekkoData(int* Length) override;
	UFUNCTION(Server, Reliable)
	void Server_ClientReady(int32 PlayerId);
	UFUNCTION(Client, Reliable)
	void Client_StartGekkoSession();
	
	UPROPERTY()
	FString OpponentAddress;
	UPROPERTY(BlueprintReadOnly)
	AGekkoGameState* GekkoGameState;
	UPROPERTY(BlueprintReadOnly)
	bool bClientReady;
};
