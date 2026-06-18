// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoNetSimulationInterface.h"
#include "GekkoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoBaseState.generated.h"

class UGekkoGameInstance;
/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoBaseState : public AGameStateBase, public IGekkoNetSimulationInterface
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual AGekkoPlayerState* GetOpponentState() const;
	UFUNCTION()
	virtual FString GetOpponentAddress() const;
	UFUNCTION()
	virtual FString GetHostAddress() const;
	
	virtual void StartGekkoSession(uint8 InIndex);
	
	virtual void GekkoAdvance(GekkoGameEvent* Event) override;
	virtual void GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UGekkoGameInstance> GekkoGameInstance;
	UPROPERTY(BlueprintReadOnly)
	int32 NetLocalPlayerID;
	
protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PlayerDisconnected(int32 Handle);
	
	bool bGekkoSessionStarted;
};
