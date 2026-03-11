#pragma once
// directive wire carries intent; static is not an excuse

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace DirectiveSlice {

using namespace rtk;
using namespace func;

struct FDirectiveRunStartedPayload {
  FString Id;
  FString NpcId;
  FString Observation;
};

struct FDirectiveReceivedPayload {
  FString Id;
  FDirectiveResponse Response;
};

struct FContextComposedPayload {
  FString Id;
  FString Prompt;
  FCortexConfig Constraints;
};

struct FVerdictValidatedPayload {
  FString Id;
  FVerdictResponse Verdict;
};

struct FDirectiveRunFailedPayload {
  FString Id;
  FString Error;
};

inline FString DirectiveIdSelector(const FDirectiveRun &Run) { return Run.Id; }

inline EntityAdapterOps<FDirectiveRun> GetDirectiveAdapter() {
  return createEntityAdapter<FDirectiveRun>(&DirectiveIdSelector);
}

struct FDirectiveSliceState {
  EntityState<FDirectiveRun> Entities;
  /**
   * User Story: As directive orchestration state, I need a selector for the
   * currently active directive run so downstream components can bind to it.
   * (From TS)
   */
  FString ActiveDirectiveId;

  FDirectiveSliceState() : Entities(GetDirectiveAdapter().getInitialState()) {}
};

namespace Actions {

inline const ActionCreator<FDirectiveRunStartedPayload> &
DirectiveRunStartedActionCreator() {
  static const ActionCreator<FDirectiveRunStartedPayload> ActionCreator =
      createAction<FDirectiveRunStartedPayload>(
          TEXT("directive/directiveRunStarted"));
  return ActionCreator;
}

inline const ActionCreator<FDirectiveReceivedPayload> &
DirectiveReceivedActionCreator() {
  static const ActionCreator<FDirectiveReceivedPayload> ActionCreator =
      createAction<FDirectiveReceivedPayload>(
          TEXT("directive/directiveReceived"));
  return ActionCreator;
}

inline const ActionCreator<FContextComposedPayload> &
ContextComposedActionCreator() {
  static const ActionCreator<FContextComposedPayload> ActionCreator =
      createAction<FContextComposedPayload>(TEXT("directive/contextComposed"));
  return ActionCreator;
}

inline const ActionCreator<FVerdictValidatedPayload> &
VerdictValidatedActionCreator() {
  static const ActionCreator<FVerdictValidatedPayload> ActionCreator =
      createAction<FVerdictValidatedPayload>(
          TEXT("directive/verdictValidated"));
  return ActionCreator;
}

inline const ActionCreator<FDirectiveRunFailedPayload> &
DirectiveRunFailedActionCreator() {
  static const ActionCreator<FDirectiveRunFailedPayload> ActionCreator =
      createAction<FDirectiveRunFailedPayload>(
          TEXT("directive/directiveRunFailed"));
  return ActionCreator;
}

inline const ActionCreator<FString> &ClearDirectivesForNpcActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("directive/clearDirectivesForNpc"));
  return ActionCreator;
}

inline AnyAction DirectiveRunStarted(const FString &Id, const FString &NpcId,
                                     const FString &Observation) {
  return DirectiveRunStartedActionCreator()(
      FDirectiveRunStartedPayload{Id, NpcId, Observation});
}

inline AnyAction DirectiveReceived(const FString &Id,
                                   const FDirectiveResponse &Response) {
  return DirectiveReceivedActionCreator()(
      FDirectiveReceivedPayload{Id, Response});
}

inline AnyAction ContextComposed(const FString &Id, const FString &Prompt,
                                 const FCortexConfig &Constraints) {
  return ContextComposedActionCreator()(
      FContextComposedPayload{Id, Prompt, Constraints});
}

inline AnyAction VerdictValidated(const FString &Id,
                                  const FVerdictResponse &Verdict) {
  return VerdictValidatedActionCreator()(FVerdictValidatedPayload{Id, Verdict});
}

inline AnyAction DirectiveRunFailed(const FString &Id, const FString &Error) {
  return DirectiveRunFailedActionCreator()(
      FDirectiveRunFailedPayload{Id, Error});
}

inline AnyAction ClearDirectivesForNpc(const FString &NpcId) {
  return ClearDirectivesForNpcActionCreator()(NpcId);
}

} // namespace Actions

