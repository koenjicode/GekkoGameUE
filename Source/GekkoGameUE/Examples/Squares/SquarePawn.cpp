// Fill out your copyright notice in the Description page of Project Settings.


#include "SquarePawn.h"
#include "SquareTypes.h"

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

