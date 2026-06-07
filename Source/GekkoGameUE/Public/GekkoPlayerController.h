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
	UFUNCTION(Server, Reliable)
	void Server_ClientReady(int32 PlayerId);
	UFUNCTION(Client, Reliable)
	void Client_StartGekkoSession();
	
	UPROPERTY(BlueprintReadOnly)
	AGekkoGameState* GekkoGameState;
	UPROPERTY(BlueprintReadOnly)
	bool bClientReady;
};
