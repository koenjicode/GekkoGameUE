
#include "RedoReplayDriver.h"

#include "RedoReplaySaveData.h"
#include "Kismet/GameplayStatics.h"

ARedoReplayDriver::ARedoReplayDriver()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARedoReplayDriver::Init(int32 StoreInputSize, int32 NumPlayers, URedoReplaySaveData* DataToUse)
{
	if (DataToUse)
	{
		PlaybackReplayData = DataToUse;
	}
	CurrentDriverState = PlaybackReplayData ? ERedoDriverType::Playback : ERedoDriverType::Recording;
	InputSizePerPlayer = StoreInputSize;
	NumOfPlayers = NumPlayers;
}

void ARedoReplayDriver::Init(int32 StoreInputSize, int32 NumPlayers)
{
	Init(StoreInputSize, NumPlayers, nullptr);
}

void ARedoReplayDriver::AdvanceLocalFrame()
{
	LocalReplayFrame++;
}

void ARedoReplayDriver::RecordInputs(void* InInputs)
{
	int32 InputSize = InputSizePerPlayer * NumOfPlayers;
	
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
	int32 InputSize = InputSizePerPlayer * NumOfPlayers;
	int32 InputOffset = Frame * InputSize;
	FMemory::Memcpy(OutInputs, PlaybackReplayData->Data.GetData() + InputOffset, InputSize);
}

void ARedoReplayDriver::ReciteInputs(void* Inputs)
{
	ReciteInputs(LocalReplayFrame, Inputs);
}

void ARedoReplayDriver::ReciteInputsForPlayer(int32 Frame, int32 ForPlayerIndex, void* OutInputs)
{
	int32 InputSize = InputSizePerPlayer * NumOfPlayers;
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
		
	NewSave->InputSizePerPlayer = InputSizePerPlayer;
	NewSave->PlayerCount = NumOfPlayers;
	NewSave->Data = DataBuffer;
		
	UGameplayStatics::SaveGameToSlot(NewSave, ReplayName, 0);
}

void ARedoReplayDriver::SetReplayData(URedoReplaySaveData* DataToUse)
{
	PlaybackReplayData = DataToUse;
}

void ARedoReplayDriver::SetLocalFrame(int32 NewLocalFrame)
{
	LocalReplayFrame = NewLocalFrame;
}

