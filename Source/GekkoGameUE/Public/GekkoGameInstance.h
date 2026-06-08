// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedFriendsGameInstance.h"
#include "Engine/GameInstance.h"
#include "GekkoGameInstance.generated.h"

class URedoReplaySaveData;

UCLASS()
class GEKKOGAMEUE_API UGekkoGameInstance : public UAdvancedFriendsGameInstance
{
	GENERATED_BODY()
	
	public:
	
	// Is the match being played locally.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLocalPlayEnabled = false;
	// Whether recording is enabled. This will start up the Replay Manager in the next match.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowRecording = true;
	// Playback the last replay that was recorded.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPlaybackLastSave = false;
	
	// Whether matches will use Direct Connections or match via Unreal's Online Subsystem.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDirectMode = false;
	// Is using Asio Transport or not.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUsingAsio = false;
	// The amount of delay used as a baseline in the match, this value can be used offline to implement an artificial amount of delay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LocalDelayAmount = 1;
	
	// The current Player ID that is used, in Online Subsystem matches this will not be used in favour of the actual Player ID's themselves.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DirectPlayerId = -1;
	// The local port that will be used by the client
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LocalPort;
	// The address that you need to connect to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> OpponentAddresses;
};
