#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace GhostSlice {

using namespace rtk;

struct FGhostSessionStartedPayload {
  FString SessionId;
  FString Status;
};

struct FGhostSessionProgressPayload {
  FString SessionId;
  FString Status;
  float Progress;

  FGhostSessionProgressPayload() : Progress(0.0f) {}
};

struct FGhostSessionFailedPayload {
  FString SessionId;
  FString Error;
};

struct FGhostSliceState {
  FString ActiveSessionId;
  FString Status;
  float Progress;
  FGhostTestReport Results;
  bool bHasResults;
  TArray<FGhostHistoryEntry> History;
  bool bLoading;
  FString Error;

  FGhostSliceState()
      : Status(TEXT("idle")), Progress(0.0f), bHasResults(false),
        bLoading(false) {}
};

namespace Actions {

inline const ActionCreator<FGhostSessionStartedPayload> &
GhostSessionStartedActionCreator() {
  static const ActionCreator<FGhostSessionStartedPayload> ActionCreator =
      createAction<FGhostSessionStartedPayload>(TEXT("ghost/sessionStarted"));
  return ActionCreator;
}

inline const ActionCreator<FGhostSessionProgressPayload> &
GhostSessionProgressActionCreator() {
  static const ActionCreator<FGhostSessionProgressPayload> ActionCreator =
      createAction<FGhostSessionProgressPayload>(TEXT("ghost/sessionProgress"));
  return ActionCreator;
}

inline const ActionCreator<FGhostTestReport> &
GhostSessionCompletedActionCreator() {
  static const ActionCreator<FGhostTestReport> ActionCreator =
      createAction<FGhostTestReport>(TEXT("ghost/sessionCompleted"));
  return ActionCreator;
}

inline const ActionCreator<FGhostSessionFailedPayload> &
GhostSessionFailedActionCreator() {
  static const ActionCreator<FGhostSessionFailedPayload> ActionCreator =
      createAction<FGhostSessionFailedPayload>(TEXT("ghost/sessionFailed"));
  return ActionCreator;
}

inline const ActionCreator<TArray<FGhostHistoryEntry>> &
GhostHistoryLoadedActionCreator() {
  static const ActionCreator<TArray<FGhostHistoryEntry>> ActionCreator =
      createAction<TArray<FGhostHistoryEntry>>(TEXT("ghost/historyLoaded"));
  return ActionCreator;
}

inline const EmptyActionCreator &ClearGhostSessionActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("ghost/clearGhostSession"));
  return ActionCreator;
}

inline AnyAction GhostSessionStarted(const FString &SessionId,
                                     const FString &Status = TEXT("running")) {
  return GhostSessionStartedActionCreator()(
      FGhostSessionStartedPayload{SessionId, Status});
}

inline AnyAction GhostSessionProgress(const FString &SessionId,
                                      const FString &Status, float Progress) {
  FGhostSessionProgressPayload Payload;
  Payload.SessionId = SessionId;
  Payload.Status = Status;
  Payload.Progress = Progress;
  return GhostSessionProgressActionCreator()(Payload);
}

inline AnyAction GhostSessionCompleted(const FGhostTestReport &Report) {
  return GhostSessionCompletedActionCreator()(Report);
}

inline AnyAction GhostSessionFailed(const FString &SessionId,
                                    const FString &Error) {
  return GhostSessionFailedActionCreator()(
      FGhostSessionFailedPayload{SessionId, Error});
}

inline AnyAction GhostHistoryLoaded(const TArray<FGhostHistoryEntry> &History) {
  return GhostHistoryLoadedActionCreator()(History);
}

inline AnyAction ClearGhostSession() {
  return ClearGhostSessionActionCreator()();
}

} // namespace Actions

inline Slice<FGhostSliceState> CreateGhostSlice() {
  return SliceBuilder<FGhostSliceState>(TEXT("ghost"), FGhostSliceState())
      .addExtraCase(
          Actions::GhostSessionStartedActionCreator(),
          [](const FGhostSliceState &State,
             const Action<FGhostSessionStartedPayload> &Action)
              -> FGhostSliceState {
            FGhostSliceState Next = State;
            Next.ActiveSessionId = Action.PayloadValue.SessionId;
            Next.Status = Action.PayloadValue.Status;
            Next.Progress = 0.0f;
            Next.bLoading = false;
            Next.Error.Empty();
            Next.bHasResults = false;
            return Next;
          })
      .addExtraCase(
          Actions::GhostSessionProgressActionCreator(),
          [](const FGhostSliceState &State,
             const Action<FGhostSessionProgressPayload> &Action)
              -> FGhostSliceState {
            FGhostSliceState Next = State;
            Next.ActiveSessionId = Action.PayloadValue.SessionId;
            Next.Status = Action.PayloadValue.Status;
            Next.Progress = Action.PayloadValue.Progress;
            return Next;
          })
      .addExtraCase(
          Actions::GhostSessionCompletedActionCreator(),
          [](const FGhostSliceState &State,
             const Action<FGhostTestReport> &Action) -> FGhostSliceState {
            FGhostSliceState Next = State;
            Next.Results = Action.PayloadValue;
            Next.bHasResults = true;
            Next.Status = TEXT("completed");
            Next.Progress = 1.0f;
            Next.bLoading = false;
            if (Action.PayloadValue.Results.Num() > 0) {
              Next.ActiveSessionId =
                  Action.PayloadValue.Results[0].Scenario.IsEmpty()
                      ? Next.ActiveSessionId
                      : Next.ActiveSessionId;
            }
            return Next;
          })
      .addExtraCase(
          Actions::GhostSessionFailedActionCreator(),
          [](const FGhostSliceState &State,
             const Action<FGhostSessionFailedPayload> &Action)
              -> FGhostSliceState {
            FGhostSliceState Next = State;
            Next.ActiveSessionId = Action.PayloadValue.SessionId;
            Next.Status = TEXT("failed");
            Next.bLoading = false;
            Next.Error = Action.PayloadValue.Error;
            return Next;
          })
      .addExtraCase(
          Actions::GhostHistoryLoadedActionCreator(),
          [](const FGhostSliceState &State,
             const Action<TArray<FGhostHistoryEntry>> &Action)
              -> FGhostSliceState {
            FGhostSliceState Next = State;
            Next.History = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::ClearGhostSessionActionCreator(),
          [](const FGhostSliceState &State,
             const Action<rtk::FEmptyPayload> &Action)
              -> FGhostSliceState {
            return FGhostSliceState();
          })
      .build();
}

} // namespace GhostSlice
