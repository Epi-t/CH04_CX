#include "UI/ChatUI.h"
#include "CXPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UChatUI::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree && !WidgetTree->RootWidget) BuildDefaultLayout();
}

void UChatUI::BuildDefaultLayout()
{
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChatRoot"));
    WidgetTree->RootWidget = Canvas;

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatBackground"));
    Background->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.72f));
    Background->SetPadding(FMargin(10.f));
    UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Background);
    CanvasSlot->SetAnchors(FAnchors(0.f, 1.f));
    CanvasSlot->SetAlignment(FVector2D(0.f, 1.f));
    CanvasSlot->SetPosition(FVector2D(24.f, -30.f));
    CanvasSlot->SetSize(FVector2D(410.f, 200.f));

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChatStack"));
    Background->SetContent(Stack);

    ChatScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatScrollBox"));
    UVerticalBoxSlot* ScrollSlot = Stack->AddChildToVerticalBox(ChatScrollBox);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

    MultilineEditableTextBox_ChatLog = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
        UMultiLineEditableTextBox::StaticClass(), TEXT("MultilineEditableTextBox_ChatLog"));
    MultilineEditableTextBox_ChatLog->SetIsReadOnly(true);
    MultilineEditableTextBox_ChatLog->SetText(FText::FromString(TEXT("채팅: Enter 또는 T키로 입력")));
    ChatScrollBox->AddChild(MultilineEditableTextBox_ChatLog);

    EditableTextBox_ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>(
        UEditableTextBox::StaticClass(), TEXT("EditableTextBox_ChatInput"));
    EditableTextBox_ChatInput->SetHintText(FText::FromString(TEXT("Enter 또는 T키로 채팅 시작")));
    Stack->AddChildToVerticalBox(EditableTextBox_ChatInput);
}

void UChatUI::NativeConstruct()
{
    Super::NativeConstruct();
    if (EditableTextBox_ChatInput &&
        !EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UChatUI::OnChatInputTextCommitted))
    {
        EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &UChatUI::OnChatInputTextCommitted);
    }
}

void UChatUI::NativeDestruct()
{
    if (EditableTextBox_ChatInput)
        EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UChatUI::OnChatInputTextCommitted);
    Super::NativeDestruct();
}

void UChatUI::FocusInput()
{
    if (EditableTextBox_ChatInput) EditableTextBox_ChatInput->SetKeyboardFocus();
}

void UChatUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
    if (!MultilineEditableTextBox_ChatLog) return;
    FString Text = MultilineEditableTextBox_ChatLog->GetText().ToString();
    if (!Text.IsEmpty()) Text += TEXT("\n");
    Text += InSender + TEXT(": ") + InMessage;
    MultilineEditableTextBox_ChatLog->SetText(FText::FromString(Text));
    if (ChatScrollBox) ChatScrollBox->ScrollToEnd();
}

void UChatUI::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type Method)
{
    if (Method != ETextCommit::OnEnter) return;
    if (ACXPlayerController* PC = GetOwningPlayer<ACXPlayerController>())
    {
        const FString Chat = Text.ToString().TrimStartAndEnd();
        if (!Chat.IsEmpty() && Chat.Len() <= 256) PC->CTSChatMessage(Chat);
        if (EditableTextBox_ChatInput) EditableTextBox_ChatInput->SetText(FText::GetEmpty());
        PC->CloseChatInput();
    }
}
