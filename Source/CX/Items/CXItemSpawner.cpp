#include "Items/CXItemSpawner.h"
#include "Items/CXPickupItem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ACXItemSpawner::ACXItemSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false; // the item itself replicates
}

void ACXItemSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) SpawnItem();
}

void ACXItemSpawner::SpawnItem()
{
    if (!HasAuthority() || !ItemClass || IsValid(SpawnedItem)) return;
    const FTransform Transform = GetActorTransform();
    // Bind OnDestroyed BEFORE construction callbacks can produce overlap events.
    ACXPickupItem* Item = GetWorld()->SpawnActorDeferred<ACXPickupItem>(ItemClass, Transform, this,
        nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Item) return;
    SpawnedItem = Item;
    Item->OnDestroyed.AddDynamic(this, &ACXItemSpawner::OnSpawnedItemDestroyed);
    UGameplayStatics::FinishSpawningActor(Item, Transform);
}

void ACXItemSpawner::OnSpawnedItemDestroyed(AActor* DestroyedActor)
{
    if (DestroyedActor != SpawnedItem) return;
    SpawnedItem = nullptr;
    if (HasAuthority() && GetWorld())
    {
        GetWorldTimerManager().SetTimer(RespawnTimer, this, &ACXItemSpawner::SpawnItem,
            FMath::Max(0.1f, RespawnSeconds), false);
    }
}

void ACXItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(RespawnTimer);
    if (IsValid(SpawnedItem)) SpawnedItem->OnDestroyed.RemoveDynamic(this, &ACXItemSpawner::OnSpawnedItemDestroyed);
    Super::EndPlay(EndPlayReason);
}
