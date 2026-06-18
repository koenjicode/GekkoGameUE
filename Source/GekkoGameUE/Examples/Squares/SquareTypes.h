// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SquareTypes.generated.h"

#define MAX_SQUARE_PLAYERS 4

#define SQUAREGAME_FIXED_SCALE 1000

USTRUCT()
struct FSquareInputs
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool Up;
	UPROPERTY()
	bool Down;
	UPROPERTY()
	bool Left;
	UPROPERTY()
	bool Right;
};