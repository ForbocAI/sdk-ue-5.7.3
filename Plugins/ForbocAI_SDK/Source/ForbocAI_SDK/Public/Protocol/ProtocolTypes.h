#pragma once

#include "CoreMinimal.h"
#include "Cortex/CortexTypes.h"
#include "Memory/MemoryTypes.h"
#include "NPC/AgentTypes.h"
#include "ProtocolTypes.generated.h"

/**
 * Directive Status
 */
UENUM(BlueprintType)
enum class EDirectiveStatus : uint8 { Idle, Running, Succeeded, Failed };

/**
 * Directive Run — A record of a protocol interaction.
 */
USTRUCT(BlueprintType)
struct FDirectiveRun {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString NpcId;

  UPROPERTY(BlueprintReadOnly)
  EDirectiveStatus Status;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> Tape;

  UPROPERTY(BlueprintReadOnly)
  FString LastResult;

  UPROPERTY(BlueprintReadOnly)
  FString Error;

  UPROPERTY(BlueprintReadOnly)
  int32 TurnCount;

  UPROPERTY(BlueprintReadOnly)
  int64 StartTime;

  UPROPERTY(BlueprintReadOnly)
  int64 EndTime;

  FDirectiveRun()
      : Status(EDirectiveStatus::Idle), TurnCount(0), StartTime(0), EndTime(0) {
  }
};

/**
 * NPC Instruction types for the protocol loop.
 */
UENUM(BlueprintType)
enum class ENPCInstructionType : uint8 {
  IdentifyActor,
  QueryVector,
  ExecuteInference,
  Finalize
};

/**
 * Memory Recall Instruction
 */
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

/**
 * Memory Store Instruction
 */
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

/**
 * NPC Instruction — A single step in the recursive protocol.
 */
USTRUCT(BlueprintType)
struct FNPCInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  ENPCInstructionType Type;

  UPROPERTY(BlueprintReadOnly)
  FString Payload;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> Tape;

  FNPCInstruction() : Type(ENPCInstructionType::Finalize) {}
};

/**
 * Recalled Memory
 */
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

namespace TypeFactory {

inline FMemoryRecallInstruction
MemoryRecallInstruction(FString Query, int32 Limit = 5,
                        float Threshold = 0.7f) {
  FMemoryRecallInstruction I;
  I.Query = MoveTemp(Query);
  I.Limit = Limit;
  I.Threshold = Threshold;
  return I;
}

inline FMemoryStoreInstruction
MemoryStoreInstruction(FString Text, FString Type = TEXT("observation"),
                       float Importance = 0.5f) {
  FMemoryStoreInstruction I;
  I.Text = MoveTemp(Text);
  I.Type = MoveTemp(Type);
  I.Importance = Importance;
  return I;
}

} // namespace TypeFactory
