// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SquareTypes.h"
#include "GameFramework/Actor.h"
#include "SquarePawn.generated.h"

UCLASS()
class GEKKOGAMEUE_API ASquarePawn : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASquarePawn();
	
	virtual void FixedTick(FSquareInputs Inputs);
	virtual void ViewTick();
	
	virtual TArray<uint8> SaveForRollback();
	virtual void LoadForRollback(const TArray<uint8>& InBytes);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetDefaultColor(FLinearColor Color);
	
	UPROPERTY(EditDefaultsOnly)
	int32 MoveSpeed = 5;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame)
	int32 RealX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame)
	int32 RealY;
};
