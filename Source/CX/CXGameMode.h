#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class APlayerStart;
class ACXGameState;

// Authority owns countdowns, game rules, and winner selection.
UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACXGameMode();
protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
private:
    bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;
    void OnCountdownTick();
    void StartPlaying();
    void FinishMatch();
    ACXGameState* GetCXGameState() const;

    UPROPERTY(EditDefaultsOnly, Category="CX|Spawning") FVector VehicleSpawnCheckHalfExtent = FVector(250., 120., 80.);
    UPROPERTY(EditDefaultsOnly, Category="CX|Spawning") FVector VehicleSpawnCheckOffset = FVector(0., 0., 80.);
    UPROPERTY(EditDefaultsOnly, Category="CX|Game", meta=(ClampMin="1")) int32 WaitingSeconds = 5;
    UPROPERTY(EditDefaultsOnly, Category="CX|Game", meta=(ClampMin="1")) int32 PlayingSeconds = 30;
    bool bCountdownStarted = false;
    FTimerHandle CountdownHandle;
};
