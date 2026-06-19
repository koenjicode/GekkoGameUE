// Fill out your copyright notice in the Description page of Project Settings.


#include "SquarePawn.h"
#include "SquareTypes.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"

// Sets default values
ASquarePawn::ASquarePawn()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ASquarePawn::FixedTick(FSquareInputs Inputs)
{
	if (!Inputs.Left && !Inputs.Right && !Inputs.Up && !Inputs.Down)
	{
		return;
	}
	
	int32 ForwardVal = Inputs.Right - Inputs.Left;
	int32 UpVal = Inputs.Up - Inputs.Down;
	
	RealX += FMath::Min(UpVal * MoveSpeed * SQUAREGAME_FIXED_SCALE, INT32_MAX);
	RealY += FMath::Min(ForwardVal * MoveSpeed * SQUAREGAME_FIXED_SCALE, INT32_MAX);
}

void ASquarePawn::ViewTick()
{
	const FVector Loc = FVector(RealX / SQUAREGAME_FIXED_SCALE, RealY / SQUAREGAME_FIXED_SCALE, 0);
	SetActorLocation(Loc, true);
}

TArray<uint8> ASquarePawn::SaveForRollback()
{
	TArray<uint8> SaveData;
	FObjectWriter Writer(SaveData);
	Writer.ArIsSaveGame = true;
	GetClass()->SerializeBin(Writer, this);
	return SaveData;
}

void ASquarePawn::LoadForRollback(const TArray<uint8>& InBytes)
{
	if (InBytes.Num() <= 1) return;
	FObjectReader Reader(InBytes);
	Reader.ArIsSaveGame = true;
	GetClass()->SerializeBin(Reader, this);
}
