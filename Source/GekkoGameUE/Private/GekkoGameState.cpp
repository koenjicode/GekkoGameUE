// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoGameInstance.h"
#include "GekkoGameLog.h"
#include "GekkoNetSubsystem.h"
#include "RedoReplayDriver.h"
#include "RedoReplaySaveData.h"
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
	if (GI)
	{
		if (GI->bLocalPlayEnabled || GI->bPlaybackLastSave)
		{
			ReplayDriver = GetWorld()->SpawnActor<ARedoReplayDriver>(ReplayDriverClass);
		}
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
	Gs.Init(num_players);
	if (ReplayDriver)
	{
		bool bReplayFound = false;
		if (GI && GI->bPlaybackLastSave)
		{
			if (const auto Save = ReplayDriver->FindReplay())
			{
				bReplayFound = true;
				ReplayDriver->Init(sizeof(Gs.state), sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, Save);
				int32 SnapshotCount = 0;
				GekkoGame::Input ReplayInputs[GekkoGame::MAX_PLAYERS] = {};
				for (int i = 0; i < ReplayDriver->GetReplayData()->ReplayLengthInFrames; ++i)
				{
					SnapshotCount++;
					ReplayDriver->UpdatePlayback(&ReplayInputs, false);
					Gs.Update(ReplayInputs);
					if (SnapshotCount >= 1)
					{
						ReplayDriver->AddSnapshot(&Gs.state);
						SnapshotCount = 0;
					}
					ReplayDriver->AdvanceLocalFrame();
				}
				ReplayDriver->SetLocalFrame(0);
				Gs.Init(num_players);
			}
		}
		if (!bReplayFound)
		{
			ReplayDriver->Init(sizeof(Gs.state), sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, nullptr);
		}
	}
}

void AGekkoGameState::ShutdownGame()
{
	FMemory::Memzero(&Gs, sizeof(Gs));
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
	if (ReplayDriver && ReplayDriver->GetDriverState() == ERedoReplayMode::Recording)
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

void AGekkoGameState::RewindToSnapshot(int32 InFrame)
{
	if (ReplayDriver)
	{
		if (ReplayDriver->RewindToSnapshot(InFrame, &Gs.state))
		{
			LocalFrame = ReplayDriver->GetReplayFrame();
		}
	}
}

void AGekkoGameState::RewindBackFromCurrentFrame(int32 FramesToRewindBack)
{
	RewindToSnapshot(LocalFrame - FramesToRewindBack);
}

void AGekkoGameState::FastForwardFromCurrentFrame(int32 FramesToFastForward)
{
	RewindToSnapshot(LocalFrame + FramesToFastForward);
}

void AGekkoGameState::HandleBuffer()
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
	// handle online play updates, if not online, handle offline updates afterwards.
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	bool bIsOnlinePlay = !GI->bLocalPlayEnabled;
	bool bIsReplayPlaying = ReplayDriver && 
		ReplayDriver->GetDriverState() == ERedoReplayMode::Playback;
	
	if (bIsOnlinePlay)
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
		return;
	}
	
	// grab inputs from replay system if a replay is active.
	GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
	bool bResumeGame = true;
	if (bIsReplayPlaying)
	{
		if (LocalFrame < ReplayDriver->GetReplayLengthInFrames())
		{
			ReplayDriver->UpdatePlayback(Inputs);
		}
		else
		{
			bResumeGame = false;
		}
	}
	else
	{
		HandleBuffer();
		int32 LocalPlayers = 2;
		for (int j = 0; j < LocalPlayers; j++)
		{
			Inputs[j] = PollInput(j);
		}
	}
	if (bResumeGame)
	{
		AdvanceGameState(Inputs);
	}
}

void AGekkoGameState::AdvanceGameState(GekkoGame::Input Inputs[4], GekkoGameEvent* Event)
{
	bool bIsOnlinePlay = Event != nullptr;
	bool bIsRecordingMatch = ReplayDriver && 
		ReplayDriver->GetDriverState() == ERedoReplayMode::Recording;
	Gs.Update(Inputs);
	if (bIsRecordingMatch)
	{
		bool bIsRollingBack = false;
		if (bIsOnlinePlay)
		{
			if (Event->data.adv.rolling_back)
			{
				ReplayDriver->SetLocalFrame(Event->data.adv.frame);
				bIsRollingBack = true;
			}
		}
		ReplayDriver->UpdateRecording(Inputs, !bIsRollingBack);
	}
	++LocalFrame;
	if (bIsOnlinePlay)
	{
		++RemoteFrame;
	}
}

void AGekkoGameState::GekkoGetLocalInputs(void* OutInputData)
{
	GekkoGame::Input local_input = PollLatestInput(0);
	FMemory::Memcpy(OutInputData, &local_input, sizeof(GekkoGame::Input));
}

void AGekkoGameState::GekkoLoad(GekkoGameEvent* Event)
{
	FMemory::Memcpy(&Gs.state, Event->data.load.state, sizeof(Gs.state));
}

void AGekkoGameState::GekkoSave(GekkoGameEvent* Event)
{
	*Event->data.save.state_len = sizeof(Gs.state);
	FMemory::Memcpy(Event->data.save.state, &Gs.state, sizeof(Gs.state));
	
	const uint32 Checksum = FCrc::MemCrc32(&Gs.state, sizeof(Gs.state));
	*Event->data.save.checksum = Checksum;
}

void AGekkoGameState::GekkoAdvance(GekkoGameEvent* Event, bool Render)
{
	UE_LOG(LogGekkoGame, Log, TEXT("f%d,"), Event->data.adv.frame);
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	GekkoGame::Input inputs[GekkoGame::MAX_PLAYERS] = {};
	for (int j = 0; j < GNS->GetNumOfPlayers(); j++) 
	{
		inputs[j] = ((GekkoGame::Input*)(Event->data.adv.inputs))[j];
		UE_LOG(LogGekkoGame, Log, TEXT(" p%d %d%d%d%d"), 
			   j, 
			   inputs[j].left, 
			   inputs[j].right, 
			   inputs[j].up, 
			   inputs[j].down);
	}
	RemoteFrame = Event->data.adv.frame;
	AdvanceGameState(inputs, Event);
}

void AGekkoGameState::GekkoDisconnect(GekkoSessionEvent* Event)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DisconnectLevel, true);
}

FVector AGekkoGameState::GetPaddlePosition(int32 index) const
{
	FVector pos;
	int game_scale = GekkoGame::GAME_SCALE;
	
	pos.X = Gs.state.e_pos[index].x / game_scale; 
	pos.Z = Gs.state.e_pos[index].y / game_scale;
	
	return pos;
}

FVector AGekkoGameState::GetBallPosition(int32 index) const
{
	FVector pos;
	int32 ball_index = index + GekkoGame::MAX_PLAYERS;
	
	int game_scale = GekkoGame::GAME_SCALE;
	
	pos.X = Gs.state.e_pos[ball_index].x / game_scale;
	pos.Z = Gs.state.e_pos[ball_index].y / game_scale; 
	
	return pos;
}
