#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/ChatSubsystem.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class ACXPawn;
class UCXUI;
class UChatUI;
class UCXMatchHUDWidget;

UCLASS()
class CX_API ACXPlayerController : public APlayerController
{
    GENERATED_BODY()
    friend class UChatUI;
    friend class ACXGameMode;

protected:
    UFUNCTION(Client, Reliable) void STCChatLog(const TArray<FChatData>& ChatLog);
    UFUNCTION(Server, Reliable) void CTSChatMessage(const FString& Message);
    UFUNCTION(Client, Reliable) void STCChatMessage(const FString& Sender, const FString& Message);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input) TObjectPtr<UInputMappingContext> InputMappingContext;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input) bool bUseSteeringWheelControls = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(EditCondition="bUseSteeringWheelControls"))
    TObjectPtr<UInputMappingContext> SteeringWheelInputMappingContext;
    UPROPERTY(Replicated) TObjectPtr<ACXPawn> VehiclePawn;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=UI) TSubclassOf<UCXUI> VehicleUIClass;
    UPROPERTY(Transient) TObjectPtr<UCXUI> VehicleUI;
    UPROPERTY(Transient) TObjectPtr<UChatUI> NativeChatUI;
    UChatUI* GetActiveChatUI() const;
    void OpenChatInput();
    void CloseChatInput();
    void FallbackAttack();
    void FallbackMine();

    // New 3rd/4th lesson HUD. If no BP is assigned, its C++ layout renders automatically.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CX|UI")
    TSubclassOf<UCXMatchHUDWidget> MatchHUDClass;
    UPROPERTY(Transient) TObjectPtr<UCXMatchHUDWidget> MatchHUD;

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnPossess(APawn* InPawn) override;
public:
    virtual void Tick(float Delta) override;
private:
    bool bResultPresented = false;
};
