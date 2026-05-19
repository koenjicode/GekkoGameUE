// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoSliderState.h"
#include "GekkoGameInstance.h"
#include "GekkoNetSubsystem.h"
#include "Kismet/GameplayStatics.h"

static constexpr float OneFrame = 1.0f / 60;

void AGekkoSliderState::Init()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	
	if (!GI)
		return;
	
	RemoteEndpoints.Add(0, {"127.0.0.1", 5000});
	RemoteEndpoints.Add(1, {"127.0.0.1", 5001});
	RemoteEndpoints.Add(2, {"127.0.0.1", 5002});
	RemoteEndpoints.Add(3, {"127.0.0.1", 5003});
	
	if (GI->bLocalPlayEnabled)
	{
		for (int i = 0; i < SliderGame::MAX_PLAYERS; ++i)
		{
			if (i > 0)
			{
				UGameplayStatics::CreatePlayer(GetWorld(), i , true);
			}
		}
	}
	Gs = {};
}

void AGekkoSliderState::BeginPlay()
{
	Super::BeginPlay();
	Init();
}

void AGekkoSliderState::Shutdown()
{
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (GNS && GNS->IsSessionRunning())
	{
		GNS->EndSession();
	}
	for (int i = 0; i < SliderGame::MAX_PLAYERS; ++i)
	{
		if (i > 0)
		{
			UGameplayStatics::RemovePlayer(UGameplayStatics::GetPlayerController(GetWorld(), i), true);
		}
	}
	FMemory::Memzero(&Gs, sizeof(Gs));
}

void AGekkoSliderState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Shutdown();
}

SliderGame::Input AGekkoSliderState::PollInput(int32 Index)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, Index);
	SliderGame::Input inp = {};

	if (PC)
	{
		constexpr float Deadzone = 0.5f;

		const float LeftX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);

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

void AGekkoSliderState::GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData)
{
	auto LocalInput = PollInput(0);
	FMemory::Memcpy(OutInputData, &LocalInput, sizeof(LocalInput));
}

void AGekkoSliderState::UpdateOnline()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetWorld()->GetGameInstance());
	
	if (!GI)
		return;
	
	UGekkoNetSubsystem* GNS = GetGameInstance()->GetSubsystem<UGekkoNetSubsystem>();
	if (GNS->IsSessionRunning())
	{
		GNS->UpdateSession();
	}
	else
	{
		FGekkoConfig Cfg {
			SliderGame::MAX_PLAYERS,
			0, 
			8,
			0,
			sizeof(SliderGame::Input),
			sizeof(SliderGame::State),
			false,
			false,
			0 };
		
		auto LocalID = GI->PlayerId;
		FSliderEndpoint LocalEndpoint = RemoteEndpoints.FindRef(LocalID);
		
		GNS->SetLocalAdapter(LocalID);
		GNS->SetSimulationHost(this);
		GNS->StartSession(Cfg, LocalEndpoint.Port, false);

		for (int i = 0; i < SliderGame::MAX_PLAYERS; ++i)
		{
			if (i == LocalID)
			{
				GNS->AddActor();
			}
			else
			{
				FSliderEndpoint RemoteEndpoint = RemoteEndpoints.FindRef(i);
				const FString AddressString = FString::Printf(TEXT("%s:%d"), *RemoteEndpoint.Address, RemoteEndpoint.Port);
				GNS->AddActor(EGekkoPlayerType::RemotePlayer, AddressString);
			}
		}
		
		GNS->OnPlayerDisconnected.AddUniqueDynamic(this, &AGekkoSliderState::OnPlayerDisconnected);
	}
}

void AGekkoSliderState::AdvanceGame(SliderGame::Input Inputs[SliderGame::MAX_PLAYERS])
{
	Gs.tick(Inputs);
	LocalFrame++;
}

void AGekkoSliderState::OnPlayerDisconnected(int32 Handle)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DisconnectLevel, true);
}

void AGekkoSliderState::UpdateOffline()
{
	for (int i = 0; i < 4; ++i)
	{
		GameInputs[i] = PollInput(i);
	}
	AdvanceGame(GameInputs);
}

void AGekkoSliderState::UpdateGame()
{
	UGekkoGameInstance* GI = Cast<UGekkoGameInstance>(GetGameInstance());
	
	if (!GI)
		return;

	if (!GI->bLocalPlayEnabled)
	{
		UpdateOnline();
	}
	else
	{
		UpdateOffline();
	}
}

void AGekkoSliderState::GekkoAdvance(GekkoGameEvent* Event)
{
	for (int j = 0; j < SliderGame::MAX_PLAYERS; j++) 
	{
		GameInputs[j] = ((SliderGame::Input*)(Event->data.adv.inputs))[j];
	}
	if (Event->data.adv.rolling_back)
	{
		RemoteFrame = Event->data.adv.frame;
	}
	AdvanceGame(GameInputs);
	RemoteFrame++;
}

void AGekkoSliderState::GekkoLoad(GekkoGameEvent* Event)
{
	FMemory::Memcpy(&Gs, Event->data.load.state, sizeof(Gs));
}

void AGekkoSliderState::GekkoSave(GekkoGameEvent* Event)
{
	*Event->data.save.state_len = sizeof(Gs);
	FMemory::Memcpy(Event->data.save.state, &Gs, sizeof(Gs));
	
	const uint32 Checksum = FCrc::MemCrc32(&Gs, sizeof(Gs));
	*Event->data.save.checksum = Checksum;
}

void AGekkoSliderState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	while (ElapsedTime >= OneFrame)
	{
		if (!bPaused)
		{
			UpdateGame();
		}
		ElapsedTime -= OneFrame;
	}
}

FLinearColor AGekkoSliderState::GetMaterialColor(int32 Index)
{
	if (Index > SliderGame::MAX_PLAYERS)
		return FColor::White;
	
	const int32 Offset = Index * SliderGame::MAX_PLAYERS;
	return FColor(SliderGame::PLAYER_COLORS[Offset], SliderGame::PLAYER_COLORS[Offset + 1], SliderGame::PLAYER_COLORS[Offset + 2], SliderGame::PLAYER_COLORS[Offset + 3]);
}

FVector AGekkoSliderState::GetCubePosition(int32 Index)
{
	if (Index > SliderGame::MAX_PLAYERS)
		return FVector::ZeroVector;
	
	return FVector(Gs.entt_px[Index], 0.f, 0.f);
}
