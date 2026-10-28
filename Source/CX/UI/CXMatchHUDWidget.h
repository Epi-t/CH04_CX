#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXGameState.h"
#include "CXMatchHUDWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * HUD for the coins, bonus max speed, live match countdown and match result.
 * Without a Widget Blueprint it creates a usable UI in C++ at runtime.
 * When subclassed in UMG, optional widgets below can be supplied in Designer
 * to override the default layout and still receive data from C++.
 * This HUD is display-only; clients must never edit server-side match data.
 */
UCLASS(Blueprintable)
class CX_API UCXMatchHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="CX|HUD")
    void UpdateVehicle(float SpeedKmh, int32 Gear);

    UFUNCTION(BlueprintCallable, Category="CX|HUD")
    void UpdatePlayerStats(int32 Coins, float BonusMaxSpeed);

    UFUNCTION(BlueprintCallable, Category="CX|HUD")
    void UpdateMatch(ECXGamePhase Phase, int32 RemainingSeconds);

    UFUNCTION(BlueprintCallable, Category="CX|HUD")
    void ShowResult(bool bWinner, bool bSharedFirstPlace, int32 FinalCoins);

    UFUNCTION(BlueprintCallable, Category="CX|HUD")
    void HideResult();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    // In a child Widget Blueprint, these names are optional (BindWidgetOptional).
    // For the fully C++-generated HUD, they are created automatically.
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_CurrentSpeed;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_Gear;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_Coins;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_BonusSpeed;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_GamePhase;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_Countdown;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UBorder> Panel_Result;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_ResultTitle;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="CX|HUD")
    TObjectPtr<UTextBlock> Text_ResultDetail;

private:
    void BuildDefaultLayout();
    static void SetText(UTextBlock* Target, const FString& Value);
    static FString FormatTime(int32 Seconds);
    static FString GetPhaseLabel(ECXGamePhase Phase);

    int32 LastCoins = MIN_int32;
    float LastBonusSpeed = TNumericLimits<float>::Max();
    int32 LastSpeedKmh = MIN_int32;
    int32 LastGear = MIN_int32;
    int32 LastRemainingTime = MIN_int32;
    ECXGamePhase LastPhase = ECXGamePhase::Finished;
    bool bResultShown = false;
};