inline Slice<FDirectiveSliceState> CreateDirectiveSlice() {
  return SliceBuilder<FDirectiveSliceState>(TEXT("directive"),
                                            FDirectiveSliceState())
      .addExtraCase(Actions::DirectiveRunStartedActionCreator(),
                    [](const FDirectiveSliceState &State,
                       const Action<FDirectiveRunStartedPayload> &Action)
                        -> FDirectiveSliceState {
                      FDirectiveSliceState Next = State;
                      FDirectiveRun Run;
                      Run.Id = Action.PayloadValue.Id;
                      Run.NpcId = Action.PayloadValue.NpcId;
                      Run.Observation = Action.PayloadValue.Observation;
                      Run.Status = EDirectiveStatus::Running;
                      Run.StartedAt = FDateTime::UtcNow().ToUnixTimestamp();
                      Next.Entities =
                          GetDirectiveAdapter().upsertOne(Next.Entities, Run);
                      Next.ActiveDirectiveId = Run.Id;
                      return Next;
                    })
      .addExtraCase(Actions::DirectiveReceivedActionCreator(),
                    [](const FDirectiveSliceState &State,
                       const Action<FDirectiveReceivedPayload> &Action)
                        -> FDirectiveSliceState {
                      FDirectiveSliceState Next = State;
                      const FDirectiveReceivedPayload &Payload =
                          Action.PayloadValue;
                      Next.Entities = GetDirectiveAdapter().updateOne(
                          Next.Entities, Payload.Id,
                          [Payload](const FDirectiveRun &Existing) {
                            FDirectiveRun Updated = Existing;
                            Updated.MemoryRecallQuery =
                                Payload.Response.MemoryRecall.Query;
                            Updated.MemoryRecallLimit =
                                Payload.Response.MemoryRecall.Limit;
                            Updated.MemoryRecallThreshold =
                                Payload.Response.MemoryRecall.Threshold;
                            return Updated;
                          });
                      return Next;
                    })
      .addExtraCase(Actions::ContextComposedActionCreator(),
                    [](const FDirectiveSliceState &State,
                       const Action<FContextComposedPayload> &Action)
                        -> FDirectiveSliceState {
                      FDirectiveSliceState Next = State;
                      const FContextComposedPayload &Payload =
                          Action.PayloadValue;
                      Next.Entities = GetDirectiveAdapter().updateOne(
                          Next.Entities, Payload.Id,
                          [Payload](const FDirectiveRun &Existing) {
                            FDirectiveRun Updated = Existing;
                            Updated.ContextPrompt = Payload.Prompt;
                            Updated.ContextConstraints = Payload.Constraints;
                            return Updated;
                          });
                      return Next;
                    })
      .addExtraCase(
          Actions::VerdictValidatedActionCreator(),
          [](const FDirectiveSliceState &State,
             const Action<FVerdictValidatedPayload> &Action)
              -> FDirectiveSliceState {
            FDirectiveSliceState Next = State;
            const FVerdictValidatedPayload &Payload = Action.PayloadValue;
            Next.Entities = GetDirectiveAdapter().updateOne(
                Next.Entities, Payload.Id,
                [Payload](const FDirectiveRun &Existing) {
                  FDirectiveRun Updated = Existing;
                  Updated.Status = EDirectiveStatus::Completed;
                  Updated.CompletedAt = FDateTime::UtcNow().ToUnixTimestamp();
                  Updated.bVerdictValid = Payload.Verdict.bValid;
                  Updated.VerdictDialogue = Payload.Verdict.Dialogue;
                  Updated.VerdictActionType = Payload.Verdict.bHasAction
                                                  ? Payload.Verdict.Action.Type
                                                  : TEXT("");
                  return Updated;
                });
            return Next;
          })
      .addExtraCase(Actions::DirectiveRunFailedActionCreator(),
                    [](const FDirectiveSliceState &State,
                       const Action<FDirectiveRunFailedPayload> &Action)
                        -> FDirectiveSliceState {
                      FDirectiveSliceState Next = State;
                      const FDirectiveRunFailedPayload &Payload =
                          Action.PayloadValue;
                      Next.Entities = GetDirectiveAdapter().updateOne(
                          Next.Entities, Payload.Id,
                          [Payload](const FDirectiveRun &Existing) {
                            FDirectiveRun Updated = Existing;
                            Updated.Status = EDirectiveStatus::Failed;
                            Updated.CompletedAt =
                                FDateTime::UtcNow().ToUnixTimestamp();
                            Updated.Error = Payload.Error;
                            return Updated;
                          });
                      return Next;
                    })
      .addExtraCase(
          Actions::ClearDirectivesForNpcActionCreator(),
          [](const FDirectiveSliceState &State,
             const Action<FString> &Action) -> FDirectiveSliceState {
            FDirectiveSliceState Next = State;
            const TArray<FDirectiveRun> Runs =
                GetDirectiveAdapter().getSelectors().selectAll(Next.Entities);
            TArray<FString> IdsToRemove;
            for (int32 Index = 0; Index < Runs.Num(); ++Index) {
              if (Runs[Index].NpcId == Action.PayloadValue) {
                IdsToRemove.Add(Runs[Index].Id);
              }
            }
            Next.Entities =
                GetDirectiveAdapter().removeMany(Next.Entities, IdsToRemove);
            if (IdsToRemove.Contains(Next.ActiveDirectiveId)) {
              Next.ActiveDirectiveId.Empty();
            }
            return Next;
          })
      .build();
}

/**
 * User Story: As directive orchestration state, I need selectors for the
 * directive run entities and the active directive.
 * (From TS: selectDirectiveById, selectAllDirectives, selectActiveDirectiveId,
 * selectActiveDirective)
 */
inline func::Maybe<FDirectiveRun> SelectDirectiveById(
    const FDirectiveSliceState &State, const FString &Id) {
  return GetDirectiveAdapter().getSelectors().selectById(State.Entities, Id);
}

inline TArray<FDirectiveRun> SelectAllDirectives(
    const FDirectiveSliceState &State) {
  return GetDirectiveAdapter().getSelectors().selectAll(State.Entities);
}

inline FString SelectActiveDirectiveId(const FDirectiveSliceState &State) {
  return State.ActiveDirectiveId;
}

inline func::Maybe<FDirectiveRun> SelectActiveDirective(
    const FDirectiveSliceState &State) {
  if (State.ActiveDirectiveId.IsEmpty()) {
    return func::nothing<FDirectiveRun>();
  }
  return SelectDirectiveById(State, State.ActiveDirectiveId);
}

} // namespace DirectiveSlice
