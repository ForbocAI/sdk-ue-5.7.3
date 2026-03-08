#pragma once

#include "CoreMinimal.h"
#include "ProtocolRequestTypes.generated.h"
#include "Protocol/ProtocolTypes.h"

/**
 * NPC Process Request — Sent to the API for the next protocol step.
 */
USTRUCT(BlueprintType)
struct FNPCProcessRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString NpcId;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> Tape;

  UPROPERTY(BlueprintReadOnly)
  FString LastResult;

  FNPCProcessRequest() {}
};

/**
 * Directive Request
 */
USTRUCT(BlueprintType)
struct FDirectiveRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FString NpcStateJson; // Serialized NPCState

  FDirectiveRequest() {}
};

/**
 * Directive Response
 */
USTRUCT(BlueprintType)
struct FDirectiveResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FMemoryRecallInstruction MemoryRecall;

  FDirectiveResponse() {}
};

/**
 * Context Request
 */
USTRUCT(BlueprintType)
struct FContextRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  TArray<FRecalledMemory> Memories;

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FString NpcStateJson;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  FContextRequest() {}
};

/**
 * Context Response
 */
USTRUCT(BlueprintType)
struct FContextResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Prompt;

  UPROPERTY(BlueprintReadOnly)
  FCortexConfig Constraints;

  FContextResponse() {}
};

/**
 * Verdict Request
 */
USTRUCT(BlueprintType)
struct FVerdictRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString GeneratedOutput;

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FString NpcStateJson;

  FVerdictRequest() {}
};

/**
 * Verdict Response
 */
USTRUCT(BlueprintType)
struct FVerdictResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  bool bValid;

  UPROPERTY(BlueprintReadOnly)
  FString Signature;

  UPROPERTY(BlueprintReadOnly)
  TArray<FMemoryStoreInstruction> MemoryStore;

  UPROPERTY(BlueprintReadOnly)
  FString StateDeltaJson;

  UPROPERTY(BlueprintReadOnly)
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly)
  FString Dialogue;

  FVerdictResponse() : bValid(true) {}
};

namespace TypeFactory {

inline FDirectiveRequest DirectiveRequest(FString Observation,
                                          FString NpcStateJson) {
  FDirectiveRequest R;
  R.Observation = MoveTemp(Observation);
  R.NpcStateJson = MoveTemp(NpcStateJson);
  return R;
}

inline FContextRequest ContextRequest(TArray<FRecalledMemory> Memories,
                                      FString Observation, FString NpcStateJson,
                                      FString Persona) {
  FContextRequest R;
  R.Memories = MoveTemp(Memories);
  R.Observation = MoveTemp(Observation);
  R.NpcStateJson = MoveTemp(NpcStateJson);
  R.Persona = MoveTemp(Persona);
  return R;
}

inline FVerdictRequest VerdictRequest(FString GeneratedOutput,
                                      FString Observation,
                                      FString NpcStateJson) {
  FVerdictRequest R;
  R.GeneratedOutput = MoveTemp(GeneratedOutput);
  R.Observation = MoveTemp(Observation);
  R.NpcStateJson = MoveTemp(NpcStateJson);
  return R;
}

} // namespace TypeFactory
