
#include "RedoReplayDriver.h"

#include "RedoReplaySaveData.h"
#include "Kismet/GameplayStatics.h"

#define SNAPSHOT_FRAME_INTERVAL 1

ARedoReplayDriver::ARedoReplayDriver()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARedoReplayDriver::Init(int32 InStateSize, int32 InInputSize, int32 InPlayerNum, URedoReplaySaveData* DataToUse)
{
	StateSize = InStateSize;
	InputSizePerPlayer = InInputSize;
	NumPlayers = InPlayerNum;
	if (DataToUse)
	{
		PlaybackData = DataToUse;
		CurrentDriverState = ERedoReplayMode::Playback;
		StateSnapshotBuffer.Reserve((PlaybackData->ReplayLengthInFrames / SNAPSHOT_FRAME_INTERVAL) * StateSize);
	}
	else
	{
		CurrentDriverState = ERedoReplayMode::Recording;
	}
}

void ARedoReplayDriver::AdvanceLocalFrame()
{
	LocalReplayFrame++;
}

void ARedoReplayDriver::UpdateRecording(void* InInputs, bool bAdvanceReplayFrame)
{
	if (bAdvanceReplayFrame)
	{
		LocalReplayFrame++;
	}
	RecordInputs(InInputs);
}

void ARedoReplayDriver::UpdatePlayback(void* OutInputs,  bool bAdvanceReplayFrame)
{
	if (bAdvanceReplayFrame)
	{
		LocalReplayFrame++;
	}
	if (LocalReplayFrame < PlaybackData->ReplayLengthInFrames)
	{
		ReciteInputs(LocalReplayFrame, OutInputs);
	}
}

void ARedoReplayDriver::UpdatePlaybackTakeover(int32 PlayerIndex)
{
}

void ARedoReplayDriver::AddSnapshot(void* InSnapshot)
{
	StateSnapshotBuffer.AddUninitialized(StateSize);
	
	int32 WriteOffset = (LocalReplayFrame * StateSize);
	void* Dest = (StateSnapshotBuffer.GetData() + WriteOffset) - StateSize;
	FMemory::Memcpy(Dest, InSnapshot, StateSize);
}

bool ARedoReplayDriver::RewindToSnapshot(int32 InFrame, void* OutState)
{
	if (!PlaybackData || InFrame > PlaybackData->ReplayLengthInFrames)
	{
		return false;
	}
	int32 SnapshotOffset = InFrame * StateSize;
	void* Dest = (StateSnapshotBuffer.GetData() + SnapshotOffset) - StateSize;
	FMemory::Memcpy(OutState, Dest, StateSize);
	
	SetLocalFrame(InFrame);
	return true;
}

void ARedoReplayDriver::RecordInputs(void* InInputs)
{
	int32 InputSize = InputSizePerPlayer * NumPlayers;
	
	// Allocates the amount of space needed based on the amount of frames passed in comparison to the last time the buffer was scaled.
	// This is useful if Redo is being used with a rollback solution, we can theoretically get incorrect input buffers. Hopefully!
	int32 AddSize = (LocalReplayFrame * InputSize) - DataBuffer.Num();
	if (AddSize > 0)
	{
		DataBuffer.AddUninitialized(AddSize);
	}
	int32 WriteOffset = (LocalReplayFrame * InputSize) - InputSize;
	FMemory::Memcpy(DataBuffer.GetData() + WriteOffset, InInputs, InputSize);
}

void ARedoReplayDriver::ReciteInputs(int32 Frame, void* OutInputs)
{
	int32 InputSize = InputSizePerPlayer * NumPlayers;
	int32 InputOffset = Frame * InputSize;
	FMemory::Memcpy(OutInputs, PlaybackData->Data.GetData() + InputOffset, InputSize);
}

void ARedoReplayDriver::ReciteInputs(void* Inputs)
{
	ReciteInputs(LocalReplayFrame, Inputs);
}

void ARedoReplayDriver::ReciteInputsForPlayer(int32 Frame, int32 ForPlayerIndex, void* OutInputs)
{
	int32 InputSize = InputSizePerPlayer * NumPlayers;
	int32 InputOffset = Frame * InputSize;
	FMemory::Memcpy(OutInputs, DataBuffer.GetData() + InputOffset + (ForPlayerIndex * InputSizePerPlayer), InputSizePerPlayer);
}

void ARedoReplayDriver::ReciteInputsForPlayer(int32 ForPlayerIndex, void* OutInputs)
{
	ReciteInputsForPlayer(LocalReplayFrame, ForPlayerIndex, OutInputs);
}

URedoReplaySaveData* ARedoReplayDriver::FindReplay()
{
	FString ReplayName = "REPLAY";
	if (UGameplayStatics::DoesSaveGameExist(ReplayName, 0))
	{
		return Cast<URedoReplaySaveData>(UGameplayStatics::LoadGameFromSlot(ReplayName, 0));
	}
	return nullptr;
}

void ARedoReplayDriver::SaveReplay()
{
	auto NewSave = Cast<URedoReplaySaveData>(UGameplayStatics::CreateSaveGameObject(URedoReplaySaveData::StaticClass()));
	if (!NewSave) return;
	
	FString ReplayName = "REPLAY";
		
	NewSave->Timestamp = FDateTime::Now();
	NewSave->ReplayLengthInFrames = LocalReplayFrame;
		
	NewSave->InputSizePerPlayer = InputSizePerPlayer;
	NewSave->PlayerCount = NumPlayers;
	NewSave->Data = DataBuffer;
		
	UGameplayStatics::SaveGameToSlot(NewSave, ReplayName, 0);
}

void ARedoReplayDriver::SetReplayData(URedoReplaySaveData* DataToUse)
{
	PlaybackData = DataToUse;
}

void ARedoReplayDriver::SetLocalFrame(int32 NewLocalFrame)
{
	LocalReplayFrame = NewLocalFrame;
}

