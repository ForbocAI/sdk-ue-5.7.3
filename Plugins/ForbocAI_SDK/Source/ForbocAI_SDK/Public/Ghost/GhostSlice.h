#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

// ==========================================================
// Ghost Slice (UE RTK)
// ==========================================================
// Equivalent to `ghostSlice.ts`
// Manages the state of the Ghost QA/test orchestration system.
// ==========================================================

namespace GhostSlice {

using namespace rtk;
using namespace func;

enum class EGhostSessionStatus : uint8 {
  Idle,
  Running,
  Paused,
  Completed,
  Error
};

// The root state for the Ghost slice
struct FGhostSliceState {
  Maybe<FString> ActiveSessionId;
  EGhostSessionStatus Status;
  float Progress; // 0.0 - 1.0
  TArray<FGhostTestResult> Results;
  TArray<FString> History; // Past session IDs or logs
  bool bLoading;
  Maybe<FString> Error;

  FGhostSliceState()
      : Status(EGhostSessionStatus::Idle), Progress(0.0f), bLoading(false) {}
};

// --- Actions ---
struct GhostSessionStartedAction : public Action {
  static const FString Type;
  FString Payload; // SessionId
  GhostSessionStartedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString GhostSessionStartedAction::Type =
    TEXT("ghost/sessionStarted");

struct GhostProgressUpdatedAction : public Action {
  static const FString Type;
  float Payload; // Progress
  GhostProgressUpdatedAction(float InPayload)
      : Action(Type), Payload(InPayload) {}
};
inline const FString GhostProgressUpdatedAction::Type =
    TEXT("ghost/progressUpdated");

struct GhostResultAddedAction : public Action {
  static const FString Type;
  FGhostTestResult Payload;
  GhostResultAddedAction(FGhostTestResult InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString GhostResultAddedAction::Type = TEXT("ghost/resultAdded");

struct GhostSessionCompletedAction : public Action {
  static const FString Type;
  GhostSessionCompletedAction() : Action(Type) {}
};
inline const FString GhostSessionCompletedAction::Type =
    TEXT("ghost/sessionCompleted");

struct GhostSessionFailedAction : public Action {
  static const FString Type;
  FString Payload; // Error
  GhostSessionFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString GhostSessionFailedAction::Type =
    TEXT("ghost/sessionFailed");

struct ClearGhostSessionAction : public Action {
  static const FString Type;
  ClearGhostSessionAction() : Action(Type) {}
};
inline const FString ClearGhostSessionAction::Type =
    TEXT("ghost/clearGhostSession");

namespace Actions {
inline GhostSessionStartedAction GhostSessionStarted(FString SessionId) {
  return GhostSessionStartedAction(MoveTemp(SessionId));
}
inline GhostProgressUpdatedAction GhostProgressUpdated(float Progress) {
  return GhostProgressUpdatedAction(Progress);
}
inline GhostResultAddedAction GhostResultAdded(FGhostTestResult Result) {
  return GhostResultAddedAction(MoveTemp(Result));
}
inline GhostSessionCompletedAction GhostSessionCompleted() {
  return GhostSessionCompletedAction();
}
inline GhostSessionFailedAction GhostSessionFailed(FString Error) {
  return GhostSessionFailedAction(MoveTemp(Error));
}
inline ClearGhostSessionAction ClearGhostSession() {
  return ClearGhostSessionAction();
}
} // namespace Actions

using namespace Actions;

// --- Slice Builder ---
inline Slice<FGhostSliceState> CreateGhostSlice() {
  return SliceBuilder<FGhostSliceState>(TEXT("ghost"), FGhostSliceState())
      .addCase<GhostSessionStartedAction>(
          [](FGhostSliceState State, const GhostSessionStartedAction &Action) {
            State.ActiveSessionId = just(Action.Payload);
            State.Status = EGhostSessionStatus::Running;
            State.Progress = 0.0f;
            State.Results.Empty();
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<GhostProgressUpdatedAction>(
          [](FGhostSliceState State, const GhostProgressUpdatedAction &Action) {
            State.Progress = Action.Payload;
            return State;
          })
      .addCase<GhostResultAddedAction>(
          [](FGhostSliceState State, const GhostResultAddedAction &Action) {
            State.Results.Add(Action.Payload);
            return State;
          })
      .addCase<GhostSessionCompletedAction>(
          [](FGhostSliceState State,
             const GhostSessionCompletedAction &Action) {
            State.Status = EGhostSessionStatus::Completed;
            State.Progress = 1.0f;
            if (State.ActiveSessionId.hasValue) {
              State.History.Add(State.ActiveSessionId.value);
            }
            return State;
          })
      .addCase<GhostSessionFailedAction>(
          [](FGhostSliceState State, const GhostSessionFailedAction &Action) {
            State.Status = EGhostSessionStatus::Error;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<ClearGhostSessionAction>(
          [](FGhostSliceState State, const ClearGhostSessionAction &Action) {
            State.ActiveSessionId = nothing<FString>();
            State.Status = EGhostSessionStatus::Idle;
            State.Progress = 0.0f;
            State.Results.Empty();
            State.Error = nothing<FString>();
            return State;
          })
      .build();
}

} // namespace GhostSlice
