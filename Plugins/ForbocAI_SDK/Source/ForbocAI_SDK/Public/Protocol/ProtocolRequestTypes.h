#pragma once

#include "CoreMinimal.h"
#include "Protocol/ProtocolTypes.h"
#include "ProtocolRequestTypes.generated.h"

USTRUCT(BlueprintType)
struct FNPCActorInfo {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString NpcId;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  FAgentState Data;
};

USTRUCT(BlueprintType)
struct FNPCProcessTape {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FString ContextJson;

  UPROPERTY(BlueprintReadOnly)
  FAgentState NpcState;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  bool bHasActor;

  UPROPERTY(BlueprintReadOnly)
  FNPCActorInfo Actor;

  UPROPERTY(BlueprintReadOnly)
  TArray<FRecalledMemory> Memories;

  UPROPERTY(BlueprintReadOnly)
  FString Prompt;

  UPROPERTY(BlueprintReadOnly)
  FCortexConfig Constraints;

  UPROPERTY(BlueprintReadOnly)
  FString GeneratedOutput;

  UPROPERTY(BlueprintReadOnly)
  FString RulesetId;

  UPROPERTY(BlueprintReadOnly)
  bool bVectorQueried;

  FNPCProcessTape() : bHasActor(false), bVectorQueried(false) {}
};

USTRUCT(BlueprintType)
struct FNPCProcessRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FNPCProcessTape Tape;

  UPROPERTY(BlueprintReadOnly)
  FString LastResultJson;

  UPROPERTY(BlueprintReadOnly)
  bool bHasLastResult;

  FNPCProcessRequest() : bHasLastResult(false) {}
};

USTRUCT(BlueprintType)
struct FNPCProcessResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FNPCInstruction Instruction;

  UPROPERTY(BlueprintReadOnly)
  FNPCProcessTape Tape;
};

USTRUCT(BlueprintType)
struct FDirectiveRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FAgentState NpcState;

  UPROPERTY(BlueprintReadOnly)
  FString ContextJson;
};

USTRUCT(BlueprintType)
struct FDirectiveResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FMemoryRecallInstruction MemoryRecall;
};

USTRUCT(BlueprintType)
struct FContextRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  TArray<FRecalledMemory> Memories;

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FAgentState NpcState;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;
};

USTRUCT(BlueprintType)
struct FContextResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Prompt;

  UPROPERTY(BlueprintReadOnly)
  FCortexConfig Constraints;
};

USTRUCT(BlueprintType)
struct FVerdictRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString GeneratedOutput;

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  FAgentState NpcState;
};

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
  FAgentState StateDelta;

  UPROPERTY(BlueprintReadOnly)
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly)
  bool bHasAction;

  UPROPERTY(BlueprintReadOnly)
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
