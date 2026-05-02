// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoNetSubsystem.h"
#include "Kismet/GameplayStatics.h"

constexpr int TARGET_FPS = 60;
constexpr float ONE_FRAME = 1.0f / TARGET_FPS;

AGekkoGameState::AGekkoGameState()
{
	PrimaryActorTick.bCanEverTick =	true;
	bReplicates = false;
	
	bLocalPlayEnabled = false;
}

void AGekkoGameState::BeginPlay()
{
	Super::BeginPlay();
	InitGame();
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

void AGekkoGameState::InitGame()
{
	if (bLocalPlayEnabled)
	{
		gs.Init(2);
		return;
	}
	
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

	if (UGekkoNetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>())
	{
		Subsystem->SetSessionConfig(config);
		Subsystem->CreateSession(hosts[0]);
	}
	
	gs.Init(config.GetNumberOfPlayers());
}

void AGekkoGameState::Update()
{
	if (bLocalPlayEnabled)
	{
		GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
		Inputs[0] = PollInput(0);
		Inputs[1] = PollInput(1);
		
		gs.Update(Inputs);
		OnUnrealDraw();
		return;
	}
	
	UGekkoNetSubsystem* SS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	gekko_network_poll(SS->Session); 
	
	auto local_input = PollInput(0);
	for (int i = 0; i < SS->Config.Players.Num(); ++i)
	{
		if (SS->IsLocalPlayer(i))
		{
			SS->AddLocalInput(i, &local_input);
		}
	}
	
	int count = 0;
	GekkoSessionEvent** events = gekko_session_events(SS->Session, &count);
	for (int i = 0; i < count; i++) {
		GekkoSessionEvent* event = events[i];
		switch (event->type) {
		case GekkoDesyncDetected:
			auto desync = event->data.desynced;
			UE_LOG(LogTemp, Error, TEXT("DESYNC!!! f:%d, rh:%d, lc:%u, rc:%u"),
				desync.frame, desync.remote_handle,
				desync.local_checksum, desync.remote_checksum);
			break;

		case GekkoPlayerConnected:
			auto connect = event->data.connected;
			UE_LOG(LogTemp, Log, TEXT("Player %d connected"), connect.handle);
			break;

		case GekkoPlayerDisconnected:
			auto disconnect = event->data.disconnected;
			UE_LOG(LogTemp, Warning, TEXT("Player %d disconnected"), disconnect.handle);
			break;

		case GekkoPlayerSyncing:
			auto sync = event->data.syncing;
			UE_LOG(LogTemp, Log, TEXT("Player %d is connecting %d/%d"), 
				sync.handle, sync.current, sync.max);
			break;
		}
	}

	count = 0;
	GekkoGameEvent** updates = gekko_update_session(SS->Session, &count);
	for (int i = 0; i < count; i++) {
		GekkoGameEvent* event = updates[i];
		switch (event->type) {
		case GekkoSaveEvent:
			*event->data.save.state_len = sizeof(gs.state);
			*event->data.save.checksum = FCrc::MemCrc32(&gs.state, sizeof(gs.state), 0);
			memcpy(event->data.save.state, &gs.state, sizeof(gs.state));
			break;

		case GekkoLoadEvent:
			memcpy(&gs.state, event->data.load.state, sizeof(gs.state));
			break;

		case GekkoAdvanceEvent:
			GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
			FString InputLog = FString::Printf(TEXT("f%d,"), event->data.adv.frame);
			for (int32 j = 0; j < SS->Config.GetNumberOfPlayers(); ++j)
			{
				Inputs[j] = ((GekkoGame::Input*)(event->data.adv.inputs))[j];

				InputLog += FString::Printf(
					TEXT(" p%d %d%d%d%d"),
					j,
					Inputs[j].left,
					Inputs[j].right,
					Inputs[j].up,
					Inputs[j].down
				);
			}
			UE_LOG(LogTemp, Verbose, TEXT("%s"), *InputLog);
			gs.Update(Inputs);
			break;
		}
	}
	OnUnrealDraw();
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

FVector AGekkoGameState::GetPaddlePosition(int32 index) const
{
	FVector pos;
	int game_scale = GekkoGame::GAME_SCALE;
	
	pos.X = gs.state.e_pos[index].x / game_scale; 
	pos.Z = gs.state.e_pos[index].y / game_scale;
	
	return pos;
}

FVector AGekkoGameState::GetBallPosition(int32 index) const
{
	FVector pos;
	int32 ball_index = index + 2;
	
	int game_scale = GekkoGame::GAME_SCALE;
	
	pos.X = gs.state.e_pos[ball_index].x / game_scale;
	pos.Z = gs.state.e_pos[ball_index].y / game_scale; 
	
	return pos;
}
