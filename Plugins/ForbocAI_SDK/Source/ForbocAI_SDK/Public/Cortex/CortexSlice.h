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
  // G2: Download progress tracking (mirrors TS setDownloadState)
  bool bIsDownloading;
  float DownloadProgress;
  // G3: Embedding readiness tracking (mirrors TS embedderReady)
  bool bEmbedderReady;
  // G7: Streaming state (mirrors TS stream.ts token-by-token)
  bool bIsStreaming;
  FString StreamAccumulated;

  FCortexSliceState()
      : Status(ECortexEngineStatus::Idle), bIsDownloading(false),
        DownloadProgress(0.0f), bEmbedderReady(false), bIsStreaming(false) {}
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

// G2: Download state action
struct FDownloadState {
  bool bIsDownloading;
  float Progress;
};

inline const ActionCreator<FDownloadState> &SetDownloadStateActionCreator() {
  static const ActionCreator<FDownloadState> ActionCreator =
      createAction<FDownloadState>(TEXT("cortex/setDownloadState"));
  return ActionCreator;
}

inline AnyAction SetDownloadState(bool bIsDownloading, float Progress) {
  FDownloadState State;
  State.bIsDownloading = bIsDownloading;
  State.Progress = Progress;
  return SetDownloadStateActionCreator()(State);
}

// G3: Embedder readiness action
inline const ActionCreator<bool> &SetEmbedderReadyActionCreator() {
  static const ActionCreator<bool> ActionCreator =
      createAction<bool>(TEXT("cortex/setEmbedderReady"));
  return ActionCreator;
}

inline AnyAction SetEmbedderReady(bool bReady) {
  return SetEmbedderReadyActionCreator()(bReady);
}

// G7: Streaming actions
inline const ActionCreator<FString> &CortexStreamStartActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/streamStart"));
  return ActionCreator;
}

inline const ActionCreator<FString> &CortexStreamTokenActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/streamToken"));
  return ActionCreator;
}

inline const ActionCreator<FString> &CortexStreamCompleteActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("cortex/streamComplete"));
  return ActionCreator;
}

inline AnyAction CortexStreamStart(const FString &Prompt) {
  return CortexStreamStartActionCreator()(Prompt);
}

inline AnyAction CortexStreamToken(const FString &Token) {
  return CortexStreamTokenActionCreator()(Token);
}

inline AnyAction CortexStreamComplete(const FString &FullText) {
  return CortexStreamCompleteActionCreator()(FullText);
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
      // G2: Download state reducer
      .addExtraCase(
          Actions::SetDownloadStateActionCreator(),
          [](const FCortexSliceState &State,
             const Action<Actions::FDownloadState> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.bIsDownloading = Action.PayloadValue.bIsDownloading;
            Next.DownloadProgress = Action.PayloadValue.Progress;
            return Next;
          })
      // G3: Embedder readiness reducer
      .addExtraCase(
          Actions::SetEmbedderReadyActionCreator(),
          [](const FCortexSliceState &State,
             const Action<bool> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.bEmbedderReady = Action.PayloadValue;
            return Next;
          })
      // G7: Streaming reducers
      .addExtraCase(
          Actions::CortexStreamStartActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.bIsStreaming = true;
            Next.StreamAccumulated.Empty();
            Next.LastPrompt = Action.PayloadValue;
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(
          Actions::CortexStreamTokenActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.StreamAccumulated += Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::CortexStreamCompleteActionCreator(),
          [](const FCortexSliceState &State,
             const Action<FString> &Action) -> FCortexSliceState {
            FCortexSliceState Next = State;
            Next.bIsStreaming = false;
            Next.LastResponseText = Action.PayloadValue;
            Next.StreamAccumulated.Empty();
            return Next;
          })
      .build();
}

} // namespace CortexSlice
