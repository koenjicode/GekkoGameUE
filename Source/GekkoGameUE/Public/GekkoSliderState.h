// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sliders.h"
#include "GekkoNetSimulationInterface.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoSliderState.generated.h"

USTRUCT(BlueprintType)
struct FSliderEndpoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Address;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Port = 0;
};

UCLASS()
class GEKKOGAMEUE_API AGekkoSliderState : public AGameStateBase, public IGekkoNetSimulationInterface
{
	GENERATED_BODY()
	
protected:
	
	void Init();
	virtual void BeginPlay() override;
	void Shutdown();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:

	SliderGame::Input PollInput(int32 Index);
	
	void AdvanceGame(SliderGame::Input Inputs[]);

	UFUNCTION()
	void OnPlayerDisconnected(int32 Handle);
	
	void UpdateOnline();
	void UpdateOffline();
	void UpdateGame();
	
public:
	
	virtual void GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData) override;
	virtual void GekkoAdvance(GekkoGameEvent* Event) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;
	
	virtual void Tick(float DeltaSeconds) override;
	
	UFUNCTION(BlueprintPure)
	int32 GetSliderGameScale() const { return SliderGame::GAME_SCALE; }
	UFUNCTION(BlueprintPure)
	FLinearColor GetMaterialColor(int32 Index);
	UFUNCTION(BlueprintPure)
	FVector GetCubePosition(int32 Index);
	
	UPROPERTY()
	TMap<int32, FSliderEndpoint> RemoteEndpoints;
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DisconnectLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPaused = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	int32 LocalFrame = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	int32 RemoteFrame = 0;

private:
	SliderGame::Input GameInputs[SliderGame::MAX_PLAYERS];
	SliderGame::State Gs = {};
	float ElapsedTime = 0.f;
};
