#pragma once

#include "CoreMinimal.h"
#include "Memory/MemoryTypes.h"
#include "NPCBaseTypes.generated.h"

/**
 * NPC State — Immutable JSON data snapshot.
 *
 * NOTE: Struct names retain the FAgent* prefix for Blueprint compatibility.
 * The file was renamed from AgentTypes.h as part of the Agent→NPC terminology
 * migration. Include "NPC/NPCBaseTypes.h" in new code.
 */
USTRUCT(BlueprintType)
struct FAgentState {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString JsonData;

  FAgentState() : JsonData(TEXT("{}")) {}
};

/**
 * NPC Action — Immutable action directive.
 */
USTRUCT(BlueprintType)
struct FAgentAction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Type;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Target;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Reason;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  float Confidence;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Signature;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString PayloadJson;

  FAgentAction() : Confidence(1.0f) {}
};

/**
 * NPC Entity — Pure immutable data.
 */
USTRUCT(BlueprintType)
struct FAgent {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FAgentState State;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  TArray<FMemoryItem> Memories;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString ApiUrl;

  FAgent() {}
};

/**
 * NPC Configuration — Plain data.
 */
USTRUCT(BlueprintType)
struct FAgentConfig {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString ApiUrl;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FAgentState InitialState;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString ApiKey;
};

/**
 * NPC Response — Plain data.
 */
USTRUCT(BlueprintType)
struct FAgentResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Dialogue;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Thought;
};

/**
 * Imported NPC — Deserialized soul data.
 */
USTRUCT(BlueprintType)
struct FImportedNpc {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString NpcId;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
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
