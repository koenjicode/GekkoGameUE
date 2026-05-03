// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"

#include "GekkoGameInstance.h"
#include "GekkoNetSubsystem.h"
#include "Kismet/GameplayStatics.h"

constexpr int TARGET_FPS = 60;
constexpr float ONE_FRAME = 1.0f / TARGET_FPS;

AGekkoGameState::AGekkoGameState()
{
	PrimaryActorTick.bCanEverTick =	true;
	bReplicates = false;
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
	if (!bStateStarted)
	{
		return;
	}
	
	ElapsedTime += DeltaSeconds;
	
	UGekkoNetSubsystem* SS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	const float FramesAhead = SS->GetFramesAhead();
	constexpr float BaseFrame = ONE_FRAME;
	
	const float AheadScale = (FramesAhead > 0.5f) ? 1.016f : 1.0f;
	const float EffectiveFrame = BaseFrame * AheadScale;

	int32 Steps = 0;
	constexpr int32 MaxStepsPerTick = 4;

	while (ElapsedTime >= EffectiveFrame && Steps < MaxStepsPerTick)
	{
		Update();
		ElapsedTime -= BaseFrame;
		++Steps;
	}

	OnUnrealDraw();
}

void AGekkoGameState::InitGame()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	int32 num_players = 2;
	if (GI->bLocalPlayEnabled)
	{
		for (int i = 0; i < num_players; ++i)
		{
			if (i > 0)
			{
				UGameplayStatics::CreatePlayer(GetWorld(), true);
			}
		}
		bStateStarted = true;
		gs.Init(2);
		return;
	}
	
	FGekkoSessionConfig config;
	int player_count = 0;
	for (int i = 0; i < num_players; ++i)
	{
		if (GI->PlayerId == i)
		{
			config.AddPlayer();
			++player_count;
		}
		else
		{
			if (!GI->RemoteAddresses.IsEmpty())
			{
				auto player = GI->RemoteAddresses[0];
				config.AddPlayer(player.Address, player.Port);
				++player_count;
			}
		}
	}

	if (player_count == num_players)
	{
		config.SessionSize.InputSize = sizeof(GekkoGame::Input);
		config.SessionSize.StateSize = sizeof(GekkoGame::Gamestate::state);
	
		UGekkoNetSubsystem* SS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
		SS->SetSessionConfig(config);
		bStateStarted = SS->CreateSession(GI->LocalPort);
		gs.Init(config.GetNumberOfPlayers());
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to start the GekkoNet match."));
}

void AGekkoGameState::Update()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	if (GI->bLocalPlayEnabled)
	{
		GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
		Inputs[0] = PollInput(0);
		Inputs[1] = PollInput(1);
		
		gs.Update(Inputs);
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
	GekkoGame::Input inp = {};

	if (PC)
	{
		const float Deadzone = 0.5f;

		const float LeftX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
		const float LeftY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

		inp.up =
			PC->IsInputKeyDown(EKeys::Up) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up) ||
			LeftY > Deadzone;

		inp.down =
			PC->IsInputKeyDown(EKeys::Down) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down) ||
			LeftY < -Deadzone;

		inp.left =
			PC->IsInputKeyDown(EKeys::Left) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Left) ||
			LeftX < -Deadzone;

		inp.right =
			PC->IsInputKeyDown(EKeys::Right) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Right) ||
			LeftX > Deadzone;
	}

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
	int32 ball_index = index + GekkoGame::MAX_PLAYERS;
	
	int game_scale = GekkoGame::GAME_SCALE;
	
	pos.X = gs.state.e_pos[ball_index].x / game_scale;
	pos.Z = gs.state.e_pos[ball_index].y / game_scale; 
	
	return pos;
}
