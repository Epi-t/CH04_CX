#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "CXSportsCar.h"
#include "UObject/ConstructorHelpers.h"
#include "Subsystem/ChatSubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

ACXGameMode::ACXGameMode()
{
    PlayerControllerClass = ACXPlayerController::StaticClass();
    PlayerStateClass = ACXPlayerState::StaticClass();
    GameStateClass = ACXGameState::StaticClass();
    DefaultPawnClass = ACXSportsCar::StaticClass();

    // These are the exact paths in the user's supplied VehicleTemplate.zip.
    // Using the existing BP classes retains the car mesh, wheel tuning and input maps.
    static ConstructorHelpers::FClassFinder<APawn> SportsCarBP(
        TEXT("/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn"));
    if (SportsCarBP.Class && SportsCarBP.Class->IsChildOf(ACXSportsCar::StaticClass()))
        DefaultPawnClass = SportsCarBP.Class;

    static ConstructorHelpers::FClassFinder<APlayerController> VehiclePCBP(
        TEXT("/Game/VehicleTemplate/Blueprints/BP_VehicleAdvPlayerController"));
    if (VehiclePCBP.Class && VehiclePCBP.Class->IsChildOf(ACXPlayerController::StaticClass()))
        PlayerControllerClass = VehiclePCBP.Class;
}

ACXGameState* ACXGameMode::GetCXGameState() const
{
    return GetGameState<ACXGameState>();
}

void ACXGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (ACXGameState* GS = GetCXGameState())
    {
        GS->SetPhase(ECXGamePhase::Waiting);
        GS->SetRemainingTime(WaitingSeconds);
    }
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (!IsValid(NewPlayer)) return;

    // Keep the original lesson-2 chat history feature.
    if (ACXPlayerController* CXPC = Cast<ACXPlayerController>(NewPlayer))
    {
        if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
        {
            const TArray<FChatData>& History = Chat->GetChatLog();
            if (!History.IsEmpty()) CXPC->STCChatLog(History);
        }
    }

    if (!bCountdownStarted)
    {
        bCountdownStarted = true;
        GetWorldTimerManager().SetTimer(CountdownHandle, this,
            &ACXGameMode::OnCountdownTick, 1.f, true);
    }
}

void ACXGameMode::OnCountdownTick()
{
    ACXGameState* GS = GetCXGameState();
    if (!IsValid(GS)) return;
    const int32 Next = FMath::Max(0, GS->GetRemainingTime() - 1);
    GS->SetRemainingTime(Next);
    if (Next != 0) return;

    GetWorldTimerManager().ClearTimer(CountdownHandle);
    if (GS->GetGamePhase() == ECXGamePhase::Waiting) StartPlaying();
    else if (GS->GetGamePhase() == ECXGamePhase::Playing) FinishMatch();
}

void ACXGameMode::StartPlaying()
{
    ACXGameState* GS = GetCXGameState();
    if (!GS) return;
    GS->SetPhase(ECXGamePhase::Playing);
    GS->SetRemainingTime(PlayingSeconds);
    GetWorldTimerManager().SetTimer(CountdownHandle, this,
        &ACXGameMode::OnCountdownTick, 1.f, true);
}

void ACXGameMode::FinishMatch()
{
    ACXGameState* GS = GetCXGameState();
    if (!GS) return;

    TArray<ACXPlayerState*> Leaders;
    int32 MaxCoins = MIN_int32;
    for (APlayerState* BaseState : GS->PlayerArray)
    {
        ACXPlayerState* PS = Cast<ACXPlayerState>(BaseState);
        if (!PS) continue;
        const int32 Value = PS->GetCoins(); // authoritative: owner-only values are NOT read from other clients
        if (Value > MaxCoins)
        {
            MaxCoins = Value;
            Leaders.Reset();
            Leaders.Add(PS);
        }
        else if (Value == MaxCoins)
        {
            Leaders.Add(PS); // tied players share first place
        }
    }
    GS->SetWinners(Leaders);
    GS->SetRemainingTime(0);
    GS->SetPhase(ECXGamePhase::Finished);
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    TArray<APlayerStart*> Available;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        if (IsPlayerStartAvailable(*It)) Available.Add(*It);
    }
    // Preserve robust fallback when vehicles cover every start location.
    if (Available.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("No unoccupied Vehicle PlayerStart; using engine fallback."));
        return Super::ChoosePlayerStart_Implementation(Player);
    }
    return Available[FMath::RandHelper(Available.Num())];
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* Start) const
{
    if (!IsValid(Start) || !GetWorld()) return false;
    const FTransform Transform = Start->GetActorTransform();
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Vehicle);
    return !GetWorld()->OverlapAnyTestByObjectType(
        Transform.TransformPosition(VehicleSpawnCheckOffset), Transform.GetRotation(), Objects,
        FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent),
        FCollisionQueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false));
}
