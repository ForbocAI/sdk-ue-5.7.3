#pragma once

#include "CoreMinimal.h"
#include "Memory/MemoryTypes.h"
#include "AgentTypes.generated.h"

/**
 * Agent State — Immutable data.
 */
USTRUCT(BlueprintType)
struct FAgentState {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString JsonData;

  FAgentState() : JsonData(TEXT("{}")) {}
};

/**
 * Agent Action — Immutable data.
 */
USTRUCT(BlueprintType)
struct FAgentAction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Type;

  UPROPERTY(BlueprintReadOnly)
  FString Target;

  UPROPERTY(BlueprintReadOnly)
  FString Reason;

  UPROPERTY(BlueprintReadOnly)
  float Confidence;

  UPROPERTY(BlueprintReadOnly)
  FString Signature;

  UPROPERTY(BlueprintReadOnly)
  FString PayloadJson;

  FAgentAction() : Confidence(1.0f) {}
};

/**
 * Agent Entity — Pure immutable data.
 */
USTRUCT(BlueprintType)
struct FAgent {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  FAgentState State;

  UPROPERTY(BlueprintReadOnly)
  TArray<FMemoryItem> Memories;

  UPROPERTY(BlueprintReadOnly)
  FString ApiUrl;

  FAgent() {}
};

/**
 * Agent Configuration — Plain data.
 */
USTRUCT(BlueprintType)
struct FAgentConfig {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  FString ApiUrl;

  UPROPERTY(BlueprintReadOnly)
  FAgentState InitialState;

  UPROPERTY(BlueprintReadOnly)
  FString ApiKey;
};

/**
 * Agent Response — Plain data.
 */
USTRUCT(BlueprintType)
struct FAgentResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Dialogue;

  UPROPERTY(BlueprintReadOnly)
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly)
  FString Thought;
};

/**
 * Imported NPC
 */
USTRUCT(BlueprintType)
struct FImportedNpc {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString NpcId;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  FString DataJson;

  FImportedNpc() {}
};

namespace TypeFactory {

inline FAgentState AgentState(FString JsonData) {
  FAgentState S;
  S.JsonData = MoveTemp(JsonData);
  return S;
}

inline FAgentAction Action(FString Type, FString Target,
                           FString Reason = TEXT("")) {
  FAgentAction A;
  A.Type = MoveTemp(Type);
  A.Target = MoveTemp(Target);
  A.Reason = MoveTemp(Reason);
  return A;
}

inline FImportedNpc ImportedNpc(FString NpcId, FString Persona,
                                FString DataJson = TEXT("{}")) {
  FImportedNpc N;
  N.NpcId = MoveTemp(NpcId);
  N.Persona = MoveTemp(Persona);
  N.DataJson = MoveTemp(DataJson);
  return N;
}

} // namespace TypeFactory
