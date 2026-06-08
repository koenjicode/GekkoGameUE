// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoGameState.h"
#include "GekkoGameInstance.h"
#include "GekkoGameLog.h"
#include "GekkoGameMode.h"
#include "GekkoNetLog.h"
#include "GekkoNetSubsystem.h"
#include "GekkoPlayerState.h"
#include "RedoReplayManager.h"
#include "RedoReplaySaveData.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

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

bool AGekkoGameState::ShouldSpawnReplayManager() const
{
	return GekkoGameInstance->bAllowRecording || GekkoGameInstance->bPlaybackLastSave;
}

void AGekkoGameState::CreateInputBuffers()
{
	if (!GekkoGameInstance->bLocalPlayEnabled)
	{
		return;
	}
	
	P1InputBuffer.Reserve(MAX_LOCAL_DELAY_FRAMES);
	P2InputBuffer.Reserve(MAX_LOCAL_DELAY_FRAMES);
}

void AGekkoGameState::PrepareReplay()
{
	if (!GekkoGameInstance->bPlaybackLastSave)
	{
		return;
	}
	
	auto Save = ReplayManager->FindReplay("REPLAY");
	
	if (Save && ReplayManager->GetReplayData()->ReplayLengthInFrames <= 0)
	{
		ReplayManager->Init(sizeof(Gs.state), sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, Save);
		int32 SnapshotCount = 0;
		GekkoGame::Input ReplayInputs[GekkoGame::MAX_PLAYERS] = {};
		
		Gs.Init(2);
		
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
	}
	else
	{
		ReplayManager->Init(sizeof(Gs.state), sizeof(GekkoGame::Input), GekkoGame::MAX_PLAYERS, nullptr);
	}
}

void AGekkoGameState::BeginPlay()
{
	Super::BeginPlay();
	
	GekkoGameInstance = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	
	if (ShouldSpawnReplayManager())
	{
		ReplayManager = GetWorld()->SpawnActor<ARedoReplayManager>(ReplayManagerClass);
	}
	
	CreateInputBuffers();
	
	if (ReplayManager)
	{
		PrepareReplay();
	}
	
	Gs.Init(2);
}

void AGekkoGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (ReplayManager && ReplayManager->GetManagerBehaviour() == ERedoReplayMode::Recording)
	{
		ReplayManager->SaveReplay();
	}
	
	FMemory::Memzero(&Gs, sizeof(Gs));
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (GNS && GNS->IsSessionRunning())
	{
		GNS->EndSession();
	}
}

void AGekkoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AGekkoGameState, bMatchStarted);
}

void AGekkoGameState::HandleReplayTakeoverTimer()
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
}

void AGekkoGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	while (ElapsedTime >= ONE_FRAME)
	{
		UpdateGame();
		OnUnrealDraw();
		ElapsedTime -= ONE_FRAME;
	}
}

FName AGekkoGameState::GetOnlineSubsystemName() const
{
	return Online::GetSubsystem(GetWorld())->GetSubsystemName();
}

bool AGekkoGameState::IsListenConnectedMatch() const
{
	return PlayerArray.Num() == 2;
}

int32 AGekkoGameState::GetPlayerID()
{
	if (PlayerArray.Num() != 2)
	{
		return -1;
	}
	
	int32 PlayerID = INDEX_NONE;
	const int32 LocalID = GetWorld()->GetFirstPlayerController()->PlayerState->GetPlayerId();
	
	for (int i = 0; i < PlayerArray.Num(); ++i)
	{
		if (LocalID == PlayerArray[i]->GetPlayerId())
		{
			PlayerID = i;
			break;
		}
	}
	
	return PlayerID;
}

int32 AGekkoGameState::GetOpponentID()
{
	if (PlayerArray.Num() != 2)
	{
		return -1;
	}
	
	return GetPlayerID() ^ 1;
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
	const int32 i = FMath::Max(0, MAX_LOCAL_DELAY_FRAMES - LocalDelay);
	return InputBuffer.IsValidIndex(i) ? InputBuffer[i] : GekkoGame::Input();
}

void AGekkoGameState::UpdateReplay()
{
	if (LocalFrame < ReplayManager->GetReplayLengthInFrames())
	{
		return;
	}
	
	HandleReplayTakeoverTimer();
	
	ReplayManager->RetrieveInputsFromReplay(&Inputs);
	
	if (bReplayTakeoverEnabled)
	{
		Inputs[ReplayTakeoverIndex] = PollInput(ReplayTakeoverIndex);
	}
	
	AdvanceGameState(Inputs);
}

void AGekkoGameState::UpdateOffline()
{
	if (ShouldPauseGame())
	{
		return;
	}
	
	HandleBuffer();
	
	for (int j = 0; j < 2; j++)
	{
		Inputs[j] = PollInput(j);
	}
	
	AdvanceGameState(Inputs);
}

void AGekkoGameState::UpdateOnline()
{
	UGekkoNetSubsystem* GNS = GekkoGameInstance->GetSubsystem<UGekkoNetSubsystem>();
		
	GNS->UpdateSession();
	
	if (NetworkStatsTimer <= 0)
	{
		NetStats = GNS->UpdateNetworkStats(NetLocalPlayerID ^ 1);
		NetworkStatsTimer = MAX_NET_STATS_TIME;
	}
		
	NetworkStatsTimer -= 1;
	NetworkStatsTimer = FMath::Max(NetworkStatsTimer, 0);
}

