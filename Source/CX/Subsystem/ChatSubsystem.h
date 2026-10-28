// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ChatSubsystem.generated.h"

USTRUCT()
struct FChatData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Sender;
	UPROPERTY()
	FString Message;

	FChatData() {}
	FChatData(const FString& InSender, const FString& InMessage)
		: Sender(InSender), Message(InMessage)
	{
	}
};

/**
 * 
 */
UCLASS()
class CX_API UChatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void AddChatMessage(const FString& InSender, const FString& InMessage);
	void SetChatLog(const TArray<FChatData>& InChatLog) { ChatLog = InChatLog; }
	void ClearChatLog() { ChatLog.Empty(); }

	const TArray<FChatData>& GetChatLog() const { return ChatLog; }

protected:
	TArray<FChatData> ChatLog;

	static inline int32 MaxChatLog = 10;
};
