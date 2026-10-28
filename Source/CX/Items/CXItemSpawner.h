#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"

class ACXPickupItem;

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
    GENERATED_BODY()
public:
    ACXItemSpawner();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditAnywhere, Category="CX|ItemSpawner") TSubclassOf<ACXPickupItem> ItemClass;
    UPROPERTY(VisibleInstanceOnly, Category="CX|ItemSpawner") TObjectPtr<ACXPickupItem> SpawnedItem;
    UPROPERTY(EditAnywhere, Category="CX|ItemSpawner", meta=(ClampMin="0.1")) float RespawnSeconds = 2.f;
    UFUNCTION() void OnSpawnedItemDestroyed(AActor* DestroyedActor);
private:
    FTimerHandle RespawnTimer;
    void SpawnItem();
};
