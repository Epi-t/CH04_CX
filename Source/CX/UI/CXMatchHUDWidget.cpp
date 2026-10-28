#include "UI/CXMatchHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateColor.h"

namespace
{
    UTextBlock* MakeText(UWidgetTree* Tree, const FName& Name, int32 FontSize,
                        const FLinearColor& Color)
    {
        UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = FontSize;
        Label->SetFont(Font);
        Label->SetColorAndOpacity(FSlateColor(Color));
        Label->SetAutoWrapText(false);
        return Label;
    }

    void AddText(UVerticalBox* Parent, UTextBlock* Label)
    {
        if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Label))
        {
            Slot->SetPadding(FMargin(0.f, 2.f));
            Slot->SetHorizontalAlignment(HAlign_Center);
        }
    }

    UBorder* MakePanel(UWidgetTree* Tree, const FName& Name, const FLinearColor& Color,
                       float HorizontalPadding = 18.f, float VerticalPadding = 10.f)
    {
        UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Panel->SetBrushColor(Color);
        Panel->SetPadding(FMargin(HorizontalPadding, VerticalPadding));
        Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        return Panel;
    }

    void PlaceAt(UCanvasPanel* Root, UWidget* Widget, const FAnchors& Anchors,
                 const FVector2D& Alignment, const FVector2D& Position)
    {
        UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
        Slot->SetAnchors(Anchors);
        Slot->SetAlignment(Alignment);
        Slot->SetPosition(Position);
        Slot->SetAutoSize(true);
    }
}

void UCXMatchHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Build the native tree before UMG creates its Slate widget hierarchy.
    // An existing Blueprint designer hierarchy is never overwritten.
    if (WidgetTree && !WidgetTree->RootWidget) BuildDefaultLayout();
}

void UCXMatchHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    LastCoins = MIN_int32;
    LastBonusSpeed = TNumericLimits<float>::Max();
    LastSpeedKmh = MIN_int32;
    LastGear = MIN_int32;
    LastRemainingTime = MIN_int32;
    LastPhase = ECXGamePhase::Finished;
    HideResult();
    UpdateVehicle(0.f, 0);
    UpdatePlayerStats(0, 0.f);
    UpdateMatch(ECXGamePhase::Waiting, 0);
}

