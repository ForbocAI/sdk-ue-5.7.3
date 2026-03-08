#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

// ==========================================================
// NPC Slice (UE RTK)
// ==========================================================
// Equivalent to `npcSlice.ts`
// Manages the state of all Agents/NPCs, replacing legacy
// AgentModule state with a normalized EntityAdapter.
// ==========================================================

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

// --- Actions ---
struct SetNPCInfoAction : public Action {
  static const FString Type;
  FNPCInternalState Payload;
  SetNPCInfoAction(FNPCInternalState InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetNPCInfoAction::Type = TEXT("npc/setNPCInfo");

struct SetActiveNPCAction : public Action {
  static const FString Type;
  FString Payload;
  SetActiveNPCAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetActiveNPCAction::Type = TEXT("npc/setActiveNPC");

struct SetNPCStateAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FAgentState State;
  };
  FPayload Payload;
  SetNPCStateAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetNPCStateAction::Type = TEXT("npc/setNPCState");
inline const FString SoulExportRejectedAction::Type =
    TEXT("soul/exportRejected");

namespace Actions {
inline SoulExportPendingAction SoulExportPending(FString SoulId) {
  return SoulExportPendingAction(MoveTemp(SoulId));
}
inline SoulExportFulfilledAction SoulExportFulfilled(FSoul Soul) {
  return SoulExportFulfilledAction(MoveTemp(Soul));
}
inline SoulExportRejectedAction SoulExportRejected(FString Error) {
  return SoulExportRejectedAction(MoveTemp(Error));
}
} // namespace Actions

using namespace Actions;

struct UpdateNPCStateAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FAgentState StateDelta;
  };
  FPayload Payload;
  UpdateNPCStateAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString UpdateNPCStateAction::Type = TEXT("npc/updateNPCState");

struct AddToHistoryAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Entry;
  };
  FPayload Payload;
  AddToHistoryAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString AddToHistoryAction::Type = TEXT("npc/addToHistory");

struct SetHistoryAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    TArray<FString> History;
  };
  FPayload Payload;
  SetHistoryAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetHistoryAction::Type = TEXT("npc/setHistory");

struct SetLastActionAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FAgentAction Action;
  };
  FPayload Payload;
  SetLastActionAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetLastActionAction::Type = TEXT("npc/setLastAction");

struct BlockActionAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Reason;
  };
  FPayload Payload;
  BlockActionAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString BlockActionAction::Type = TEXT("npc/blockAction");

