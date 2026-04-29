// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoNetSubsystem.h"
#include "Kismet/GameplayStatics.h"

constexpr int TARGET_FPS = 60;
constexpr float ONE_FRAME = 1.0f / TARGET_FPS;

void AGekkoGameState::BeginPlay()
{
	Super::BeginPlay();
	
	// TODO: very rough and temporary, needs some sort of way to set the player id.
	int32 player_id = 0;
	int hosts[2] = {};

	if (player_id == 0)
	{
		hosts[0] = 7000;
		hosts[1] = 7001;
	}
	else
	{
		hosts[0] = 7001;
		hosts[1] = 7002;
	}
	
	FGekkoSessionConfig config;
	config.AddPlayer();
	config.AddPlayer("127.0.0.1:", hosts[1]);
	
	config.SessionSize.InputSize = sizeof(GekkoGame::Input);
	config.SessionSize.StateSize = sizeof(GekkoGame::Gamestate::state);
	
	UGekkoNetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (Subsystem)
	{
		Subsystem->SetSessionConfig(config);
		Subsystem->CreateSession(hosts[0]);
	}
	
	gs.Init(config.GetNumberOfPlayers());
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
		Update();
		ElapsedTime -= ONE_FRAME;
	}
}

void AGekkoGameState::Update()
{
	UGekkoNetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	
	auto local_input = PollInput(0);
	for (int i = 0; i < Subsystem->Config.LocalPlayers.Num(); i++) {
		gekko_add_local_input(Subsystem->Session, Subsystem->Config.LocalPlayers[i].PlayerIndex, &local_input);
	}
}

GekkoGame::Input AGekkoGameState::PollInput(int32 ControllerIndex) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, ControllerIndex);
	
	GekkoGame::Input inp;
	inp.up = PC->IsInputKeyDown(EKeys::Up);
	inp.down = PC->IsInputKeyDown(EKeys::Down);
	inp.left = PC->IsInputKeyDown(EKeys::Left);
	inp.right = PC->IsInputKeyDown(EKeys::Right);
	return inp;
}
