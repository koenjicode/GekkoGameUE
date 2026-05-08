// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GekkoGameInstance.generated.h"

UCLASS()
class GEKKOGAMEUE_API UGekkoGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLocalPlayEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LocalDelayAmount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerId = -1;
};
