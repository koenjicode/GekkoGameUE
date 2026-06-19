// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoNetSimulationInterface.h"
#include "GekkoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GekkoBaseState.generated.h"

class UGekkoGameInstance;
/**
 * 
 */
UCLASS()
class GEKKOGAMEUE_API AGekkoBaseState : public AGameStateBase, public IGekkoNetSimulationInterface
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(BlueprintPure)
	virtual bool CanPause();
	UFUNCTION(BlueprintPure)
	virtual bool ShouldPauseGame() const;
	UFUNCTION(BlueprintCallable)
	void SetGamePaused(bool bPaused);
	UFUNCTION(BlueprintCallable)
	void TogglePause();
	
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintPure)
	virtual bool IsOffline();
	
	virtual void FixedTick();
	
	UFUNCTION()
	virtual AGekkoPlayerState* GetOpponentState() const;
	UFUNCTION()
	virtual FString GetOpponentAddress() const;
	UFUNCTION()
	virtual FString GetHostAddress() const;
	
	virtual void StartGekkoSession(uint8 InIndex);
	
	virtual void GekkoAdvance(GekkoGameEvent* Event) override;
	virtual void GekkoGetLocalInput(int32 LocalPlayer, void* OutInputData) override;
	virtual void GekkoLoad(GekkoGameEvent* Event) override;
	virtual void GekkoSave(GekkoGameEvent* Event) override;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UGekkoGameInstance> GekkoGameInstance;
	UPROPERTY(BlueprintReadOnly)
	int32 NetLocalPlayerID;
	UPROPERTY(VisibleInstanceOnly, Replicated)
	bool bMatchStarted;
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly)
	bool bGamePaused;

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PlayerDisconnected(int32 Handle);
	
	UPROPERTY(VisibleDefaultsOnly)
	bool bGekkoSessionStarted;
	
private:
	float ElapsedTime;
};
