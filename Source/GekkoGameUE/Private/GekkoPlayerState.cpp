// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoPlayerState.h"

#include "Net/UnrealNetwork.h"

void AGekkoPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AGekkoPlayerState, GekkoPlayerIndex);
}
