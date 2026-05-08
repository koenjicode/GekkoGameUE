// Fill out your copyright notice in the Description page of Project Settings.


#include "RedoReplayDriver.h"


// Sets default values
ARedoReplayDriver::ARedoReplayDriver()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ARedoReplayDriver::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARedoReplayDriver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