bool AGekkoGameState::IsPlayingOffline() const
{
	return GetNetMode() == NM_Standalone && !GekkoGameInstance->bDirectMode;
}

void AGekkoGameState::UpdateGame()
{
	if (!HasMatchStarted())
	{
		return;
	}
	
	if (!IsPlayingOffline())
	{
		UpdateOnline();
	}
	else if (ReplayManager && ReplayManager->GetManagerBehaviour() == ERedoReplayMode::Playback)
	{
		UpdateReplay();
	}
	else
	{
		UpdateOffline();
	}
}

void AGekkoGameState::AdvanceGameState(GekkoGame::Input InInputs[4], GekkoGameEvent* Event)
{
	if (ReplayManager && ReplayManager->IsRecording())
	{
		if (Event && Event->data.adv.rolling_back)
		{
			ReplayManager->SetLocalFrame(Event->data.adv.frame);
		}
		ReplayManager->RecordInputsForReplay(InInputs);
	}
	
	Gs.Update(InInputs);
	++LocalFrame;
	
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
	UE_LOG(LogGekkoGame, Verbose, TEXT("f%d,"), Event->data.adv.frame);
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	GekkoGame::Input inputs[GekkoGame::MAX_PLAYERS] = {};
	for (int j = 0; j < GNS->GetNumOfPlayers(); j++) 
	{
		inputs[j] = ((GekkoGame::Input*)(Event->data.adv.inputs))[j];
		UE_LOG(LogGekkoGame, VeryVerbose, TEXT(" p%d %d%d%d%d"), j, inputs[j].left, inputs[j].right, inputs[j].up,  inputs[j].down);
	}
	RemoteFrame = Event->data.adv.frame;
	AdvanceGameState(inputs, Event);
}

bool AGekkoGameState::CanRewind()
{
	return ReplayManager && ReplayManager->GetManagerBehaviour() == ERedoReplayMode::Playback;
}

bool AGekkoGameState::CanPause()
{
	return !GetWorld()->GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>()->IsSessionRunning();
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

AGekkoPlayerState* AGekkoGameState::GetOpponentState() const
{
	auto PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}
	auto LocalState = PC->PlayerState;
	if (!LocalState)
	{
		return nullptr;
	}
	for (int i = 0; i < PlayerArray.Num(); i++)
	{
		if (LocalState->GetPlayerId() != PlayerArray[i]->GetPlayerId())
		{
			return Cast<AGekkoPlayerState>(PlayerArray[i]);
		}
	}
	return nullptr;
}

bool AGekkoGameState::HasMatchStarted() const
{
	bool bCanStart;
	
	if (IsNetMode(NM_Standalone))
	{
		bCanStart = bMatchStarted;
	}
	else
	{
		bCanStart =  bMatchStarted && bGekkoSessionStarted;
	}
	
	if (bCanStart)
	{
		return Super::HasMatchStarted();
	}
	
	return false;
}

FString AGekkoGameState::GetOpponentAddress() const
{
	if (GekkoGameInstance->bDirectMode)
	{
		if (GekkoGameInstance->OpponentAddresses.Num() > 0)
		{
			return GekkoGameInstance->OpponentAddresses[0];
		}
	}
	
	return GetOpponentState()->GetUniqueId().ToString();
}

void AGekkoGameState::StartGekkoSession(uint8 InIndex)
{
	UGekkoNetSubsystem* GekkoNet = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();

	if (!GekkoNet)
	{
		return;
	}
	
	GekkoNet->SetSimulationHost(this);
	GekkoNet->bUseAsioTransport = GekkoGameInstance->bUsingAsio;
	GekkoNet->bUseDirectAdapterIfAvailable = GekkoGameInstance->bDirectMode;

	if (GekkoNet->bUseDirectAdapterIfAvailable)
	{
		GekkoNet->SetLocalPort(GekkoGameInstance->LocalPort);
	}

	FGekkoConfig SessionConfig 
	{
		2,
		0,
		8,
		0,
		sizeof(GekkoGame::Input),
		sizeof(GekkoGame::Gamestate::state),
		false,
		true, 
	};
	
	GekkoNet->StartSession(SessionConfig, false);
	FString OpponentAddress = GetOpponentAddress();
	
	if (InIndex == 0)
	{
		NetLocalPlayerID = GekkoNet->AddActor();
		GekkoNet->AddActor(EGekkoPlayerType::RemotePlayer, OpponentAddress);
	}
	else
	{
		GekkoNet->AddActor(EGekkoPlayerType::RemotePlayer, OpponentAddress);
		NetLocalPlayerID = GekkoNet->AddActor();
	}
	
	GekkoNet->OnPlayerDisconnected.AddUniqueDynamic(this, &AGekkoGameState::PlayerDisconnected);
	
	bGekkoSessionStarted = true;
	
	UE_LOG(LogGekkoGame, Log, TEXT("Started session as Player %d"), InIndex + 1);
}