struct ClearBlockAction : public Action {
  static const FString Type;
  FString Payload;
  ClearBlockAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString ClearBlockAction::Type = TEXT("npc/clearBlock");

struct RemoveNPCAction : public Action {
  static const FString Type;
  FString Payload;
  RemoveNPCAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString RemoveNPCAction::Type = TEXT("npc/removeNPC");

namespace Actions {
inline SetNPCInfoAction SetNPCInfo(FNPCInternalState Info) {
  return SetNPCInfoAction(MoveTemp(Info));
}
inline SetActiveNPCAction SetActiveNPC(FString Id) {
  return SetActiveNPCAction(MoveTemp(Id));
}
inline SetNPCStateAction SetNPCState(FString Id, FAgentState State) {
  return SetNPCStateAction({MoveTemp(Id), MoveTemp(State)});
}
inline UpdateNPCStateAction UpdateNPCState(FString Id, FAgentState Delta) {
  return UpdateNPCStateAction({MoveTemp(Id), MoveTemp(Delta)});
}
inline AddToHistoryAction AddToHistory(FString Id, FString Entry) {
  return AddToHistoryAction({MoveTemp(Id), MoveTemp(Entry)});
}
inline SetHistoryAction SetHistory(FString Id, TArray<FString> History) {
  return SetHistoryAction({MoveTemp(Id), MoveTemp(History)});
}
inline SetLastActionAction SetLastAction(FString Id, FAgentAction LastAction) {
  return SetLastActionAction({MoveTemp(Id), MoveTemp(LastAction)});
}
inline BlockActionAction BlockAction(FString Id, FString Reason) {
  return BlockActionAction({MoveTemp(Id), MoveTemp(Reason)});
}
inline ClearBlockAction ClearBlock(FString Id) {
  return ClearBlockAction(MoveTemp(Id));
}
inline RemoveNPCAction RemoveNPC(FString Id) {
  return RemoveNPCAction(MoveTemp(Id));
}
} // namespace Actions

using namespace Actions;

// --- Slice Builder ---
inline Slice<FNPCSliceState> CreateNPCSlice() {
  return SliceBuilder<FNPCSliceState>(TEXT("npc"), FNPCSliceState())
      .addCase<SetNPCInfoAction>(
          [](FNPCSliceState State, const SetNPCInfoAction &Action) {
            FNPCInternalState NewNPC = Action.Payload;
            FNPCStateLogEntry LogEntry;
            LogEntry.Timestamp = FDateTime::Now().ToString();
            LogEntry.ActionType = TEXT("INITIALIZE");
            LogEntry.State = NewNPC.State;
            NewNPC.StateLog.Add(LogEntry);
            State.Entities = GetNPCAdapter().upsertOne(State.Entities, NewNPC);
            return State;
          })
      .addCase<SetActiveNPCAction>(
          [](FNPCSliceState State, const SetActiveNPCAction &Action) {
            State.ActiveNpcId = just(Action.Payload);
            return State;
          })
      .addCase<SetNPCStateAction>(
          [](FNPCSliceState State, const SetNPCStateAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload.Id);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              Upd.State = Action.Payload.State;

              FNPCStateLogEntry LogEntry;
              LogEntry.Timestamp = FDateTime::Now().ToString();
              LogEntry.ActionType = TEXT("SET_STATE");
              LogEntry.State = Upd.State;
              Upd.StateLog.Add(LogEntry);

              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<UpdateNPCStateAction>(
          [](FNPCSliceState State, const UpdateNPCStateAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload.Id);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              // Merge delta into state (simple replace for now)
              Upd.State = Action.Payload.StateDelta;

              FNPCStateLogEntry LogEntry;
              LogEntry.Timestamp = FDateTime::Now().ToString();
              LogEntry.ActionType = TEXT("UPDATE_STATE");
              LogEntry.State = Upd.State;
              Upd.StateLog.Add(LogEntry);

              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<AddToHistoryAction>(
          [](FNPCSliceState State, const AddToHistoryAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload.Id);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              Upd.History.Add(Action.Payload.Entry);
              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<SetHistoryAction>(
          [](FNPCSliceState State, const SetHistoryAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload.Id);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              Upd.History = Action.Payload.History;
              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<SetLastActionAction>(
          [](FNPCSliceState State, const SetLastActionAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload.Id);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              Upd.LastAction = just(Action.Payload.Action);
              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<BlockActionAction>(
          [](FNPCSliceState State, const BlockActionAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload.Id);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              Upd.bIsBlocked = true;
              Upd.BlockReason = just(Action.Payload.Reason);
              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<ClearBlockAction>(
          [](FNPCSliceState State, const ClearBlockAction &Action) {
            Maybe<FNPCInternalState> Existing =
                GetNPCAdapter().selectById(State.Entities, Action.Payload);
            if (Existing.hasValue) {
              FNPCInternalState Upd = Existing.value;
              Upd.bIsBlocked = false;
              Upd.BlockReason = nothing<FString>();
              State.Entities = GetNPCAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<RemoveNPCAction>(
          [](FNPCSliceState State, const RemoveNPCAction &Action) {
            State.Entities =
                GetNPCAdapter().removeOne(State.Entities, Action.Payload);
            if (State.ActiveNpcId.hasValue &&
                State.ActiveNpcId.value == Action.Payload) {
              State.ActiveNpcId = nothing<FString>();
            }
            return State;
          })
      .build();
}

} // namespace NPCSlice
