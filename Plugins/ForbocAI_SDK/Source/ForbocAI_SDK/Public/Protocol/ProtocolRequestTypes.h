#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Protocol/ProtocolTypes.h"
#include "ProtocolRequestTypes.generated.h"
// clang-format on

USTRUCT(BlueprintType)
struct FNPCActorInfo {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString NpcId;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentState Data;
};

USTRUCT(BlueprintType)
struct FNPCProcessTape {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Observation;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString ContextJson;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentState NpcState;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  bool bHasActor;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FNPCActorInfo Actor;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  TArray<FRecalledMemory> Memories;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Prompt;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FCortexConfig Constraints;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString GeneratedOutput;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString RulesetId;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  bool bVectorQueried;

  FNPCProcessTape() : bHasActor(false), bVectorQueried(false) {}
};

USTRUCT(BlueprintType)
struct FNPCProcessRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FNPCProcessTape Tape;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString LastResultJson;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  bool bHasLastResult;

  FNPCProcessRequest() : bHasLastResult(false) {}
};

USTRUCT(BlueprintType)
struct FNPCProcessResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FNPCInstruction Instruction;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FNPCProcessTape Tape;
};

USTRUCT(BlueprintType)
struct FDirectiveRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Observation;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentState NpcState;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString ContextJson;
};

USTRUCT(BlueprintType)
struct FDirectiveResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FMemoryRecallInstruction MemoryRecall;
};

USTRUCT(BlueprintType)
struct FContextRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  TArray<FRecalledMemory> Memories;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Observation;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentState NpcState;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Persona;
};

USTRUCT(BlueprintType)
struct FContextResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Prompt;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FCortexConfig Constraints;
};

USTRUCT(BlueprintType)
struct FVerdictRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString GeneratedOutput;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Observation;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentState NpcState;
};

USTRUCT(BlueprintType)
struct FVerdictResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  bool bValid;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Signature;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  TArray<FMemoryStoreInstruction> MemoryStore;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentState StateDelta;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  bool bHasAction;

  UPROPERTY(BlueprintReadOnly, Category = "Protocol")
  FString Dialogue;

  FVerdictResponse() : bValid(true), bHasAction(false) {}
};

namespace TypeFactory {

inline FNPCProcessTape ProcessTape(const FString &Observation,
                                   const FString &ContextJson,
                                   const FAgentState &NpcState,
                                   const FString &Persona) {
  FNPCProcessTape Tape;
  Tape.Observation = Observation;
  Tape.ContextJson = ContextJson;
  Tape.NpcState = NpcState;
  Tape.Persona = Persona;
  return Tape;
}

inline FDirectiveRequest DirectiveRequest(const FString &Observation,
                                          const FAgentState &NpcState,
                                          const FString &ContextJson) {
  FDirectiveRequest Request;
  Request.Observation = Observation;
  Request.NpcState = NpcState;
  Request.ContextJson = ContextJson;
  return Request;
}

inline FContextRequest ContextRequest(const TArray<FRecalledMemory> &Memories,
                                      const FString &Observation,
                                      const FAgentState &NpcState,
                                      const FString &Persona) {
  FContextRequest Request;
  Request.Memories = Memories;
  Request.Observation = Observation;
  Request.NpcState = NpcState;
  Request.Persona = Persona;
  return Request;
}

inline FVerdictRequest VerdictRequest(const FString &GeneratedOutput,
                                      const FString &Observation,
                                      const FAgentState &NpcState) {
  FVerdictRequest Request;
  Request.GeneratedOutput = GeneratedOutput;
  Request.Observation = Observation;
  Request.NpcState = NpcState;
  return Request;
}

} // namespace TypeFactory
