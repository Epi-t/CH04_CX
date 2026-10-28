#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXPickupItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;
class ACXPawn;

// Blueprint child classes may choose the mesh, particle, and sound.
UCLASS()
class CX_API ACXPickupItem : public AActor
{
    GENERATED_BODY()
public:
    ACXPickupItem();

protected:
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup") TObjectPtr<USphereComponent> PickupSphere;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup") TObjectPtr<UStaticMeshComponent> PickupMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup|FX") TObjectPtr<UParticleSystem> PickupParticle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup|FX") TObjectPtr<USoundBase> PickupSound;

    UFUNCTION() void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    virtual bool GiveReward(ACXPawn* Vehicle) { return false; }
};

UCLASS()
class CX_API ACXCoinItem : public ACXPickupItem
{
    GENERATED_BODY()
protected:
    virtual bool GiveReward(ACXPawn* Vehicle) override;
};

UCLASS()
class CX_API ACXSpeedItem : public ACXPickupItem
{
    GENERATED_BODY()
protected:
    virtual bool GiveReward(ACXPawn* Vehicle) override;
};
