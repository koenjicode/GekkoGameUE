// Fill out your copyright notice in the Description page of Project Settings.


#include "RedoReplaySaveData.h"

void URedoReplaySaveData::FillHeader(int32 InputSize, int32 NumOfPlayers)
{
	InputSizePerPlayer = InputSize;
	PlayerCount = NumOfPlayers;
}

void URedoReplaySaveData::SaveInputs(int32 Frame, void* Inputs)
{
	int32 InputSize = InputSizePerPlayer * PlayerCount;
	int32 WriteOffset = Data.Num();
	
	Data.AddUninitialized(InputSize);
	
	FMemory::Memcpy(Data.GetData() + WriteOffset, Inputs, InputSize);
}

void URedoReplaySaveData::GetInputs(int32 Frame, void& OutInputs)
{
	int32 InputSize = InputSizePerPlayer * PlayerCount;
	FMemory::Memcpy(OutInputs, Data.GetData() + (Frame * InputSize), InputSize);
}
