
#include "RedoReplayManager.h"
#include "Redo.h"
#include "RedoReplaySaveData.h"
#include "Kismet/GameplayStatics.h"

#define LAST_SNAPSHOT_FRAME_PADDING 1

ARedoReplayManager::ARedoReplayManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARedoReplayManager::Init(int32 InStateSize, int32 InInputSize, int32 InPlayerNum, URedoReplaySaveData* DataToUse, int32 SnapshotFrameInterval)
{
	StateSize = InStateSize;
	InputSizePerPlayer = InInputSize;
	NumPlayers = InPlayerNum;
	if (DataToUse)
	{
		PlaybackData = DataToUse;
		CurrentReplayBehaviour = ERedoReplayMode::Playback;
		StateSnapshotBuffer.Reserve((PlaybackData->ReplayLengthInFrames / SnapshotFrameInterval) * StateSize);
	}
	else
	{
		CurrentReplayBehaviour = ERedoReplayMode::Recording;
	}
}

void ARedoReplayManager::AddSnapshot(void* InSnapshot)
{
	StateSnapshotBuffer.AddUninitialized(StateSize);
	
	int32 WriteOffset = (LocalReplayFrame * StateSize);
	void* Dest = (StateSnapshotBuffer.GetData() + WriteOffset);
	FMemory::Memcpy(Dest, InSnapshot, StateSize);
}

bool ARedoReplayManager::RewindToSnapshot(int32 InFrame, void* OutState)
{
	// TODO, add additional assignable events when you've hit the limits of a replay.
	if (!PlaybackData)
	{
		UE_LOG(LogRedo, Warning, TEXT("No playback data is present, cannot rewind."));
		return false;
	}
	if (LocalReplayFrame >= PlaybackData->ReplayLengthInFrames && InFrame >= PlaybackData->ReplayLengthInFrames)
	{
		UE_LOG(LogRedo, Warning, TEXT("Cannot rewind to frame %d after reaching maximum replay length (frame %d)"), InFrame, LocalReplayFrame);
		return false;
	}
	const int32 Max = PlaybackData->ReplayLengthInFrames - LAST_SNAPSHOT_FRAME_PADDING;
	InFrame = FMath::Clamp(InFrame, 0, Max);
	
	int32 SnapshotOffset = InFrame * StateSize;
	void* Dest = (StateSnapshotBuffer.GetData() + SnapshotOffset);
	FMemory::Memcpy(OutState, Dest, StateSize);
	
	SetLocalFrame(InFrame);
	UE_LOG(LogRedo, Log, TEXT("Rewinded to Snapshot on frame %d."), LocalReplayFrame);
	return true;
}

void ARedoReplayManager::RecordInputsForReplay(void* InInputs)
{
	int32 InputSize = InputSizePerPlayer * NumPlayers;
	// Allocates the amount of space needed based on the amount of frames passed in comparison to the last time the buffer was scaled.
	// This is useful if Redo is being used with a rollback solution, we can theoretically get incorrect input buffers. Hopefully!
	int32 AddSize = ((LocalReplayFrame + 1) * InputSize) - DataBuffer.Num();
	if (AddSize > 0)
	{
		DataBuffer.AddUninitialized(AddSize);
	}
	int32 WriteOffset = (LocalReplayFrame * InputSize);
	FMemory::Memcpy(DataBuffer.GetData() + WriteOffset, InInputs, InputSize);
}

void ARedoReplayManager::RetrieveInputsFromReplay(int32 Frame, void* OutInputs)
{
	int32 InputSize = InputSizePerPlayer * NumPlayers;
	int32 InputOffset = Frame * InputSize;
	FMemory::Memcpy(OutInputs, PlaybackData->Data.GetData() + InputOffset, InputSize);
}

void ARedoReplayManager::RetrieveInputsFromReplay(void* Inputs)
{
	RetrieveInputsFromReplay(LocalReplayFrame, Inputs);
}

URedoReplaySaveData* ARedoReplayManager::FindReplay(FString SearchString, int32 Index)
{
	if (UGameplayStatics::DoesSaveGameExist(SearchString, Index))
	{
		return Cast<URedoReplaySaveData>(UGameplayStatics::LoadGameFromSlot(SearchString, Index));
	}
	return nullptr;
}

void ARedoReplayManager::SaveReplay()
{
	auto NewSave = Cast<URedoReplaySaveData>(UGameplayStatics::CreateSaveGameObject(URedoReplaySaveData::StaticClass()));
	if (!NewSave) return;
		
	NewSave->Timestamp = FDateTime::Now();
	NewSave->ReplayLengthInFrames = LocalReplayFrame;
		
	NewSave->InputSizePerPlayer = InputSizePerPlayer;
	NewSave->PlayerCount = NumPlayers;
	// TODO: Potentially add a custom serialization to reduce the size of the data?
	NewSave->Data = DataBuffer;
	
	UGameplayStatics::SaveGameToSlot(NewSave, ReplayName, 0);
}

void ARedoReplayManager::SetReplayData(URedoReplaySaveData* DataToUse)
{
	PlaybackData = DataToUse;
}

void ARedoReplayManager::SetLocalFrame(int32 InLocalFrame)
{
	const int32 Max = PlaybackData ? PlaybackData->ReplayLengthInFrames : InLocalFrame;
	LocalReplayFrame = FMath::Clamp(InLocalFrame, 0, Max);
}

void ARedoReplayManager::AdvanceLocalFrame()
{
	SetLocalFrame(LocalReplayFrame + 1);
}

int32 ARedoReplayManager::GetReplayLengthInFrames() const
{
	if (PlaybackData == nullptr)
	{
		return 0;
	}
	return PlaybackData->ReplayLengthInFrames;
}

float ARedoReplayManager::GetReplayLength() const
{
	const int32 InFrames = GetReplayLengthInFrames();
	return InFrames * (1.f/60);
}

