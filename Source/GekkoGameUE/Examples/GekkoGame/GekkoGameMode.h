// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GekkoGameMode.generated.h"

class UGekkoGameInstance;
class AGekkoGameState;
/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	
	AGekkoGameMode();
	
protected:
	
	virtual void BeginPlay() override;
	
public:
	UFUNCTION(BlueprintPure)
	bool IsLocalPlay() const;
	UFUNCTION()
	void ReadyPlayer(int32 PlayerId);
	virtual int32 GetNumPlayers() override;
	UFUNCTION()
	virtual bool CanStartMatch();
	UFUNCTION()
	void StartMatch();
	
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY()
	bool bMatchStarted = false;
	UPROPERTY()
	bool bPlayerStateUpdated = false;
	UPROPERTY()
	TArray<int32> PlayersReady;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UGekkoGameInstance> GekkoGameInstance;
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AGekkoGameState> GekkoGameState;
};
