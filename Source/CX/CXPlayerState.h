#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

// Player-owned values: only the owning client receives these replicated properties.
UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="CX|Stats") int32 GetCoins() const { return Coins; }
    UFUNCTION(BlueprintPure, Category="CX|Stats") float GetBonusMaxSpeed() const { return BonusMaxSpeed; }

    // Mutators run exclusively on the authority. They are not client RPCs.
    UFUNCTION(BlueprintCallable, Category="CX|Stats") void AddCoins(int32 Amount);
    UFUNCTION(BlueprintCallable, Category="CX|Stats") bool TrySpendCoins(int32 Amount);
    UFUNCTION(BlueprintCallable, Category="CX|Stats") void AddBonusMaxSpeed(float Amount);
    UFUNCTION(BlueprintCallable, Category="CX|Stats") void ApplyWheelPenalty(float Penalty);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Replicated, VisibleAnywhere, Category="CX|Stats") int32 Coins = 0;
    UPROPERTY(Replicated, VisibleAnywhere, Category="CX|Stats") float BonusMaxSpeed = 0.f;

    // Server-only balance for wheel damage, separate from legitimate speed bonuses.
    float OutstandingWheelPenalty = 0.f;
    FTimerHandle WheelRecoveryHandle;
    void RecoverWheelPenalty();
};
