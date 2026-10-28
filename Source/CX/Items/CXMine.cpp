#include "Items/CXMine.h"
#include "CXPawn.h"
#include "CXGameState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ACXMine::ACXMine()
{
    bReplicates = true;
    SetReplicateMovement(false);
    TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
    SetRootComponent(TriggerSphere);
    TriggerSphere->SetSphereRadius(125.f);
    TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
    TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerSphere->SetGenerateOverlapEvents(true);
    MineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MineMesh"));
    MineMesh->SetupAttachment(TriggerSphere);
    MineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InitialLifeSpan = 60.f;
}

void ACXMine::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &ACXMine::OnMineOverlap);
}

void ACXMine::OnMineOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
    if (!HasAuthority() || IsActorBeingDestroyed() || OtherActor == GetOwner()) return;
    ACXPawn* Vehicle = Cast<ACXPawn>(OtherActor);
    if (!IsValid(Vehicle)) return;
    Vehicle->DisableThrottleForSeconds(2.f);
    if (ACXGameState* GS = GetWorld()->GetGameState<ACXGameState>())
    {
        GS->MulticastPickupFX(GetActorLocation(), ExplosionParticle, ExplosionSound);
    }
    Destroy();
}
