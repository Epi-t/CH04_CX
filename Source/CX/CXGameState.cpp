#include "CXGameState.h"
#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ACXGameState::ACXGameState() { bReplicates = true; }

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXGameState, GamePhase);
    DOREPLIFETIME(ACXGameState, RemainingTime);
    DOREPLIFETIME(ACXGameState, Winners);
}

bool ACXGameState::IsWinner(const ACXPlayerState* State) const
{
    return IsValid(State) && Winners.Contains(State);
}

void ACXGameState::SetPhase(ECXGamePhase NewPhase)
{
    if (!HasAuthority()) return;
    GamePhase = NewPhase;
    ForceNetUpdate();
}

void ACXGameState::SetRemainingTime(int32 Seconds)
{
    if (!HasAuthority()) return;
    RemainingTime = FMath::Max(0, Seconds);
    ForceNetUpdate();
}

void ACXGameState::SetWinners(const TArray<ACXPlayerState*>& NewWinners)
{
    if (!HasAuthority()) return;
    Winners = NewWinners;
    ForceNetUpdate();
}

void ACXGameState::MulticastPickupFX_Implementation(FVector_NetQuantize Location, UParticleSystem* Particle, USoundBase* Sound)
{
    if (Particle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, Location);
    }
    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
    }
}
