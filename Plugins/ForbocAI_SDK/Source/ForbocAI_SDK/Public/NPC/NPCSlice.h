#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"
#include "NPC/NPCSliceActions.h"
#include "NPC/NPCTypes.h"

// ==========================================================
// NPC Slice (UE RTK)
// ==========================================================

namespace NPCSlice {

using namespace rtk;
using namespace func;

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
