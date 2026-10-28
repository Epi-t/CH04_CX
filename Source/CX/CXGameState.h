#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;
class UParticleSystem;
class USoundBase;

UENUM(BlueprintType)
enum class ECXGamePhase : uint8
{
    Waiting UMETA(DisplayName="Waiting"),
    Playing UMETA(DisplayName="Playing"),
    Finished UMETA(DisplayName="Finished")
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    ACXGameState();
    UFUNCTION(BlueprintPure, Category="CX|Game") ECXGamePhase GetGamePhase() const { return GamePhase; }
    UFUNCTION(BlueprintPure, Category="CX|Game") int32 GetRemainingTime() const { return RemainingTime; }
    const TArray<ACXPlayerState*>& GetWinners() const { return Winners; }
    UFUNCTION(BlueprintPure, Category="CX|Game") int32 GetWinnerCount() const { return Winners.Num(); }
    UFUNCTION(BlueprintPure, Category="CX|Game") bool IsPlaying() const { return GamePhase == ECXGamePhase::Playing; }
    UFUNCTION(BlueprintPure, Category="CX|Game") bool IsWinner(const ACXPlayerState* State) const;

    void SetPhase(ECXGamePhase NewPhase);
    void SetRemainingTime(int32 Seconds);
    void SetWinners(const TArray<ACXPlayerState*>& NewWinners);

    // Persistent replicated GameState is safer than multicasting from an item immediately before Destroy().
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPickupFX(FVector_NetQuantize Location, UParticleSystem* Particle, USoundBase* Sound);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(Replicated, VisibleAnywhere, Category="CX|Game") ECXGamePhase GamePhase = ECXGamePhase::Waiting;
    UPROPERTY(Replicated, VisibleAnywhere, Category="CX|Game") int32 RemainingTime = 0;
    UPROPERTY(Replicated, VisibleAnywhere, Category="CX|Game") TArray<ACXPlayerState*> Winners;
};
