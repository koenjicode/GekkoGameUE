// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoGameState.h"
#include "GekkoGameInstance.h"
#include "GekkoGameLog.h"
#include "GekkoNetSubsystem.h"
#include "RedoReplayManager.h"
#include "RedoReplaySaveData.h"
#include "Kismet/GameplayStatics.h"

#define TARGET_FRAMERATE 60
#define ONE_FRAME (1.0f / TARGET_FRAMERATE)

#define MAX_UPDATES_PER_TICK 30
#define MAX_LOCAL_DELAY_FRAMES 9
#define MAX_NET_STATS_TIME 60


AGekkoGameState::AGekkoGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	ReplayManagerClass = ARedoReplayManager::StaticClass();
}

void AGekkoGameState::BeginPlay()
{
	Super::BeginPlay();
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	if (GI)
	{
		if (GI->bLocalPlayEnabled || GI->bPlaybackLastSave)
		{
			ReplayManager = GetWorld()->SpawnActor<ARedoReplayManager>(ReplayManagerClass);
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
	if (ReplayManager)
	{
		bool bReplayFound = false;
		if (GI && GI->bPlaybackLastSave)
		{
			if (const auto Save = ReplayManager->FindReplay("REPLAY"))
			{
				bReplayFound = true;
				ReplayManager->Init(sizeof(Gs.state), sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, Save);
				int32 SnapshotCount = 0;
				GekkoGame::Input ReplayInputs[GekkoGame::MAX_PLAYERS] = {};
				for (int i = 0; i < ReplayManager->GetReplayData()->ReplayLengthInFrames; ++i)
				{
					SnapshotCount++;
					ReplayManager->RetrieveInputsFromReplay(&ReplayInputs);
					Gs.Update(ReplayInputs);
					if (SnapshotCount >= 1)
					{
						ReplayManager->AddSnapshot(&Gs.state);
						SnapshotCount = 0;
					}
					ReplayManager->AdvanceLocalFrame();
				}
				ReplayManager->SetLocalFrame(0);
				Gs.Init(num_players);
			}
		}
		if (!bReplayFound)
		{
			ReplayManager->Init(sizeof(Gs.state), sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, nullptr);
		}
	}
}

void AGekkoGameState::ShutdownGame()
{
	FMemory::Memzero(&Gs, sizeof(Gs));
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	GNS->EndSession();
}

void AGekkoGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ShutdownGame();
	if (ReplayManager && ReplayManager->GetManagerBehaviour() == ERedoReplayMode::Recording)
	{
		ReplayManager->SaveReplay();
	}
}

void AGekkoGameState::HandleTime()
{
	if (ReplayTakeoverStartTimer > 0)
	{
		ReplayTakeoverStartTimer -= ONE_FRAME;
		if (ReplayTakeoverStartTimer <= 0.f)
		{
			SetGamePaused(false);
			UE_LOG(LogGekkoGame, Log, TEXT("Replay takeover started!"));
		}
	}
	ElapsedTime -= ONE_FRAME;
}

void AGekkoGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	while (ElapsedTime >= ONE_FRAME)
	{
		//while elapsed time is greater than one frame...
		if (!ShouldPauseGame())
		{
			UpdateGame();
		}
		HandleTime();
	}
	OnUnrealDraw();
}

