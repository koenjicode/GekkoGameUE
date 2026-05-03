// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GekkoGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FGekkoRemoteAddress
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Address;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Port;
	
};

UCLASS()
class GEKKOGAMEUE_API UGekkoGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLocalPlayEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGekkoRemoteAddress> RemoteAddresses;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LocalPort = 7000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerId = -1;
};
