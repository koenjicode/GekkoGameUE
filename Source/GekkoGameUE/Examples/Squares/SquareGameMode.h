// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoGameUE/Examples/GekkoBaseGameMode.h"
#include "SquareGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API ASquareGameMode : public AGekkoBaseGameMode
{
	GENERATED_BODY()
	
public:
	
	virtual FGekkoConfig MakeConfig() override;
};
