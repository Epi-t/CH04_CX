#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMine.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;

UCLASS()
class CX_API ACXMine : public AActor
{
    GENERATED_BODY()
public:
    ACXMine();
protected:
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, Category="Mine") TObjectPtr<USphereComponent> TriggerSphere;
    UPROPERTY(VisibleAnywhere, Category="Mine") TObjectPtr<UStaticMeshComponent> MineMesh;
    UPROPERTY(EditDefaultsOnly, Category="Mine|FX") TObjectPtr<UParticleSystem> ExplosionParticle;
    UPROPERTY(EditDefaultsOnly, Category="Mine|FX") TObjectPtr<USoundBase> ExplosionSound;
    UFUNCTION() void OnMineOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);
};
