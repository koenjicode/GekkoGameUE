// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareGameState.h"
#include "GekkoNetSubsystem.h"
#include "SquarePawn.h"
#include "GekkoGameUE/GekkoGameLog.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"
#include "Kismet/GameplayStatics.h"

static const FLinearColor PlayerColors[] =
{
	FLinearColor::Red,
	FLinearColor::Green,
	FLinearColor::Blue,
	FLinearColor::Black
};

ASquareGameState::ASquareGameState()
{
	SquareActorClass = ASquarePawn::StaticClass();
}

void ASquareGameState::BeginPlay()
{
	Super::BeginPlay();
	
	for (int i = 0; i < MAX_SQUARE_PLAYERS; ++i)
	{
		SquareActors[i] = GetWorld()->SpawnActor<ASquarePawn>(SquareActorClass);
		SquareActors[i]->RealY += 250 * i * SQUAREGAME_FIXED_SCALE;
		SquareActors[i]->SetDefaultColor(PlayerColors[i]);
	}
	
	RollbackStateSize = SquareActors[0]->SaveForRollback().Num() * MAX_SQUARE_PLAYERS;
	RollbackState.AddUninitialized(RollbackStateSize);
	SaveState();
}

void ASquareGameState::UpdateOnline()
{
	auto GekkoNet = GekkoGameInstance->GetSubsystem<UGekkoNetSubsystem>();
	if (!GekkoNet->IsSessionRunning())
	{
		return;
	}
	GekkoNet->UpdateSession();
}

void ASquareGameState::UpdateOffline()
{
	for (int i = 0; i < MAX_SQUARE_PLAYERS; ++i)
	{
		Inputs[i] = PollInputs(i);
	}
	AdvanceGameState(Inputs);
}

void ASquareGameState::UpdateGame()
{
	if (!HasMatchStarted())
	{
		return;
	}
	if (!IsOffline())
	{
		UpdateOnline();
	}
	else
	{
		UpdateOffline();
	}
}

void ASquareGameState::FixedTick()
{
	UpdateGame();
	UpdateView();
}

void ASquareGameState::UpdateView()
{
	for (int i = 0; i < MAX_SQUARE_PLAYERS; ++i)
	{
		SquareActors[i]->ViewTick();
	}
}

void ASquareGameState::GekkoAdvance(GekkoGameEvent* Event)
{
	FMemory::Memcpy(&Inputs, Event->data.adv.inputs, Event->data.adv.input_len);
	if (Event->data.adv.rolling_back)
	{
		RemoteFrame = Event->data.adv.frame;
		UE_LOG(LogGekkoGame, Warning, TEXT("Rollback occured on frame %d"), Event->data.adv.frame);
	}
	
	AdvanceGameState(Inputs);
	RemoteFrame++;
}

void ASquareGameState::GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData)
{
	auto LocalInput = PollInputs(0);
	FMemory::Memcpy(OutInputData, &LocalInput, sizeof(FSquareInputs));
}

void ASquareGameState::GekkoLoad(GekkoGameEvent* Event)
{
	Super::GekkoLoad(Event);
	
	FMemory::Memcpy(RollbackState.GetData(), Event->data.load.state, Event->data.load.state_len);
	LoadState();
	UE_LOG(LogGekkoGame, Warning, TEXT("Loaded frame %d"), Event->data.load.frame);
}

void ASquareGameState::GekkoSave(GekkoGameEvent* Event)
{
	SaveState();
	
	*Event->data.save.state_len = RollbackStateSize;
	FMemory::Memcpy(Event->data.save.state, RollbackState.GetData(), RollbackStateSize);
	
	const uint32 Checksum = FCrc::MemCrc32(RollbackState.GetData(), *Event->data.save.state_len);
	*Event->data.save.checksum = Checksum;
	
	UE_LOG(LogGekkoGame, Warning, TEXT("Saved frame %d"), Event->data.save.frame);
}

void ASquareGameState::SaveState()
{
	RollbackState.Reset();
	
	for (int i = 0; i < MAX_SQUARE_PLAYERS; i++)
	{
		auto PawnChunk = SquareActors[i]->SaveForRollback();
		RollbackState.Append(PawnChunk);
	}
}

void ASquareGameState::LoadState()
{
	if (RollbackStateSize < 1)
	{
		return;
	}
	int32 PlayerStateSize = RollbackStateSize / MAX_SQUARE_PLAYERS;
	TArray<uint8> RollbackChunk;
	RollbackChunk.AddUninitialized(PlayerStateSize);
	
	for (int i = 0; i < MAX_SQUARE_PLAYERS; i++)
	{
		FMemory::Memcpy(RollbackChunk.GetData(), RollbackState.GetData() + (PlayerStateSize * i), PlayerStateSize);
		SquareActors[i]->LoadForRollback(RollbackChunk);
	}
}

FSquareInputs ASquareGameState::PollInputs(int32 Player)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, Player);
	FSquareInputs PlayerInputs = {};

	if (PC)
	{
		constexpr float Deadzone = 0.2f;

		const float LeftX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
		const float LeftY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

		PlayerInputs.Left =
			PC->IsInputKeyDown(EKeys::A) ||
			PC->IsInputKeyDown(EKeys::Left) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Left) ||
			LeftX < -Deadzone;

		PlayerInputs.Right =
			PC->IsInputKeyDown(EKeys::D) ||
			PC->IsInputKeyDown(EKeys::Right) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Right) ||
			LeftX > Deadzone;
		
		PlayerInputs.Up =
			PC->IsInputKeyDown(EKeys::W) ||
			PC->IsInputKeyDown(EKeys::Up) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up) ||
			LeftY > Deadzone;

		PlayerInputs.Down =
			PC->IsInputKeyDown(EKeys::S) ||
			PC->IsInputKeyDown(EKeys::Down) ||
			PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down) ||
			LeftY < -Deadzone;
		
		PlayerInputs.A =
			PC->IsInputKeyDown(EKeys::Z) ||
			PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
	}
	
	return PlayerInputs;
}

void ASquareGameState::AdvanceGameState(FSquareInputs InInputs[4])
{
	for (int i = 0; i < MAX_SQUARE_PLAYERS; i++)
	{
		auto& SquarePawn = SquareActors[i];
		SquarePawn->FixedTick(InInputs[i]);
	}
	LocalFrame++;
}

int32 ASquareGameState::GetPlayerCount()
{
	return MAX_SQUARE_PLAYERS;
}

ASquarePawn* ASquareGameState::GetSquareActor(int Index)
{
	if (Index > MAX_SQUARE_PLAYERS)
	{
		return nullptr;
	}
	return SquareActors[Index];
}
