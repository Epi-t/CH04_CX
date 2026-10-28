#include "CXUI.h"
#include "UI/ChatUI.h"

void UCXUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
    if (IsValid(UI_Chat)) UI_Chat->OnNewChatMessage(InSender, InMessage);
}

void UCXUI::UpdateSpeed(float NewSpeed)
{
    OnSpeedUpdate(FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f));
}

void UCXUI::UpdateGear(int32 NewGear) { OnGearUpdate(NewGear); }
void UCXUI::UpdatePlayerStats(int32 Coins, float BonusMaxSpeed)
{
    OnPlayerStatsUpdate(Coins, BonusMaxSpeed);
}
void UCXUI::UpdateGameStatus(ECXGamePhase Phase, int32 RemainingSeconds)
{
    OnGameStatusUpdate(Phase, RemainingSeconds);
}
void UCXUI::ShowGameResult(bool bWinner, bool bSharedFirstPlace, int32 Coins)
{
    OnGameResultUpdate(bWinner, bSharedFirstPlace, Coins);
}
