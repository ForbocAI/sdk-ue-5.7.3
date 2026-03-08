#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace NPCSlice {

using namespace rtk;
using namespace func;

struct FNPCStateLogEntry {
  FString Timestamp;
  FString ActionType;
  FAgentState State;
};

// Extends a basic FAgent to include action history and block status.
struct FNPCInternalState {
  FString Id;
  FString Persona;
  FAgentState State;
  TArray<FString> History;
  Maybe<FAgentAction> LastAction;
  bool bIsBlocked;
  Maybe<FString> BlockReason;
  TArray<FNPCStateLogEntry> StateLog;

  FNPCInternalState() : bIsBlocked(false) {}
};

inline FString NPCIdSelector(const FNPCInternalState &NPC) { return NPC.Id; }

// Global EntityAdapter instance for NPCs
inline EntityAdapter<FNPCInternalState, decltype(&NPCIdSelector)>
GetNPCAdapter() {
  return EntityAdapter<FNPCInternalState, decltype(&NPCIdSelector)>(
      &NPCIdSelector);
}

// The root state for the NPC slice
struct FNPCSliceState {
  EntityState<FNPCInternalState> Entities;
  Maybe<FString> ActiveNpcId;

  FNPCSliceState() : Entities(GetNPCAdapter().getInitialState()) {}
};

} // namespace NPCSlice
