#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

// ==========================================================
// Directive Slice (UE RTK)
// ==========================================================
// Equivalent to `directiveSlice.ts`
// Tracks multi-round protocol state and acts as a protocol audit log.
// ==========================================================

namespace DirectiveSlice {

using namespace rtk;
using namespace func;

inline FString DirectiveIdSelector(const FDirectiveRun &Run) { return Run.Id; }

// Global EntityAdapter instance for Directive Runs
inline EntityAdapter<FDirectiveRun, decltype(&DirectiveIdSelector)>
GetDirectiveAdapter() {
  return EntityAdapter<FDirectiveRun, decltype(&DirectiveIdSelector)>(
      &DirectiveIdSelector);
}

// The root state for the Directive slice
struct FDirectiveSliceState {
  EntityState<FDirectiveRun> Entities;
  Maybe<FString> ActiveDirectiveId;

  FDirectiveSliceState() : Entities(GetDirectiveAdapter().getInitialState()) {}
};

// --- Actions ---
struct DirectiveRunStartedAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString NpcId;
  };
  FPayload Payload;
  DirectiveRunStartedAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString DirectiveRunStartedAction::Type =
    TEXT("directive/directiveRunStarted");

struct DirectiveReceivedAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Directive;
  };
  FPayload Payload;
  DirectiveReceivedAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString DirectiveReceivedAction::Type =
    TEXT("directive/directiveReceived");

struct ContextComposedAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Context;
  };
  FPayload Payload;
  ContextComposedAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString ContextComposedAction::Type =
    TEXT("directive/contextComposed");

struct VerdictValidatedAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Verdict;
  };
  FPayload Payload;
  VerdictValidatedAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString VerdictValidatedAction::Type =
    TEXT("directive/verdictValidated");

struct DirectiveRunFailedAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Error;
  };
  FPayload Payload;
  DirectiveRunFailedAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString DirectiveRunFailedAction::Type =
    TEXT("directive/directiveRunFailed");

struct ClearDirectivesForNpcAction : public Action {
  static const FString Type;
  FString Payload; // NpcId
  ClearDirectivesForNpcAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString ClearDirectivesForNpcAction::Type =
    TEXT("directive/clearDirectivesForNpc");

namespace Actions {
inline DirectiveRunStartedAction
DirectiveRunStarted(DirectiveRunStartedAction::FPayload Payload) {
  return DirectiveRunStartedAction(MoveTemp(Payload));
}
inline DirectiveReceivedAction
DirectiveReceived(DirectiveReceivedAction::FPayload Payload) {
  return DirectiveReceivedAction(MoveTemp(Payload));
}
inline ContextComposedAction
ContextComposed(ContextComposedAction::FPayload Payload) {
  return ContextComposedAction(MoveTemp(Payload));
}
inline VerdictValidatedAction
VerdictValidated(VerdictValidatedAction::FPayload Payload) {
  return VerdictValidatedAction(MoveTemp(Payload));
}
inline DirectiveRunFailedAction
DirectiveRunFailed(DirectiveRunFailedAction::FPayload Payload) {
  return DirectiveRunFailedAction(MoveTemp(Payload));
}
inline ClearDirectivesForNpcAction ClearDirectivesForNpc(FString NpcId) {
  return ClearDirectivesForNpcAction(MoveTemp(NpcId));
}
} // namespace Actions

using namespace Actions;

// --- Slice Builder ---
inline Slice<FDirectiveSliceState> CreateDirectiveSlice() {
  return SliceBuilder<FDirectiveSliceState>(TEXT("directive"),
                                            FDirectiveSliceState())
      .addCase<DirectiveRunStartedAction>(
          [](FDirectiveSliceState State,
             const DirectiveRunStartedAction &Action) {
            FDirectiveRun NewRun;
            NewRun.Id = Action.Payload.Id;
            NewRun.NpcId = Action.Payload.NpcId;
            NewRun.Status = EDirectiveStatus::Running;
            NewRun.StartTime = FDateTime::Now().ToUnixTimestamp();
            State.Entities =
                GetDirectiveAdapter().upsertOne(State.Entities, NewRun);
            State.ActiveDirectiveId = just(NewRun.Id);
            return State;
          })
      .addCase<DirectiveReceivedAction>(
          [](FDirectiveSliceState State,
             const DirectiveReceivedAction &Action) {
            Maybe<FDirectiveRun> Run = GetDirectiveAdapter().selectById(
                State.Entities, Action.Payload.Id);
            if (Run.hasValue) {
              FDirectiveRun Upd = Run.value;
              Upd.Tape.Add(FString::Printf(TEXT("DIRECTIVE_RECEIVED: %s"),
                                           *Action.Payload.Directive));
              Upd.TurnCount++;
              State.Entities =
                  GetDirectiveAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<ContextComposedAction>([](FDirectiveSliceState State,
                                         const ContextComposedAction &Action) {
        Maybe<FDirectiveRun> Run =
            GetDirectiveAdapter().selectById(State.Entities, Action.Payload.Id);
        if (Run.hasValue) {
          FDirectiveRun Upd = Run.value;
          Upd.Tape.Add(FString::Printf(TEXT("CONTEXT_COMPOSED: %s"),
                                       *Action.Payload.Context));
          Upd.TurnCount++;
          State.Entities = GetDirectiveAdapter().updateOne(State.Entities, Upd);
        }
        return State;
      })
      .addCase<VerdictValidatedAction>(
          [](FDirectiveSliceState State, const VerdictValidatedAction &Action) {
            Maybe<FDirectiveRun> Run = GetDirectiveAdapter().selectById(
                State.Entities, Action.Payload.Id);
            if (Run.hasValue) {
              FDirectiveRun Upd = Run.value;
              Upd.Status = EDirectiveStatus::Succeeded;
              Upd.EndTime = FDateTime::Now().ToUnixTimestamp();
              Upd.LastResult = Action.Payload.Verdict;
              Upd.Tape.Add(FString::Printf(TEXT("VERDICT_VALIDATED: %s"),
                                           *Action.Payload.Verdict));
              State.Entities =
                  GetDirectiveAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<DirectiveRunFailedAction>(
          [](FDirectiveSliceState State,
             const DirectiveRunFailedAction &Action) {
            Maybe<FDirectiveRun> Run = GetDirectiveAdapter().selectById(
                State.Entities, Action.Payload.Id);
            if (Run.hasValue) {
              FDirectiveRun Upd = Run.value;
              Upd.Status = EDirectiveStatus::Failed;
              Upd.EndTime = FDateTime::Now().ToUnixTimestamp();
              Upd.Error = Action.Payload.Error;
              Upd.Tape.Add(
                  FString::Printf(TEXT("FAILED: %s"), *Action.Payload.Error));
              State.Entities =
                  GetDirectiveAdapter().updateOne(State.Entities, Upd);
            }
            return State;
          })
      .addCase<ClearDirectivesForNpcAction>(
          [](FDirectiveSliceState State,
             const ClearDirectivesForNpcAction &Action) {
            TArray<FDirectiveRun> Runs =
                GetDirectiveAdapter().selectAll(State.Entities);
            TArray<FString> IdsToRemove;
            for (const auto &Run : Runs) {
              if (Run.NpcId == Action.Payload) {
                IdsToRemove.Add(Run.Id);
              }
            }
            State.Entities =
                GetDirectiveAdapter().removeMany(State.Entities, IdsToRemove);
            return State;
          })
      .build();
}

} // namespace DirectiveSlice
