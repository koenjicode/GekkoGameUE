// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareGameState.h"
#include "GekkoNetSubsystem.h"
#include "SquarePawn.h"
#include "GekkoGameUE/Core/GekkoGameInstance.h"
#include "Kismet/GameplayStatics.h"

static constexpr float OneFrame = 1.0f / 60;

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
	}
}

bool ASquareGameState::IsOffline() const
{
	return GetNetMode() == NM_Standalone && !GekkoGameInstance->bDirectMode;
}

void ASquareGameState::UpdateOnline()
{
	auto GekkoNet = GekkoGameInstance->GetSubsystem<UGekkoNetSubsystem>();
	
	if (!GekkoNet || !GekkoNet->IsSessionRunning())
	{
		return;
	}
	
	GekkoNet->UpdateSession();
}

void ASquareGameState::UpdateOffline()
{
	for (int i = 0; i < MAX_SQUARE_PLAYERS; ++i)
	{
		PollInputs(i);
	}
	AdvanceGameState(Inputs);
}

void ASquareGameState::UpdateGame()
{
	if (!IsOffline())
	{
		UpdateOnline();
	}
	else
	{
		UpdateOffline();
	}
}

void ASquareGameState::UpdateView()
{
	for (int i = 0; i < MAX_SQUARE_PLAYERS; ++i)
	{
		SquareActors[i]->ViewTick();
	}
}

void ASquareGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	while (ElapsedTime >= OneFrame)
	{
		if (!bPaused)
		{
			UpdateGame();
			UpdateView();
		}
		ElapsedTime -= OneFrame;
	}
}

void ASquareGameState::GekkoAdvance(GekkoGameEvent* Event)
{
	Super::GekkoAdvance(Event);
	FMemory::Memcpy(Inputs, Event->data.adv.inputs, sizeof(Inputs));
	if (Event->data.adv.rolling_back)
	{
		RemoteFrame = Event->data.adv.frame;
	}
	AdvanceGameState(Inputs);
	RemoteFrame++;
}

void ASquareGameState::GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData)
{
	Super::GekkoGetLocalInput(LocalPlayer, OutInputData);
}

void ASquareGameState::GekkoLoad(GekkoGameEvent* Event)
{
	Super::GekkoLoad(Event);
}

void ASquareGameState::GekkoSave(GekkoGameEvent* Event)
{
	Super::GekkoSave(Event);
}

void ASquareGameState::PollInputs(int32 Player)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, Player);
	auto& PlayerInputs = Inputs[Player];

	if (PC)
	{
		constexpr float Deadzone = 0.5f;

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
	}
}

void ASquareGameState::AdvanceGameState(FSquareInputs InInputs[4])
{
	for (int i = 0; i < 4; i++)
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
