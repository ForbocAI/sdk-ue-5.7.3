#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.generated.h" // UHT generated

// Ensure API macro is correct (UBT will define FORBOCAI_SDK_API)
// We only need to export types if used across modules.
// For now, removing FORBOCAI_API prefix from structs helps avoid linker errors
// if macro mismatch exists or use FORBOCAI_SDK_API if we want to be strict.

/**
 * Agent State (Immutable Data)
 */
USTRUCT()
struct FAgentState {
  GENERATED_BODY()

  UPROPERTY()
  FString Mood;

  UPROPERTY()
  TArray<FString> Inventory;

  UPROPERTY()
  TMap<FString, float> Skills;

  UPROPERTY()
  TMap<FString, float> Relationships;

  FAgentState() : Mood(TEXT("Neutral")) {}
  FAgentState(FString InMood) : Mood(InMood) {}
};

/**
 * Memory Item
 */
USTRUCT()
struct FMemoryItem {
  GENERATED_BODY()

  UPROPERTY()
  FString Id;

  UPROPERTY()
  FString Text;

  UPROPERTY()
  FString Type;

  UPROPERTY()
  float Importance;

  UPROPERTY()
  int64 Timestamp;

  FMemoryItem() : Importance(0.5f), Timestamp(0) {}
};

/**
 * Agent Action
 */
USTRUCT()
struct FAgentAction {
  GENERATED_BODY()

  UPROPERTY()
  FString Type;

  UPROPERTY()
  FString Target;

  UPROPERTY()
  FString Reason;

  UPROPERTY()
  FString PayloadJson;

  FAgentAction() {}
  FAgentAction(FString InType, FString InTarget, FString InReason = TEXT(""))
      : Type(InType), Target(InTarget), Reason(InReason) {}
};

/**
 * Validation Result
 */
USTRUCT()
struct FValidationResult {
  GENERATED_BODY()

  UPROPERTY()
  bool bValid;

  UPROPERTY()
  FString Reason;

  FValidationResult() : bValid(true) {}
  FValidationResult(bool bInValid, FString InReason)
      : bValid(bInValid), Reason(InReason) {}
};

/**
 * Soul (Portable Identity)
 */
USTRUCT()
struct FSoul {
  GENERATED_BODY()

  UPROPERTY()
  FString Id;

  UPROPERTY()
  FString Version;

  UPROPERTY()
  FString Name;

  UPROPERTY()
  FString Persona;

  UPROPERTY()
  FAgentState State;

  UPROPERTY()
  TArray<FMemoryItem> Memories;

  FSoul() {}
};

/**
 * Agent Response
 */
struct FAgentResponse {
  FString Dialogue;
  FAgentAction Action;
  FString Thought;
};

// Configuration for factory
struct FAgentConfig {
  FString Persona;
  FString ApiUrl;
  FAgentState InitialState;
};
