#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "ChatUI.generated.h"

// Can be instanced directly: if there is no Widget Blueprint, a native chat UI is built.
UCLASS(Blueprintable)
class CX_API UChatUI : public UUserWidget
{
    GENERATED_BODY()
public:
    void OnNewChatMessage(const FString& InSender, const FString& InMessage);
    void FocusInput();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UScrollBox> ChatScrollBox;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMultiLineEditableTextBox> MultilineEditableTextBox_ChatLog;
private:
    void BuildDefaultLayout();
};
