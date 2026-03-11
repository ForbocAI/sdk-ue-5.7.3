#pragma once

// clang-format off
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Types.h"
#include "NPCTypes.generated.h"
// clang-format on

USTRUCT(BlueprintType)
struct FNPCHistoryEntry {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Role;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Content;
};

USTRUCT(BlueprintType)
struct FNPCStateLogEntry {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  int64 Timestamp;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FAgentState Delta;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FAgentState State;

  FNPCStateLogEntry() : Timestamp(0) {}
};

USTRUCT(BlueprintType)
struct FNPCInternalState {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FAgentState State;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  TArray<FNPCHistoryEntry> History;

  FAgentAction LastAction;
  bool bHasLastAction;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  bool bIsBlocked;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  FString BlockReason;

  UPROPERTY(BlueprintReadOnly, Category = "NPC")
  TArray<FNPCStateLogEntry> StateLog;

  FNPCInternalState() : bHasLastAction(false), bIsBlocked(false) {}
};

namespace NPCSlice {

using namespace rtk;
using namespace func;
using ::FNPCHistoryEntry;
using ::FNPCInternalState;
using ::FNPCStateLogEntry;

inline FString NPCIdSelector(const FNPCInternalState &NPC) { return NPC.Id; }

inline EntityAdapterOps<FNPCInternalState> GetNPCAdapter() {
  return createEntityAdapter<FNPCInternalState>(&NPCIdSelector);
}

struct FNPCSliceState {
  EntityState<FNPCInternalState> Entities;
  FString ActiveNpcId;

  FNPCSliceState() : Entities(GetNPCAdapter().getInitialState()) {}
};

inline FAgentState MergeStateDelta(const FAgentState &Current,
                                   const FAgentState &Delta) {
  if (Delta.JsonData.IsEmpty() || Delta.JsonData == TEXT("{}")) {
    return Current;
  }

  if (Current.JsonData.IsEmpty() || Current.JsonData == TEXT("{}")) {
    return Delta;
  }

  TSharedPtr<FJsonObject> CurrentJson = MakeShared<FJsonObject>();
  TSharedPtr<FJsonObject> DeltaJson = MakeShared<FJsonObject>();

  if (!FJsonSerializer::Deserialize(
          TJsonReaderFactory<>::Create(Current.JsonData), CurrentJson) ||
      !CurrentJson.IsValid()) {
    return Delta;
  }

  if (!FJsonSerializer::Deserialize(
          TJsonReaderFactory<>::Create(Delta.JsonData), DeltaJson) ||
      !DeltaJson.IsValid()) {
    return Delta;
  }

  for (const auto &Pair : DeltaJson->Values) {
    CurrentJson->SetField(Pair.Key, Pair.Value);
  }

  FString MergedJson;
  FJsonSerializer::Serialize(CurrentJson.ToSharedRef(),
                             TJsonWriterFactory<>::Create(&MergedJson));
  return TypeFactory::AgentState(MergedJson);
}

inline FNPCStateLogEntry MakeStateLogEntry(const FAgentState &Delta,
                                           const FAgentState &State) {
  FNPCStateLogEntry Entry;
  Entry.Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
  Entry.Delta = Delta;
  Entry.State = State;
  return Entry;
}

} // namespace NPCSlice
