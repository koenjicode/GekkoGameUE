// Fill out your copyright notice in the Description page of Project Settings.


#include "RedoReplayDriver.h"

#include "RedoReplaySaveData.h"


// Sets default values
ARedoReplayDriver::ARedoReplayDriver()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void ARedoReplayDriver::Init(int32 StoreInputSize, int32 NumPlayers)
{
	bIsRecording = ReplayData == nullptr;
	if (bIsRecording)
	{
		ReplayData = NewObject<URedoReplaySaveData>(this);
		ReplayData->FillHeader(StoreInputSize, NumPlayers);
	}
}

void ARedoReplayDriver::RecordFrame(int32 Frame, void* Inputs) const
{
	ReplayData->SaveInputs(Frame, Inputs);
}

void ARedoReplayDriver::GetInputsForFrame(int32 Frame, void& Inputs)
{
	
}

void ARedoReplayDriver::SetReplayData(URedoReplaySaveData* DataToUse)
{
	ReplayData = DataToUse;
}

