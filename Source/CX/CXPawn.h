// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "WheeledVehiclePawn.h"
#include "CXPawn.generated.h"

class UCameraComponent;
class UParticleSystem;
class ACXMine;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateVehicle, Log, All);

/**
 *  Vehicle Pawn class
 *  Handles common functionality for all vehicle types,
 *  including input handling and camera management.
 *  
 *  Specific vehicle configurations are handled in subclasses.
 */
UCLASS(abstract)
class ACXPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	/** Spring Arm for the front camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	/** Front Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	/** Spring Arm for the back camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	/** Back Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	/** Cast pointer to the Chaos Vehicle movement component */
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

protected:

	/** Steering Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* SteeringAction;

	/** Throttle Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ThrottleAction;

	/** Brake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* BrakeAction;

	/** Handbrake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* HandbrakeAction;

	/** Look Around Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAroundAction;

	/** Toggle Camera Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ToggleCameraAction;

	/** Reset Vehicle Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ResetVehicleAction;

    // BP: create IA_Attack (Digital/Bool, LMB) and IA_Mine (Digital/Bool, F) in the mapping context.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CX|Input") TObjectPtr<UInputAction> AttackAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CX|Input") TObjectPtr<UInputAction> MineAction;
    UPROPERTY(EditDefaultsOnly, Category="CX|Attack") FVector BumperTraceOffset = FVector(220.f, 0.f, 65.f);
    UPROPERTY(EditDefaultsOnly, Category="CX|Attack", meta=(ClampMin="1")) float AttackRange = 700.f;
    UPROPERTY(EditDefaultsOnly, Category="CX|Attack") TObjectPtr<UParticleSystem> HitParticle;
    UPROPERTY(EditDefaultsOnly, Category="CX|Mine") TSubclassOf<ACXMine> MineClass;
    UPROPERTY(EditDefaultsOnly, Category="CX|Mine") FVector MineSpawnOffset = FVector(-250.f, 0.f, 30.f);
    // BonusMaxSpeed is in km/h. Apply as a throttle ceiling rather than modifying tire/engine tuning assets.
    UPROPERTY(EditDefaultsOnly, Category="CX|Speed", meta=(ClampMin="20")) float BaseMaxSpeedKmh = 120.f;

    UPROPERTY(ReplicatedUsing=OnRep_ThrottleDisabled) bool bThrottleDisabled = false;
    FTimerHandle ThrottleRecoveryTimer;
    bool bHandbrakeHeld = false;
    bool bPreviousPlaying = false;
    bool IsPlayingPhase() const;
    bool IsThrottleBlocked() const;
    void UpdateGameInputLock();
    UFUNCTION() void OnRep_ThrottleDisabled();
    void RecoverThrottle();
    void Attack(const FInputActionValue& Value);
    void PlaceMine(const FInputActionValue& Value);

    UFUNCTION(Server, Reliable) void ServerAttack();
    UFUNCTION(Server, Reliable) void ServerPlaceMine();
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastHitEffect(FVector_NetQuantize Start, FVector_NetQuantize End,
        bool bHitVehicle, FVector_NetQuantize HitLocation, FVector_NetQuantizeNormal HitNormal);

    /** Keeps track of which camera is active */
	bool bFrontCameraActive = false;

public:
	ACXPawn();

	// Begin Pawn interface

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	// End Pawn interface

	// Begin Actor interface

    virtual void Tick(float Delta) override;
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UFUNCTION(BlueprintPure, Category="CX|Game") bool IsGamePlayable() const { return IsPlayingPhase(); }
    bool HasAttackAction() const { return AttackAction != nullptr; }
    bool HasMineAction() const { return MineAction != nullptr; }
    void RequestAttack() { if (IsPlayingPhase()) ServerAttack(); }
    void RequestMine() { if (IsPlayingPhase()) ServerPlaceMine(); }
    // Server is the authority on mine hit; client receives the replicated disable state.
    void DisableThrottleForSeconds(float Duration);

	// End Actor interface

protected:

	/** Handles steering input */
	void Steering(const FInputActionValue& Value);

	/** Handles throttle input */
	void Throttle(const FInputActionValue& Value);

	/** Handles brake input */
	void Brake(const FInputActionValue& Value);

	/** Handles brake start/stop inputs */
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	/** Handles handbrake start/stop inputs */
	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	/** Handles look around input */
	void LookAround(const FInputActionValue& Value);

	/** Handles toggle camera input */
	void ToggleCamera(const FInputActionValue& Value);

	/** Handles reset vehicle input */
	void ResetVehicle(const FInputActionValue& Value);

	/** Called when the brake lights are turned on or off */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

public:
	/** Returns the front spring arm subobject */
	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }
	/** Returns the front camera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }
	/** Returns the back spring arm subobject */
	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }
	/** Returns the back camera subobject */
	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }
	/** Returns the cast Chaos Vehicle Movement subobject */
	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
};