void AGekkoGameState::RewindToSnapshot(int32 InFrame)
{
	if (CanRewind())
	{
		if (ReplayManager->RewindToSnapshot(InFrame, &Gs.state))
		{
			LocalFrame = ReplayManager->GetReplayFrame();
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

void AGekkoGameState::TakeoverReplay(int32 InTakeoverIndex)
{
	if (!ReplayManager || !ReplayManager->IsPlayingBackReplay())
	{
		return;
	}
	ReplayTakeoverIndex = InTakeoverIndex;
	ReplayTakeoverSnapshotFrame = LocalFrame;
	
	ReplayTakeoverStartTimer = 1.f;
	bReplayTakeoverEnabled = true;
}

void AGekkoGameState::RewindToTakeoverSnapshot()
{
	if (!bReplayTakeoverEnabled)
	{
		return;
	}
	RewindToSnapshot(ReplayTakeoverSnapshotFrame);
}

void AGekkoGameState::EndTakeover()
{
	RewindToSnapshot(ReplayTakeoverSnapshotFrame);
	SetGamePaused(true);
	bReplayTakeoverEnabled = false;
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
	bool bIsReplayPlaying = ReplayManager && 
		ReplayManager->GetManagerBehaviour() == ERedoReplayMode::Playback;
	
	if (bIsOnlinePlay)
	{
		UGekkoNetSubsystem* GNS = GI->GetSubsystem<UGekkoNetSubsystem>();
		if (!GNS->IsSessionRunning())
		{
			FGekkoConfig SessionConfig {
				2,
				0,
				8,
				0,
				sizeof(GekkoGame::Input),
				sizeof(GekkoGame::Gamestate::state),
				false,
				false, };
			GNS->SetTransportType(GekkoTransportMethod);
#if WITH_EDITOR
			GNS->SetLocalAdapter(GI->PlayerId);
#endif
			GNS->SetSimulationHost(this);
			GNS->StartSession(SessionConfig, GI->PlayerId == 0 ? 7000 : 7001, false);

			for (int i = 0; i < 2; ++i)
			{
				if (i == GI->PlayerId)
				{
					NetLocalPlayerID = GNS->AddActor();
				}
				else
				{
					GNS->AddActor(EGekkoPlayerType::RemotePlayer, GI->PlayerId == 0 ? "127.0.0.1:7001" : "127.0.0.1:7000");
				}
			}
			
			GNS->OnPlayerDisconnected.AddUniqueDynamic(this, &AGekkoGameState::OnPlayerDisconnected);
		}
		GNS->UpdateSession();
		if (NetworkStatsTimer <= 0)
		{
			NetStats = GNS->UpdateNetworkStats(NetLocalPlayerID ^ 1);
			NetworkStatsTimer = MAX_NET_STATS_TIME;
		}
		
		NetworkStatsTimer -= 1;
		NetworkStatsTimer = FMath::Max(NetworkStatsTimer, 0);
		return;
	}
	
	// grab inputs from replay system if a replay is active.
	GekkoGame::Input Inputs[GekkoGame::MAX_PLAYERS] = {};
	bool bResumeGame = true;
	HandleBuffer();
	if (bIsReplayPlaying)
	{
		if (LocalFrame < ReplayManager->GetReplayLengthInFrames())
		{
			ReplayManager->RetrieveInputsFromReplay(&Inputs);
			if (bReplayTakeoverEnabled)
			{
				Inputs[ReplayTakeoverIndex] = PollInput(ReplayTakeoverIndex);
			}
		}
		else
		{
			bResumeGame = false;
		}
	}
	else
	{
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
	if (ReplayManager)
	{
		if (ReplayManager->IsRecording())
		{
			if (Event && Event->data.adv.rolling_back)
			{
				ReplayManager->SetLocalFrame(Event->data.adv.frame);
			}
			ReplayManager->RecordInputsForReplay(Inputs);
		}
	}
	Gs.Update(Inputs);
	++LocalFrame;
	if (Event != nullptr)
	{
		++RemoteFrame;
	}
	if (ReplayManager)
	{
		ReplayManager->AdvanceLocalFrame();
	}
}

void AGekkoGameState::GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData)
{
	GekkoGame::Input LocalInput = PollLatestInput(0);
	FMemory::Memcpy(OutInputData, &LocalInput, sizeof(GekkoGame::Input));
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

void AGekkoGameState::GekkoAdvance(GekkoGameEvent* Event)
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

void AGekkoGameState::OnPlayerDisconnected(int32 Handle)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DisconnectLevel, true);
}

bool AGekkoGameState::CanRewind()
{
	return ReplayManager && ReplayManager->GetManagerBehaviour() == ERedoReplayMode::Playback;
}

bool AGekkoGameState::CanPause()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	UGekkoNetSubsystem* GNS = GI->GetSubsystem<UGekkoNetSubsystem>();
	return !GNS->IsSessionRunning();
}

bool AGekkoGameState::ShouldPauseGame() const
{
	return bGamePaused || ReplayTakeoverStartTimer > 0;
}

void AGekkoGameState::SetGamePaused(bool bPaused)
{
	if (CanPause())
	{
		bGamePaused = bPaused;
		UE_LOG(LogGekkoGame, Log, TEXT("%s game frame %d."), bGamePaused ? TEXT("Paused") : TEXT("Unpaused"), LocalFrame);
	}
}

void AGekkoGameState::TogglePause()
{
	SetGamePaused(!bGamePaused);
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
