// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoNetTypes.h"
#include "GameFramework/GameModeBase.h"
#include "GekkoBaseGameMode.generated.h"

class AGekkoBaseState;
class UGekkoGameInstance;
/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoBaseGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	
	AGekkoBaseGameMode();
	
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
	
	virtual FGekkoConfig MakeConfig();
	
	virtual bool HasMatchStarted() const override;
	
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
	TObjectPtr<AGekkoBaseState> GekkoGameState;
	UPROPERTY(BlueprintReadOnly)
	bool bGekkoSessionStarted;
	UPROPERTY(EditDefaultsOnly)
	int RequiredNumberOfPlayers = 2;
};
