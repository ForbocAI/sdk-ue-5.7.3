#pragma once
// G7: Stream helpers — mirrors TS stream.ts
// Convenience free functions for consuming streaming cortex completions

#include "Core/functional_core.hpp"
#include "Cortex/CortexModule.h"
#include "CoreMinimal.h"

namespace StreamHelpers {

using FOnChunk = std::function<void(const FString &Chunk)>;
using FOnComplete = std::function<void(const FString &FullText)>;
using FOnError = std::function<void(const FString &Error)>;

/**
 * Stream tokens from a cortex with a per-chunk callback.
 * Mirrors TS `streamFromCortex(cortex, prompt, onChunk, options)`.
 * Returns a Future resolving to the full response.
 */
inline TFuture<CortexTypes::CortexCompletionResult>
StreamFromCortex(const FCortex &Cortex, const FString &Prompt,
                 const FOnChunk &OnChunk) {
  return CortexOps::CompleteStream(Cortex, Prompt, OnChunk);
}

/**
 * Stream tokens from a cortex with completion and error callbacks.
 * Mirrors TS `streamToCallback` + error handling pattern.
 * Fire-and-forget: callbacks run on the game thread.
 */
inline void StreamFromCortexWithCallbacks(const FCortex &Cortex,
                                          const FString &Prompt,
                                          const FOnChunk &OnChunk,
                                          const FOnComplete &OnComplete,
                                          const FOnError &OnError) {
  auto Future = CortexOps::CompleteStream(Cortex, Prompt, OnChunk);
  Async(EAsyncExecution::Thread, [Future = MoveTemp(Future), OnComplete,
                                   OnError]() mutable {
    auto Result = Future.Get();
    AsyncTask(ENamedThreads::GameThread, [Result, OnComplete, OnError]() {
      if (Result.isLeft) {
        if (OnError) {
          OnError(Result.left);
        }
      } else {
        if (OnComplete) {
          OnComplete(Result.right.Text);
        }
      }
    });
  });
}

} // namespace StreamHelpers
