// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoGameInstance.h"
#include "GekkoGameLog.h"
#include "GekkoNetSubsystem.h"
#include "RedoReplayDriver.h"
#include "Kismet/GameplayStatics.h"

#define TARGET_FRAMERATE 60
#define ONE_FRAME (1.0f / TARGET_FRAMERATE)

#define MAX_UPDATES_PER_TICK 30
#define MAX_LOCAL_DELAY_FRAMES 9


AGekkoGameState::AGekkoGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	ReplayDriverClass = ARedoReplayDriver::StaticClass();
}

void AGekkoGameState::BeginPlay()
{
	Super::BeginPlay();
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	ReplayDriver = GetWorld()->SpawnActor<ARedoReplayDriver>(ReplayDriverClass);
	if (ReplayDriver)
	{
		URedoReplaySaveData* Save;
		if (GI && GI->bPlaybackLastSave)
		{
			Save = ReplayDriver->FindReplay();
		}
		else
		{
			Save = nullptr;
		}
		ReplayDriver->Init(sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, Save);
	}
	Init();
}

void AGekkoGameState::Init()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	int32 num_players = 2;
	// make a additional player controller than can accept inputs
	if (GI->bLocalPlayEnabled)
	{
		for (int i = 0; i < num_players; ++i)
		{
			if (i > 0)
			{
				UGameplayStatics::CreatePlayer(GetWorld(), true);
			}
		}
	}
	P1InputBuffer.Reserve(MAX_LOCAL_DELAY_FRAMES);
	P2InputBuffer.Reserve(MAX_LOCAL_DELAY_FRAMES);
	gs.Init(num_players);
}

void AGekkoGameState::ShutdownGame()
{
	FMemory::Memzero(&gs, sizeof(gs));
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (GNS->IsSessionActive() && GNS->GetSessionState() != EGekkoSessionState::Exiting)
	{
		GNS->ShutdownGekko();
	}
}

void AGekkoGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ShutdownGame();
	if (IsRecordingMatch())
	{
		ReplayDriver->SaveReplay();
	}
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

void AGekkoGameState::HandleBufferedInput()
{
	int32 num_players = 2;
	for (int i = 0; i < num_players; ++i)
	{
		TRingBuffer<GekkoGame::Input> &InputBuffer = i == 0 ? P1InputBuffer : P2InputBuffer;
		if (InputBuffer.Num() > MAX_LOCAL_DELAY_FRAMES)
		{
			InputBuffer.PopFront();
		}
		// added latest input at the end of the buffer.
		InputBuffer.Add(PollLatestInput(i));
	}
}

GekkoGame::Input AGekkoGameState::PollLatestInput(int32 PlayerIndex) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, PlayerIndex);
	GekkoGame::Input inp = {};

	if (PC)
	{
		const float Deadzone = 0.5f;

		const float LeftX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
		const float LeftY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

		inp.up =
			PC->IsInputKeyDown(EKeys::W) ||
			PC->IsInputKeyDown(EKeys::Up) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up) ||
			LeftY > Deadzone;

		inp.down =
			PC->IsInputKeyDown(EKeys::S) ||
			PC->IsInputKeyDown(EKeys::Down) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down) ||
			LeftY < -Deadzone;

		inp.left =
			PC->IsInputKeyDown(EKeys::A) ||
			PC->IsInputKeyDown(EKeys::Left) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Left) ||
			LeftX < -Deadzone;

		inp.right =
			PC->IsInputKeyDown(EKeys::D) ||
			PC->IsInputKeyDown(EKeys::Right) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Right) ||
			LeftX > Deadzone;
	}

	return inp;
}

GekkoGame::Input AGekkoGameState::PollInput(int32 PlayerIndex) const
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	int32 LocalDelay = GI ? GI->LocalDelayAmount : 0;
	
	auto InputBuffer = PlayerIndex == 0 ? P1InputBuffer : P2InputBuffer;

	if (LocalDelay == 0)
	{
		return PollLatestInput(PlayerIndex);
	}
	const int32 i = FMath::Max(MAX_LOCAL_DELAY_FRAMES - LocalDelay);
	return InputBuffer.IsValidIndex(i) ? InputBuffer[i] : GekkoGame::Input();
}

void AGekkoGameState::UpdateGame()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	if (GI->bLocalPlayEnabled)
	{
		int32 local_players = 2;
		GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
		if (ReplayDriver)
		{
			ReplayDriver->AdvanceLocalFrame();
		}
		if (IsPlayingBackReplay())
		{
			ReplayDriver->ReciteInputs(&Inputs);
		}
		else
		{
			HandleBufferedInput();
			for (int j = 0; j < local_players; j++)
			{
				Inputs[j] = PollInput(j);
			}
			if (IsRecordingMatch())
			{
				ReplayDriver->RecordInputs(Inputs);
			}
		}
		gs.Update(Inputs);
	}
	else
	{
		UGekkoNetSubsystem* GNS = GI->GetSubsystem<UGekkoNetSubsystem>();
		if (!GNS->IsSessionActive())
		{
			FGekkoSessionConfig SessionConfig {
			2,
			0,
			10,
			0,
			sizeof(GekkoGame::Input),
			sizeof(GekkoGame::Gamestate::state),
			false,
			0};
			GNS->SetPlayerID(GI->PlayerId);
			GNS->SetLocalDelay(GI->LocalDelayAmount);
			GNS->StartGekko(SessionConfig, this);
		}
		GNS->UpdateGekko();
	}
}

bool AGekkoGameState::IsRecordingMatch() const
{
	if (ReplayDriver != nullptr)
	{
		return ReplayDriver->IsRecording();
	}
	return false;
}

bool AGekkoGameState::IsPlayingBackReplay() const
{
	if (ReplayDriver != nullptr)
	{
		return ReplayDriver->GetDriverState() == ERedoDriverType::Playback;
	}
	return false;
}

void AGekkoGameState::GekkoGetLocalInputs(void* OutInputData)
{
	GekkoGame::Input local_input = PollLatestInput(0);
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
	
	const uint32 Checksum = FCrc::MemCrc32(&gs.state, sizeof(gs.state));
	*Event->data.save.checksum = Checksum;
}

void AGekkoGameState::GekkoAdvance(GekkoGameEvent* Event, bool Render)
{
	UE_LOG(LogGekkoGame, Log, TEXT("f%d,"), Event->data.adv.frame);

	UGekkoNetSubsystem* gekko_system = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	GekkoGame::Input inputs[GekkoGame::MAX_PLAYERS] = {};
	for (int j = 0; j < gekko_system->GetNumOfPlayers(); j++) 
	{
		inputs[j] = ((GekkoGame::Input*)(Event->data.adv.inputs))[j];
		UE_LOG(LogGekkoGame, Log, TEXT(" p%d %d%d%d%d"), 
			   j, 
			   inputs[j].left, 
			   inputs[j].right, 
			   inputs[j].up, 
			   inputs[j].down);
	}
	if (IsRecordingMatch())
	{
		ReplayDriver->SetLocalFrame(Event->data.adv.frame);
		ReplayDriver->RecordInputs(Event->data.adv.inputs);
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
