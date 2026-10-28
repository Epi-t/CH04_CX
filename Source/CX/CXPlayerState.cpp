#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ACXPlayerState, BonusMaxSpeed, COND_OwnerOnly);
}

void ACXPlayerState::AddCoins(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0) return;
    Coins = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Coins) + Amount, 0, MAX_int32));
    ForceNetUpdate();
}

bool ACXPlayerState::TrySpendCoins(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0 || Coins < Amount) return false;
    Coins -= Amount;
    ForceNetUpdate();
    return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
    if (!HasAuthority() || !FMath::IsFinite(Amount)) return;
    BonusMaxSpeed = FMath::Clamp(BonusMaxSpeed + Amount, -500.f, 500.f);
    ForceNetUpdate();
}

void ACXPlayerState::ApplyWheelPenalty(float Penalty)
{
    if (!HasAuthority() || Penalty <= 0.f || !FMath::IsFinite(Penalty)) return;
    // Track exactly how much should be restored, so recovery never adds an extra bonus.
    const float Applied = FMath::Min(Penalty, FMath::Max(0.f, BonusMaxSpeed + 500.f));
    OutstandingWheelPenalty += Applied;
    AddBonusMaxSpeed(-Applied);
    if (UWorld* World = GetWorld())
    {
        if (!World->GetTimerManager().IsTimerActive(WheelRecoveryHandle))
        {
            World->GetTimerManager().SetTimer(WheelRecoveryHandle, this,
                &ACXPlayerState::RecoverWheelPenalty, 1.0f, true);
        }
    }
}

void ACXPlayerState::RecoverWheelPenalty()
{
    if (!HasAuthority()) return;
    const float Recovery = FMath::Min(5.f, OutstandingWheelPenalty);
    OutstandingWheelPenalty -= Recovery;
    AddBonusMaxSpeed(Recovery);
    if (OutstandingWheelPenalty <= KINDA_SMALL_NUMBER)
    {
        OutstandingWheelPenalty = 0.f;
        GetWorldTimerManager().ClearTimer(WheelRecoveryHandle);
    }
}

void ACXPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(WheelRecoveryHandle);
    Super::EndPlay(EndPlayReason);
}
