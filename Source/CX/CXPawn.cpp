// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "Items/CXMine.h"
#include "GameFramework/Controller.h"
#include "Engine/DamageEvents.h"  // FDamageEvent, FPointDamageEvent
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Particles/ParticleSystem.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
    bReplicates = true;
    MineClass = ACXMine::StaticClass();
    SetReplicateMovement(true);
    // construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

        // reset the vehicle
        EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
        if (AttackAction)
            EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::Attack);
        if (MineAction)
            EnhancedInputComponent->BindAction(MineAction, ETriggerEvent::Started, this, &ACXPawn::PlaceMine);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

    BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
    UpdateGameInputLock();
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(IsPlayingPhase() ? SteeringValue : 0.f);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	const float SpeedKmh = FMath::Abs(ChaosVehicleMovement->GetForwardSpeed()) * 0.036f;
    float Bonus = 0.f;
    if (const AController* PC = GetController())
    {
        if (const ACXPlayerState* PS = PC->GetPlayerState<ACXPlayerState>()) Bonus = PS->GetBonusMaxSpeed();
    }
    const bool bAtSpeedLimit = SpeedKmh >= FMath::Max(20.f, BaseMaxSpeedKmh + Bonus);
    ChaosVehicleMovement->SetThrottleInput((!IsPlayingPhase() || IsThrottleBlocked() ||
        (bAtSpeedLimit && ThrottleValue > 0.f)) ? 0.f : ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	bHandbrakeHeld = true;
    ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	bHandbrakeHeld = false;
    ChaosVehicleMovement->SetHandbrakeInput(!IsPlayingPhase());

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
    if (!IsPlayingPhase()) return;
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}


bool ACXPawn::IsPlayingPhase() const
{
    const UWorld* World = GetWorld();
    const ACXGameState* GS = World ? World->GetGameState<ACXGameState>() : nullptr;
    return GS && GS->IsPlaying();
}

bool ACXPawn::IsThrottleBlocked() const { return bThrottleDisabled; }

void ACXPawn::UpdateGameInputLock()
{
    const bool bPlaying = IsPlayingPhase();
    // This applies on the owning client AND on the authoritative vehicle instance.
    if (!bPlaying)
    {
        ChaosVehicleMovement->SetThrottleInput(0.f);
        ChaosVehicleMovement->SetSteeringInput(0.f);
        ChaosVehicleMovement->SetHandbrakeInput(true);
    }
    else if (!bPreviousPlaying)
    {
        ChaosVehicleMovement->SetHandbrakeInput(bHandbrakeHeld);
    }
    if (bThrottleDisabled) ChaosVehicleMovement->SetThrottleInput(0.f);
    bPreviousPlaying = bPlaying;
}

void ACXPawn::OnRep_ThrottleDisabled()
{
    if (bThrottleDisabled && ChaosVehicleMovement)
        ChaosVehicleMovement->SetThrottleInput(0.f);
}

void ACXPawn::DisableThrottleForSeconds(float Duration)
{
    if (!HasAuthority()) return;
    bThrottleDisabled = true;
    ForceNetUpdate();
    OnRep_ThrottleDisabled();
    GetWorldTimerManager().SetTimer(ThrottleRecoveryTimer, this,
        &ACXPawn::RecoverThrottle, FMath::Max(0.1f, Duration), false);
}

void ACXPawn::RecoverThrottle()
{
    if (!HasAuthority()) return;
    bThrottleDisabled = false;
    ForceNetUpdate();
}

void ACXPawn::Attack(const FInputActionValue& Value)
{
    if (IsPlayingPhase()) ServerAttack();
}

void ACXPawn::PlaceMine(const FInputActionValue& Value)
{
    if (IsPlayingPhase()) ServerPlaceMine();
}

void ACXPawn::ServerAttack_Implementation()
{
    if (!IsPlayingPhase() || !GetController()) return;
    const FVector Start = GetActorTransform().TransformPosition(BumperTraceOffset);
    const FVector End = Start + GetActorForwardVector() * FMath::Max(1.f, AttackRange);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CXVehicleAttack), false, this);
    Params.AddIgnoredActor(this);
    FCollisionObjectQueryParams ObjectTypes;
    ObjectTypes.AddObjectTypesToQuery(ECC_Vehicle);
    FHitResult Hit;
    const bool bBlockingHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectTypes, Params);
    ACXPawn* Victim = bBlockingHit ? Cast<ACXPawn>(Hit.GetActor()) : nullptr;
    const bool bHitVehicle = Victim && Victim != this;
    if (bHitVehicle)
    {
        // PointDamage carries the bone name for the optional wheel-damage challenge.
        UGameplayStatics::ApplyPointDamage(Victim, 5.f, GetActorForwardVector(), Hit,
            GetController(), this, nullptr);
    }
    MulticastHitEffect(Start, bBlockingHit ? Hit.ImpactPoint : End, bHitVehicle,
        bBlockingHit ? Hit.ImpactPoint : FVector::ZeroVector,
        bBlockingHit ? Hit.ImpactNormal : FVector::UpVector);
}

void ACXPawn::MulticastHitEffect_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End,
    bool bHitVehicle, FVector_NetQuantize HitLocation, FVector_NetQuantizeNormal HitNormal)
{
    if (!GetWorld()) return;
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 3.f);
    if (bHitVehicle && HitParticle)
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticle, HitLocation, HitNormal.Rotation());
}

float ACXPawn::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (!HasAuthority() || !IsPlayingPhase() || DamageAmount <= 0.f ||
        !EventInstigator || EventInstigator == GetController()) return 0.f;

    ACXPlayerState* Victim = GetController() ? GetController()->GetPlayerState<ACXPlayerState>() : nullptr;
    ACXPlayerState* Attacker = EventInstigator->GetPlayerState<ACXPlayerState>();
    if (!Victim || !Attacker) return 0.f;
    if (Victim->TrySpendCoins(5)) Attacker->AddCoins(5);

    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& Point = static_cast<const FPointDamageEvent&>(DamageEvent);
        // The sample car uses Phys_Wheel_FL / FR / BL / BR for wheel bones.
        if (Point.HitInfo.BoneName.ToString().Contains(TEXT("Wheel"), ESearchCase::IgnoreCase))
            Victim->ApplyWheelPenalty(20.f);
    }
    return Applied;
}

void ACXPawn::ServerPlaceMine_Implementation()
{
    if (!IsPlayingPhase() || !MineClass || !GetWorld()) return;
    const FVector Location = GetActorTransform().TransformPosition(MineSpawnOffset);
    const FTransform SpawnTransform(GetActorRotation(), Location);
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.Instigator = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<ACXMine>(MineClass, SpawnTransform, Params);
}

void ACXPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXPawn, bThrottleDisabled);
}

void ACXPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ThrottleRecoveryTimer);
    Super::EndPlay(Reason);
}

#undef LOCTEXT_NAMESPACE