#include "Items/CXPickupItem.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

ACXPickupItem::ACXPickupItem()
{
    bReplicates = true;
    NetUpdateFrequency = 1.f;
    SetReplicateMovement(false);

    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    SetRootComponent(PickupSphere);
    PickupSphere->SetSphereRadius(140.f);
    PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
    PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    PickupSphere->SetGenerateOverlapEvents(true);

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(PickupSphere);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACXPickupItem::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ACXPickupItem::OnSphereOverlap);
    }
}

void ACXPickupItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || IsActorBeingDestroyed()) return;
    ACXPawn* Vehicle = Cast<ACXPawn>(OtherActor);
    if (!IsValid(Vehicle) || !GiveReward(Vehicle)) return;

    if (ACXGameState* GS = GetWorld()->GetGameState<ACXGameState>())
    {
        GS->MulticastPickupFX(GetActorLocation(), PickupParticle, PickupSound);
    }
    Destroy(); // destruction is replicated by the server
}

bool ACXCoinItem::GiveReward(ACXPawn* Vehicle)
{
    if (!Vehicle || !Vehicle->GetController()) return false;
    ACXPlayerState* PS = Vehicle->GetController()->GetPlayerState<ACXPlayerState>();
    if (!PS) return false;
    PS->AddCoins(10);
    return true;
}

bool ACXSpeedItem::GiveReward(ACXPawn* Vehicle)
{
    if (!Vehicle || !Vehicle->GetController()) return false;
    ACXPlayerState* PS = Vehicle->GetController()->GetPlayerState<ACXPlayerState>();
    if (!PS) return false;
    PS->AddBonusMaxSpeed(10.f);
    return true;
}
