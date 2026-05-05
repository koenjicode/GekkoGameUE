// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoGameInstance.h"
#include "GekkoGameLog.h"
#include "GekkoNetSubsystem.h"
#include "Kismet/GameplayStatics.h"

#define TARGET_FRAMERATE 60
#define ONE_FRAME (1.0f / TARGET_FRAMERATE)

#define MAX_UPDATES_PER_TICK 30


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

void AGekkoGameState::InitGame()
{
	UGekkoGameInstance* game_instance = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	int32 num_players = 2;
	// make a additional player controller than can accept inputs
	if (game_instance->bLocalPlayEnabled)
	{
		for (int i = 0; i < num_players; ++i)
		{
			if (i > 0)
			{
				UGameplayStatics::CreatePlayer(GetWorld(), true);
			}
		}
	}
	gs.Init(num_players);
}

void AGekkoGameState::ShutdownGame()
{
	FMemory::Memzero(&gs, sizeof(gs));
	UGekkoNetSubsystem* gekko_system = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (gekko_system->Session != nullptr && gekko_system->SessionState != EGekkoSessionState::Exiting)
	{
		gekko_system->ShutdownGekko();
	}
}

void AGekkoGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ShutdownGame();
}

void AGekkoGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	while (ElapsedTime >= ONE_FRAME)
	{
		//while elapsed time is greater than one frame...
		UpdateGame();
		ElapsedTime -= ONE_FRAME;
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

void AGekkoGameState::UpdateGame()
{
	UGekkoGameInstance* game_instance = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	if (game_instance->bLocalPlayEnabled)
	{
		int32 local_players = 2;
		GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
		for (int j = 0; j < local_players; j++)
		{
			Inputs[j] = PollInput(j);
		}
		gs.Update(Inputs);
	}
	else
	{
		UGekkoNetSubsystem* gekko_system = game_instance->GetSubsystem<UGekkoNetSubsystem>();
		if (gekko_system->Session == nullptr)
		{
			gekko_system->PlayerNumber = game_instance->PlayerId;
			FGekkoSessionConfig SessionConfig {
			2,
			0,
			10,
			0,
			sizeof(GekkoGame::Input),
			sizeof(GekkoGame::Gamestate::state),
			false,
			0};
			gekko_system->StartGekko(SessionConfig);
		}
		gekko_system->UpdateNetplay();
	}
}

void AGekkoGameState::GekkoGetLocalInputs(void* OutInputData)
{
	GekkoGame::Input local_input = PollInput(0);
	FMemory::Memcpy(OutInputData, &local_input, sizeof(GekkoGame::Input));
}

void AGekkoGameState::GekkoLoad(GekkoGameEvent* Event)
{
	FMemory::Memcpy(&gs.state, Event->data.load.state, sizeof(gs.state));
}

void AGekkoGameState::GekkoSave(GekkoGameEvent* Event)
{
	*Event->data.save.state_len = sizeof(gs.state);
	FMemory::Memcpy(Event->data.save.state, &gs.state, sizeof(gs.state));
}

void AGekkoGameState::GekkoAdvance(GekkoGameEvent* Event, bool Render)
{
	UE_LOG(LogGekkoGame, Log, TEXT("f%d,"), Event->data.adv.frame);

	UGekkoNetSubsystem* gekko_system = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	GekkoGame::Input inputs[GekkoGame::MAX_PLAYERS] = {};
	for (int j = 0; j < gekko_system->NumPlayers; j++) 
	{
		inputs[j] = ((GekkoGame::Input*)(Event->data.adv.inputs))[j];
		UE_LOG(LogGekkoGame, Log, TEXT(" p%d %d%d%d%d"), 
			   j, 
			   inputs[j].left, 
			   inputs[j].right, 
			   inputs[j].up, 
			   inputs[j].down);
	}
	gs.Update(inputs);
}

void AGekkoGameState::GekkoDisconnect(GekkoSessionEvent* Event)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DisconnectLevel, true);
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