void UCXMatchHUDWidget::BuildDefaultLayout()
{
    if (!WidgetTree) return;

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootHUD"));
    Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = Root;

    const FLinearColor PanelColor(0.025f, 0.045f, 0.075f, 0.82f);
    const FLinearColor White(0.97f, 0.98f, 1.0f, 1.0f);
    const FLinearColor Mint(0.35f, 0.95f, 0.78f, 1.0f);
    const FLinearColor Light(0.73f, 0.81f, 0.93f, 1.0f);

    // Upper-left: current vehicle speed and gear.
    UBorder* VehiclePanel = MakePanel(WidgetTree, TEXT("VehiclePanel"), PanelColor);
    UVerticalBox* VehicleStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VehicleStack"));
    VehiclePanel->SetContent(VehicleStack);
    Text_CurrentSpeed = MakeText(WidgetTree, TEXT("Text_CurrentSpeed"), 28, White);
    Text_Gear = MakeText(WidgetTree, TEXT("Text_Gear"), 14, Light);
    AddText(VehicleStack, Text_CurrentSpeed);
    AddText(VehicleStack, Text_Gear);
    PlaceAt(Root, VehiclePanel, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(28.f, 25.f));

    // Upper-center: shared match state and server-side remaining time.
    UBorder* PhasePanel = MakePanel(WidgetTree, TEXT("PhasePanel"), PanelColor, 24.f, 12.f);
    UVerticalBox* PhaseStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PhaseStack"));
    PhasePanel->SetContent(PhaseStack);
    Text_GamePhase = MakeText(WidgetTree, TEXT("Text_GamePhase"), 19, Mint);
    Text_Countdown = MakeText(WidgetTree, TEXT("Text_Countdown"), 32, White);
    AddText(PhaseStack, Text_GamePhase);
    AddText(PhaseStack, Text_Countdown);
    PlaceAt(Root, PhasePanel, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 23.f));

    // Upper-right: only the local player's owner-replicated statistics.
    UBorder* StatsPanel = MakePanel(WidgetTree, TEXT("StatsPanel"), PanelColor, 20.f, 12.f);
    UVerticalBox* StatsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatsStack"));
    StatsPanel->SetContent(StatsStack);
    Text_Coins = MakeText(WidgetTree, TEXT("Text_Coins"), 25, Mint);
    Text_BonusSpeed = MakeText(WidgetTree, TEXT("Text_BonusSpeed"), 15, White);
    AddText(StatsStack, Text_Coins);
    AddText(StatsStack, Text_BonusSpeed);
    PlaceAt(Root, StatsPanel, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-28.f, 25.f));

    // Screen-center: hidden until GameState is Finished and Winners replicate.
    Panel_Result = MakePanel(WidgetTree, TEXT("Panel_Result"),
        FLinearColor(0.02f, 0.05f, 0.10f, 0.94f), 45.f, 35.f);
    UVerticalBox* ResultStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResultStack"));
    Panel_Result->SetContent(ResultStack);
    Text_ResultTitle = MakeText(WidgetTree, TEXT("Text_ResultTitle"), 39, Mint);
    Text_ResultDetail = MakeText(WidgetTree, TEXT("Text_ResultDetail"), 20, White);
    AddText(ResultStack, Text_ResultTitle);
    AddText(ResultStack, Text_ResultDetail);
    PlaceAt(Root, Panel_Result, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

void UCXMatchHUDWidget::SetText(UTextBlock* Target, const FString& Value)
{
    if (IsValid(Target)) Target->SetText(FText::FromString(Value));
}

FString UCXMatchHUDWidget::FormatTime(int32 Seconds)
{
    const int32 SafeSeconds = FMath::Max(0, Seconds);
    return FString::Printf(TEXT("%02d:%02d"), SafeSeconds / 60, SafeSeconds % 60);
}

FString UCXMatchHUDWidget::GetPhaseLabel(ECXGamePhase Phase)
{
    switch (Phase)
    {
    case ECXGamePhase::Waiting:  return TEXT("대기 중");
    case ECXGamePhase::Playing:  return TEXT("게임 진행 중");
    case ECXGamePhase::Finished: return TEXT("게임 종료");
    default:                     return TEXT("상태 확인 중");
    }
}

void UCXMatchHUDWidget::UpdateVehicle(float SpeedKmh, int32 Gear)
{
    const int32 RoundedSpeed = FMath::RoundToInt(FMath::Abs(SpeedKmh));
    if (RoundedSpeed != LastSpeedKmh)
    {
        LastSpeedKmh = RoundedSpeed;
        SetText(Text_CurrentSpeed, FString::Printf(TEXT("%d km/h"), RoundedSpeed));
    }
    if (Gear != LastGear)
    {
        LastGear = Gear;
        const FString GearLabel = Gear < 0 ? TEXT("R") : (Gear == 0 ? TEXT("N") : FString::FromInt(Gear));
        SetText(Text_Gear, FString::Printf(TEXT("기어  %s"), *GearLabel));
    }
}

void UCXMatchHUDWidget::UpdatePlayerStats(int32 Coins, float BonusMaxSpeed)
{
    if (Coins != LastCoins)
    {
        LastCoins = Coins;
        SetText(Text_Coins, FString::Printf(TEXT("코인  %d 원"), Coins));
    }
    if (!FMath::IsNearlyEqual(BonusMaxSpeed, LastBonusSpeed))
    {
        LastBonusSpeed = BonusMaxSpeed;
        SetText(Text_BonusSpeed, FString::Printf(TEXT("최대 속도 보너스  %+.0f km/h"), BonusMaxSpeed));
    }
}

void UCXMatchHUDWidget::UpdateMatch(ECXGamePhase Phase, int32 RemainingSeconds)
{
    if (Phase != LastPhase)
    {
        LastPhase = Phase;
        SetText(Text_GamePhase, GetPhaseLabel(Phase));
    }
    if (RemainingSeconds != LastRemainingTime)
    {
        LastRemainingTime = RemainingSeconds;
        SetText(Text_Countdown, FormatTime(RemainingSeconds));
    }
}

void UCXMatchHUDWidget::ShowResult(bool bWinner, bool bSharedFirstPlace, int32 FinalCoins)
{
    if (bResultShown) return;
    bResultShown = true;
    SetText(Text_ResultTitle, !bWinner ? TEXT("패배") :
        (bSharedFirstPlace ? TEXT("공동 우승!") : TEXT("승리!")));
    SetText(Text_ResultDetail, FString::Printf(TEXT("최종 코인  %d 원"), FinalCoins));
    if (IsValid(Panel_Result)) Panel_Result->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCXMatchHUDWidget::HideResult()
{
    bResultShown = false;
    if (IsValid(Panel_Result)) Panel_Result->SetVisibility(ESlateVisibility::Collapsed);
}
