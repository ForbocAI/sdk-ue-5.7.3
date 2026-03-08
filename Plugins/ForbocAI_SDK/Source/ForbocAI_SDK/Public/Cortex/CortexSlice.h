#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

// ==========================================================
// Cortex Slice (UE RTK)
// ==========================================================
// Equivalent to `cortexSlice.ts`
// Tracks the status of local/remote inference engines and model context.
// ==========================================================

namespace CortexSlice {

using namespace rtk;
using namespace func;

enum class ECortexEngineStatus : uint8 { Idle, Initializing, Ready, Error };

// State for the Cortex slice
struct FCortexSliceState {
  ECortexEngineStatus Status;
  bool bIsInitializing;
  Maybe<FString> Error;
  FCortexStatus EngineStatus;

  FCortexSliceState()
      : Status(ECortexEngineStatus::Idle), bIsInitializing(false) {}
};

// --- Actions ---
struct CortexInitPendingAction : public Action {
  static const FString Type;
  CortexInitPendingAction() : Action(Type) {}
};
inline const FString CortexInitPendingAction::Type = TEXT("cortex/initPending");

struct CortexInitSuccessAction : public Action {
  static const FString Type;
  FCortexStatus Payload;
  CortexInitSuccessAction(FCortexStatus InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString CortexInitSuccessAction::Type = TEXT("cortex/initSuccess");

struct CortexInitFailedAction : public Action {
  static const FString Type;
  FString Payload;
  CortexInitFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString CortexInitFailedAction::Type = TEXT("cortex/initFailed");

struct CortexCompletePendingAction : public Action {
  static const FString Type;
  CortexCompletePendingAction() : Action(Type) {}
};
inline const FString CortexCompletePendingAction::Type =
    TEXT("cortex/completePending");

struct CortexCompleteSuccessAction : public Action {
  static const FString Type;
  CortexCompleteSuccessAction() : Action(Type) {}
};
inline const FString CortexCompleteSuccessAction::Type =
    TEXT("cortex/completeSuccess");

struct CortexCompleteFailedAction : public Action {
  static const FString Type;
  FString Payload;
  CortexCompleteFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString CortexCompleteFailedAction::Type =
    TEXT("cortex/completeFailed");

namespace Actions {
inline CortexInitPendingAction CortexInitPending() {
  return CortexInitPendingAction();
}
inline CortexInitSuccessAction CortexInitFulfilled(FCortexStatus Status) {
  return CortexInitSuccessAction(MoveTemp(Status));
}
inline CortexInitFailedAction CortexInitRejected(FString Error) {
  return CortexInitFailedAction(MoveTemp(Error));
}
inline CortexCompletePendingAction CortexCompletePending(FString Prompt) {
  return CortexCompletePendingAction();
}
inline CortexCompleteSuccessAction CortexCompleteFulfilled() {
  return CortexCompleteSuccessAction();
}
inline CortexCompleteFailedAction CortexCompleteRejected(FString Error) {
  return CortexCompleteFailedAction(MoveTemp(Error));
}
} // namespace Actions

using namespace Actions;

// --- Slice Builder ---
inline Slice<FCortexSliceState> CreateCortexSlice() {
  return SliceBuilder<FCortexSliceState>(TEXT("cortex"), FCortexSliceState())
      .addCase<CortexInitPendingAction>(
          [](FCortexSliceState State, const CortexInitPendingAction &Action) {
            State.Status = ECortexEngineStatus::Initializing;
            State.bIsInitializing = true;
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<CortexInitSuccessAction>(
          [](FCortexSliceState State, const CortexInitSuccessAction &Action) {
            State.Status = ECortexEngineStatus::Ready;
            State.bIsInitializing = false;
            State.EngineStatus = Action.Payload;
            return State;
          })
      .addCase<CortexInitFailedAction>(
          [](FCortexSliceState State, const CortexInitFailedAction &Action) {
            State.Status = ECortexEngineStatus::Error;
            State.bIsInitializing = false;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<CortexCompletePendingAction>(
          [](FCortexSliceState State,
             const CortexCompletePendingAction &Action) {
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<CortexCompleteSuccessAction>(
          [](FCortexSliceState State,
             const CortexCompleteSuccessAction &Action) { return State; })
      .addCase<CortexCompleteFailedAction>(
          [](FCortexSliceState State,
             const CortexCompleteFailedAction &Action) {
            State.Error = just(Action.Payload);
            return State;
          })
      .build();
}

} // namespace CortexSlice
