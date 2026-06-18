// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GekkoPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	uint8 GekkoPlayerIndex = 255;
};
