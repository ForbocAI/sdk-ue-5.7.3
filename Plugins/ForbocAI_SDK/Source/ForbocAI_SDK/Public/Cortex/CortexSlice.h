#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace CortexSlice {

using namespace rtk;
using namespace func;

enum class ECortexEngineStatus : uint8 { Idle, Initializing, Ready, Error };

struct FCortexSliceState {
  ECortexEngineStatus Status;
  FCortexStatus EngineStatus;
  FString LastPrompt;
  FString LastResponseText;
  FString Error;

  FCortexSliceState() : Status(ECortexEngineStatus::Idle) {}
};

namespace Actions {

inline const ActionCreator<FString> &CortexInitPendingActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/initPending"));
  return ActionCreator;
}

inline const ActionCreator<FCortexStatus> &CortexInitSuccessActionCreator() {
  static const ActionCreator<FCortexStatus> ActionCreator =
      createAction<FCortexStatus>(TEXT("cortex/initSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &CortexInitFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/initFailed"));
  return ActionCreator;
}

inline const ActionCreator<FString> &CortexCompletePendingActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/completePending"));
  return ActionCreator;
}

inline const ActionCreator<FCortexResponse> &
CortexCompleteSuccessActionCreator() {
  static const ActionCreator<FCortexResponse> ActionCreator =
      createAction<FCortexResponse>(TEXT("cortex/completeSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &CortexCompleteFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/completeFailed"));
  return ActionCreator;
}

inline AnyAction CortexInitPending(const FString &ModelPath) {
  return CortexInitPendingActionCreator()(ModelPath);
}

inline AnyAction CortexInitFulfilled(const FCortexStatus &Status) {
  return CortexInitSuccessActionCreator()(Status);
}

inline AnyAction CortexInitRejected(const FString &Error) {
  return CortexInitFailedActionCreator()(Error);
}

inline AnyAction CortexCompletePending(const FString &Prompt) {
  return CortexCompletePendingActionCreator()(Prompt);
}

inline AnyAction CortexCompleteFulfilled(const FCortexResponse &Response) {
  return CortexCompleteSuccessActionCreator()(Response);
}

inline AnyAction CortexCompleteRejected(const FString &Error) {
  return CortexCompleteFailedActionCreator()(Error);
}

} // namespace Actions

inline Slice<FCortexSliceState> CreateCortexSlice() {
  return SliceBuilder<FCortexSliceState>(TEXT("cortex"), FCortexSliceState())
      .addExtraCase(
          Actions::CortexInitPendingActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.Status = ECortexEngineStatus::Initializing;
            Next.Error.Empty();
            Next.EngineStatus.Model = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::CortexInitSuccessActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FCortexStatus> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.Status = ECortexEngineStatus::Ready;
            Next.EngineStatus = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::CortexInitFailedActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.Status = ECortexEngineStatus::Error;
            Next.Error = Action.PayloadValue;
            Next.EngineStatus.Error = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::CortexCompletePendingActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.LastPrompt = Action.PayloadValue;
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(
          Actions::CortexCompleteSuccessActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FCortexResponse> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.LastResponseText = Action.PayloadValue.Text;
            return Next;
          })
      .addExtraCase(
          Actions::CortexCompleteFailedActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.Error = Action.PayloadValue;
            return Next;
          })
      .build();
}

} // namespace CortexSlice
