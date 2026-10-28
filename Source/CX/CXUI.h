#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXGameState.h"
#include "CXUI.generated.h"

class UChatUI;

// Existing Blueprint vehicle HUD extended with optional Blueprint event hooks.
UCLASS(abstract)
class CX_API UCXUI : public UUserWidget
{
    GENERATED_BODY()
public:
    void OnNewChatMessage(const FString& InSender, const FString& InMessage);
    UChatUI* GetChatWidget() const { return UI_Chat.Get(); }
    void UpdateSpeed(float NewSpeed);
    void UpdateGear(int32 NewGear);
    void UpdatePlayerStats(int32 Coins, float BonusMaxSpeed);
    void UpdateGameStatus(ECXGamePhase Phase, int32 RemainingSeconds);
    void ShowGameResult(bool bWinner, bool bSharedFirstPlace, int32 Coins);

protected:
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UChatUI> UI_Chat;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vehicle") bool bIsMPH = false;

    UFUNCTION(BlueprintImplementableEvent, Category="Vehicle") void OnSpeedUpdate(float NewSpeed);
    UFUNCTION(BlueprintImplementableEvent, Category="Vehicle") void OnGearUpdate(int32 NewGear);
    // Implement these three events in the existing Vehicle UI Blueprint.
    UFUNCTION(BlueprintImplementableEvent, Category="CX|HUD") void OnPlayerStatsUpdate(int32 Coins, float BonusMaxSpeed);
    UFUNCTION(BlueprintImplementableEvent, Category="CX|HUD") void OnGameStatusUpdate(ECXGamePhase Phase, int32 RemainingSeconds);
    UFUNCTION(BlueprintImplementableEvent, Category="CX|HUD") void OnGameResultUpdate(bool bWinner, bool bSharedFirstPlace, int32 Coins);
};
