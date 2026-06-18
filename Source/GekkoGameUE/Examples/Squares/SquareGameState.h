// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SquareTypes.h"
#include "GekkoGameUE/Examples/GekkoBaseState.h"
#include "SquareGameState.generated.h"

class ASquarePawn;
class UGekkoGameInstance;

UCLASS()
class GEKKOGAMEUE_API ASquareGameState : public AGekkoBaseState
{
	GENERATED_BODY()
	
protected:
	
	ASquareGameState();
	
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void UpdateGame();
	UFUNCTION()
	bool IsOffline() const;
	UFUNCTION()
	void UpdateOnline();
	UFUNCTION()
	void UpdateOffline();

	void UpdateView();
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void GekkoAdvance(GekkoGameEvent* Event) override;
	virtual void GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;
	
	void PollInputs(int32 Player);
	
	virtual void AdvanceGameState(FSquareInputs InInputs[4]);
	

public:
	FSquareInputs Inputs[4];
	ASquarePawn* SquareActors[4];
	UPROPERTY(EditDefaultsOnly)
	int32 SquareLimits;
	UPROPERTY()
	bool bPaused;
	UPROPERTY()
	bool bGekkoSessionStarted;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ASquarePawn> SquareActorClass;
	
	UPROPERTY()
	int32 LocalFrame;
	UPROPERTY()
	int32 RemoteFrame;
	
	UFUNCTION(BlueprintPure)
	int32 GetPlayerCount();
	
	UFUNCTION(BlueprintPure)
	ASquarePawn* GetSquareActor(int Index);

private:
	UPROPERTY()
	float ElapsedTime;
	UPROPERTY()
	int32 ElapsedMatchTime;
};
