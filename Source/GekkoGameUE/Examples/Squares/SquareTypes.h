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
	uint8 Up : 1;
	UPROPERTY()
	uint8 Down : 1;
	UPROPERTY()
	uint8 Left : 1;
	UPROPERTY()
	uint8 Right : 1;
};