#pragma once

#include "CoreMinimal.h"
#include "Cortex/CortexTypes.h"
#include "Memory/MemoryTypes.h"
#include "NPC/AgentTypes.h"
#include "ProtocolTypes.generated.h"

UENUM(BlueprintType)
enum class EDirectiveStatus : uint8 { Running, Completed, Failed };

USTRUCT(BlueprintType)
struct FDirectiveRun {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString NpcId;

  UPROPERTY(BlueprintReadOnly)
  FString Observation;

  UPROPERTY(BlueprintReadOnly)
  EDirectiveStatus Status;

  UPROPERTY(BlueprintReadOnly)
  int64 StartedAt;

  UPROPERTY(BlueprintReadOnly)
  int64 CompletedAt;

  UPROPERTY(BlueprintReadOnly)
  FString Error;

  UPROPERTY(BlueprintReadOnly)
  FString MemoryRecallQuery;

  UPROPERTY(BlueprintReadOnly)
  int32 MemoryRecallLimit;

  UPROPERTY(BlueprintReadOnly)
  float MemoryRecallThreshold;

  UPROPERTY(BlueprintReadOnly)
  FString ContextPrompt;

  UPROPERTY(BlueprintReadOnly)
  FCortexConfig ContextConstraints;

  UPROPERTY(BlueprintReadOnly)
  bool bVerdictValid;

  UPROPERTY(BlueprintReadOnly)
  FString VerdictDialogue;

  UPROPERTY(BlueprintReadOnly)
  FString VerdictActionType;

  FDirectiveRun()
      : Status(EDirectiveStatus::Running), StartedAt(0), CompletedAt(0),
        MemoryRecallLimit(0), MemoryRecallThreshold(0.0f), bVerdictValid(false) {
  }
};

USTRUCT(BlueprintType)
struct FMemoryRecallInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Query;

  UPROPERTY(BlueprintReadOnly)
  int32 Limit;

  UPROPERTY(BlueprintReadOnly)
  float Threshold;

  FMemoryRecallInstruction() : Limit(5), Threshold(0.7f) {}
};

USTRUCT(BlueprintType)
struct FMemoryStoreInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Text;

  UPROPERTY(BlueprintReadOnly)
  FString Type;

  UPROPERTY(BlueprintReadOnly)
  float Importance;

  FMemoryStoreInstruction() : Importance(0.5f) {}
};

USTRUCT(BlueprintType)
struct FRecalledMemory {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Text;

  UPROPERTY(BlueprintReadOnly)
  FString Type;

  UPROPERTY(BlueprintReadOnly)
  float Importance;

  UPROPERTY(BlueprintReadOnly)
  float Similarity;

  FRecalledMemory() : Importance(0.5f), Similarity(0.0f) {}
};

UENUM(BlueprintType)
enum class ENPCInstructionType : uint8 {
  IdentifyActor,
  QueryVector,
  ExecuteInference,
  Finalize
};

USTRUCT(BlueprintType)
struct FNPCInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  ENPCInstructionType Type;

  UPROPERTY(BlueprintReadOnly)
  FString Query;

  UPROPERTY(BlueprintReadOnly)
  int32 Limit;

  UPROPERTY(BlueprintReadOnly)
  float Threshold;

  UPROPERTY(BlueprintReadOnly)
  FString Prompt;

  UPROPERTY(BlueprintReadOnly)
  FCortexConfig Constraints;

  UPROPERTY(BlueprintReadOnly)
  bool bValid;

  UPROPERTY(BlueprintReadOnly)
  FString Signature;

  UPROPERTY(BlueprintReadOnly)
  TArray<FMemoryStoreInstruction> MemoryStore;

  UPROPERTY(BlueprintReadOnly)
  FAgentState StateTransform;

  UPROPERTY(BlueprintReadOnly)
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly)
  bool bHasAction;

  UPROPERTY(BlueprintReadOnly)
  FString Dialogue;

  FNPCInstruction()
      : Type(ENPCInstructionType::Finalize), Limit(0), Threshold(0.7f),
        bValid(true), bHasAction(false) {}
};

namespace TypeFactory {

inline FMemoryRecallInstruction MemoryRecallInstruction(
    const FString &Query, int32 Limit = 5, float Threshold = 0.7f) {
  FMemoryRecallInstruction Instruction;
  Instruction.Query = Query;
  Instruction.Limit = Limit;
  Instruction.Threshold = Threshold;
  return Instruction;
}

inline FMemoryStoreInstruction
MemoryStoreInstruction(const FString &Text,
                       const FString &Type = TEXT("observation"),
                       float Importance = 0.5f) {
  FMemoryStoreInstruction Instruction;
  Instruction.Text = Text;
  Instruction.Type = Type;
  Instruction.Importance = Importance;
  return Instruction;
}

} // namespace TypeFactory
