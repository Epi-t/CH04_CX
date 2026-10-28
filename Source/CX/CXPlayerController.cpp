#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "UI/CXMatchHUDWidget.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void ACXPlayerController::STCChatLog_Implementation(const TArray<FChatData>& ChatLog)
{
    if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
    {
        Chat->SetChatLog(ChatLog);
        // If initial replication/UI already finished, flush immediately.
        if (UChatUI* UI = GetActiveChatUI())
        {
            for (const FChatData& Entry : Chat->GetChatLog())
                UI->OnNewChatMessage(Entry.Sender, Entry.Message);
            Chat->ClearChatLog();
        }
    }
}

void ACXPlayerController::CTSChatMessage_Implementation(const FString& Message)
{
    if (Message.TrimStartAndEnd().IsEmpty() || Message.Len() > 256 || !IsValid(PlayerState)) return;
    const FString Sender = PlayerState->GetPlayerName();
    if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
    {
        Chat->AddChatMessage(Sender, Message);
    }
    for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
    {
        It->STCChatMessage(Sender, Message);
    }
}

void ACXPlayerController::STCChatMessage_Implementation(const FString& Sender, const FString& Message)
{
    if (UChatUI* UI = GetActiveChatUI()) UI->OnNewChatMessage(Sender, Message);
    else if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
        Chat->AddChatMessage(Sender, Message); // avoid UI-not-ready null dereference
}

void ACXPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;

    // This widget is available immediately, without creating a Widget Blueprint.
    // A designer subclass can be assigned using MatchHUDClass in BP_CXPlayerController.
    TSubclassOf<UCXMatchHUDWidget> EffectiveMatchHUDClass = MatchHUDClass;
    if (!EffectiveMatchHUDClass) EffectiveMatchHUDClass = UCXMatchHUDWidget::StaticClass();
    MatchHUD = CreateWidget<UCXMatchHUDWidget>(this, EffectiveMatchHUDClass);
    if (IsValid(MatchHUD))
    {
        // Higher ZOrder than the vehicle HUD; all match UI is non-interactive.
        MatchHUD->AddToViewport(10);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create CXMatchHUDWidget."));
    }

    // Preserve the 2nd-lesson vehicle display/chat UI as an independent widget.
    if (VehicleUIClass)
    {
        VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);
        if (IsValid(VehicleUI)) VehicleUI->AddToViewport(0);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("VehicleUIClass is not set. CX match HUD works, but original chat/vehicle UI needs its BP."));
    }

    // W_VehicleAdvUI from the uploaded VehicleTemplate package predates lesson-2 chat.
    // Supply a fully native chat window if the original BP has no bound chat widget.
    if (!IsValid(VehicleUI) || !VehicleUI->GetChatWidget())
    {
        NativeChatUI = CreateWidget<UChatUI>(this, UChatUI::StaticClass());
        if (IsValid(NativeChatUI)) NativeChatUI->AddToViewport(12);
    }

    if (UChatUI* UI = GetActiveChatUI())
    {
        if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
        {
            for (const FChatData& Entry : Chat->GetChatLog())
            {
                UI->OnNewChatMessage(Entry.Sender, Entry.Message);
            }
            Chat->ClearChatLog();
        }
    }
}

void ACXPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent)
    {
        // These raw-key fallbacks work even without new IA_Attack / IA_Mine assets.
        InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACXPlayerController::OpenChatInput);
        InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ACXPlayerController::OpenChatInput);
        InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACXPlayerController::FallbackAttack);
        InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ACXPlayerController::FallbackMine);
    }
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InputMappingContext) Subsystem->AddMappingContext(InputMappingContext, 0);
        if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
            Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
    }
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXPlayerController, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!IsLocalController()) return;

    ACXPawn* Vehicle = IsValid(VehiclePawn) ? VehiclePawn.Get() : Cast<ACXPawn>(GetPawn());
    if (Vehicle && Vehicle->GetChaosVehicleMovement())
    {
        const float SpeedCmPerSec = Vehicle->GetChaosVehicleMovement()->GetForwardSpeed();
        const int32 Gear = Vehicle->GetChaosVehicleMovement()->GetCurrentGear();
        if (IsValid(VehicleUI))
        {
            VehicleUI->UpdateSpeed(SpeedCmPerSec);
            VehicleUI->UpdateGear(Gear);
        }
        if (IsValid(MatchHUD)) MatchHUD->UpdateVehicle(SpeedCmPerSec * 0.036f, Gear);
    }

    const ACXPlayerState* PS = GetPlayerState<ACXPlayerState>();
    if (PS)
    {
        if (IsValid(VehicleUI)) VehicleUI->UpdatePlayerStats(PS->GetCoins(), PS->GetBonusMaxSpeed());
        if (IsValid(MatchHUD)) MatchHUD->UpdatePlayerStats(PS->GetCoins(), PS->GetBonusMaxSpeed());
    }

    const ACXGameState* GS = GetWorld() ? GetWorld()->GetGameState<ACXGameState>() : nullptr;
    if (!GS) return;

    if (IsValid(VehicleUI)) VehicleUI->UpdateGameStatus(GS->GetGamePhase(), GS->GetRemainingTime());
    if (IsValid(MatchHUD)) MatchHUD->UpdateMatch(GS->GetGamePhase(), GS->GetRemainingTime());

    // Winners may replicate after the Finished phase. Keep checking until a result is available.
    if (GS->GetGamePhase() == ECXGamePhase::Finished && !bResultPresented &&
        PS && GS->GetWinners().Num() > 0)
    {
        const bool bWon = GS->IsWinner(PS);
        const bool bShared = bWon && GS->GetWinners().Num() > 1;
        if (IsValid(VehicleUI)) VehicleUI->ShowGameResult(bWon, bShared, PS->GetCoins());
        if (IsValid(MatchHUD)) MatchHUD->ShowResult(bWon, bShared, PS->GetCoins());
        bResultPresented = true;
    }
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    VehiclePawn = Cast<ACXPawn>(InPawn);
}


UChatUI* ACXPlayerController::GetActiveChatUI() const
{
    if (IsValid(NativeChatUI)) return NativeChatUI.Get();
    return IsValid(VehicleUI) ? VehicleUI->GetChatWidget() : nullptr;
}

void ACXPlayerController::OpenChatInput()
{
    if (!IsLocalController()) return;
    UChatUI* Chat = GetActiveChatUI();
    if (!Chat) return;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(Chat->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    bShowMouseCursor = true;
    Chat->FocusInput();
}

void ACXPlayerController::CloseChatInput()
{
    if (!IsLocalController()) return;
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
}

void ACXPlayerController::FallbackAttack()
{
    if (ACXPawn* Vehicle = Cast<ACXPawn>(GetPawn()))
        if (!Vehicle->HasAttackAction()) Vehicle->RequestAttack();
}

void ACXPlayerController::FallbackMine()
{
    if (ACXPawn* Vehicle = Cast<ACXPawn>(GetPawn()))
        if (!Vehicle->HasMineAction()) Vehicle->RequestMine();
}
