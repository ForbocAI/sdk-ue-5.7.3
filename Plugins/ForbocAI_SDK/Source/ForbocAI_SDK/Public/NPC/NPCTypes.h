#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Types.h"
#include "NPCTypes.generated.h"

USTRUCT(BlueprintType)
struct FNPCHistoryEntry {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Role;

  UPROPERTY(BlueprintReadOnly)
  FString Content;
};

USTRUCT(BlueprintType)
struct FNPCStateLogEntry {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  int64 Timestamp;

  UPROPERTY(BlueprintReadOnly)
  FAgentState Delta;

  UPROPERTY(BlueprintReadOnly)
  FAgentState State;

  FNPCStateLogEntry() : Timestamp(0) {}
};

USTRUCT(BlueprintType)
struct FNPCInternalState {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  FAgentState State;

  UPROPERTY(BlueprintReadOnly)
  TArray<FNPCHistoryEntry> History;

  FAgentAction LastAction;
  bool bHasLastAction;

  UPROPERTY(BlueprintReadOnly)
  bool bIsBlocked;

  UPROPERTY(BlueprintReadOnly)
  FString BlockReason;

  UPROPERTY(BlueprintReadOnly)
  TArray<FNPCStateLogEntry> StateLog;

  FNPCInternalState() : bHasLastAction(false), bIsBlocked(false) {}
};

namespace NPCSlice {

using namespace rtk;
using namespace func;
using ::FNPCHistoryEntry;
using ::FNPCStateLogEntry;
using ::FNPCInternalState;

inline FString NPCIdSelector(const FNPCInternalState &NPC) { return NPC.Id; }

inline EntityAdapterOps<FNPCInternalState>
GetNPCAdapter() {
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

  if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Delta.JsonData),
                                    DeltaJson) ||
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
