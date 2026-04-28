// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoNetSubsystem.h"

constexpr int TARGET_FPS = 60;
constexpr float ONE_FRAME = 1.0f / TARGET_FPS;

void AGekkoGameState::BeginPlay()
{
	Super::BeginPlay();
	
	FGekkoSessionConfig config;
	config.AddPlayer();
	config.AddPlayer("127.0.0.1:", 7001);
	
	config.SessionSize.InputSize = sizeof(GekkoGame::Input);
	config.SessionSize.StateSize = sizeof(GekkoGame::Gamestate::state);
	
	UGekkoNetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (Subsystem)
	{
		Subsystem->Create(config, false);
	}
	
	int32 num_players = config.LocalPlayers.Num() + config.RemotePlayers.Num();
	gs.Init(num_players);
}

void AGekkoGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	gs = {};
}

void AGekkoGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	ElapsedTime += DeltaSeconds;
	while (ElapsedTime >= ONE_FRAME)
	{
		ElapsedTime -= ONE_FRAME;
	}
}
