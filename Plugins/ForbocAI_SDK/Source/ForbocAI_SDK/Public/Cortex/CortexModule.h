#pragma once

#include "Async/Async.h"
#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "Types.h"

// ==========================================================
// Cortex Module — Local SLM Inference (UE SDK)
// ==========================================================
// Strict functional programming implementation for local
// Small Language Model inference using sqlite-vss.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

// Functional Core Type Aliases for Cortex operations
namespace CortexTypes {
using func::AsyncResult;
using func::ConfigBuilder;
using func::Curried;
using func::Either;
using func::just;
using func::Lazy;
using func::make_left;
using func::make_right;
using func::Maybe;
using func::nothing;
using func::Pipeline;
using func::TestResult;
using func::ValidationPipeline;

// Type aliases for Cortex operations
using CortexCreationResult = Either<FString, FCortex>;
using CortexInitResult = Either<FString, bool>;
using CortexCompletionResult = Either<FString, FCortexResponse>;
using CortexStreamResult = Either<FString, TArray<FString>>;
} // namespace CortexTypes

// ==========================================================
// Cortex Module — Local SLM Inference (UE SDK)
// ==========================================================
// Strict functional programming implementation for local
// Small Language Model inference using sqlite-vss.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

// Types (FCortex, FCortexConfig, FCortexResponse) are defined in
// ForbocAI_SDK_Types.h

/**
 * Cortex Operations — Stateless free functions.
 */
namespace CortexOps {

/**
 * Factory: Creates a Cortex instance from configuration.
 * Pure function: Config -> Cortex
 * @param Config The configuration to use.
 * @return A new Cortex instance.
 */
FORBOCAI_SDK_API FCortex Create(const FCortexConfig &Config);

/**
 * Initializes the Cortex engine and loads the model asynchronously.
 * Pure function: Cortex -> Result
 * @param Cortex The Cortex instance to initialize.
 * @return A validation result indicating success or failure.
 */
FORBOCAI_SDK_API TFuture<CortexTypes::CortexInitResult> Init(FCortex &Cortex);

/**
 * Generates text completion from prompt.
 * Pure function: (Cortex, Prompt, Context) -> Response
 * @param Cortex The Cortex instance to use.
 * @param Prompt The input prompt text.
 * @param Context Optional context data.
 * @return The generated response as a `TFuture`.
 */
FORBOCAI_SDK_API TFuture<CortexTypes::CortexCompletionResult>
Complete(const FCortex &Cortex, const FString &Prompt,
         const TMap<FString, FString> &Context = TMap<FString, FString>());

/** Per-token callback type for streaming inference. */
using FOnCortexToken = std::function<void(const FString &Token)>;

/**
 * Streams text generation token by token via callback (G7).
 * @param Cortex The Cortex instance to use.
 * @param Prompt The input prompt text.
 * @param OnToken Called for each generated token on the game thread.
 * @param Context Optional context data.
 * @return Future resolving to the full accumulated response.
 */
FORBOCAI_SDK_API TFuture<CortexTypes::CortexCompletionResult>
CompleteStream(const FCortex &Cortex, const FString &Prompt,
               const FOnCortexToken &OnToken,
               const TMap<FString, FString> &Context = TMap<FString, FString>());

/**
 * Gets the current status of the Cortex engine.
 * Pure function: Cortex -> Status
 * @param Cortex The Cortex instance to check.
 * @return The current status information.
 */
FORBOCAI_SDK_API FString GetStatus(const FCortex &Cortex);

/**
 * Cleans up resources and shuts down the engine.
 * Pure function: Cortex -> Void
 * @param Cortex The Cortex instance to clean up.
 */
FORBOCAI_SDK_API void Shutdown(FCortex &Cortex);

} // namespace CortexOps

// Cortex Helpers (Functional)
namespace CortexHelpers {
// Helper to create a lazy cortex
inline CortexTypes::Lazy<FCortex>
createLazyCortex(const FCortexConfig &config) {
  return func::lazy(
      [config]() -> FCortex { return CortexOps::Create(config); });
}

// Helper to create a validation pipeline for cortex configuration
inline CortexTypes::ValidationPipeline<FCortexConfig, FString>
cortexConfigValidationPipeline() {
  return func::validationPipeline<FCortexConfig, FString>()
      .add([](const FCortexConfig &config)
               -> CortexTypes::Either<FString, FCortexConfig> {
        if (config.Model.IsEmpty()) {
          return CortexTypes::make_left(FString(TEXT("Model cannot be empty")),
                                        FCortexConfig{});
        }
        return CortexTypes::make_right(FString(), config);
      })
      .add([](const FCortexConfig &config)
               -> CortexTypes::Either<FString, FCortexConfig> {
        if (config.MaxTokens < 1 || config.MaxTokens > 2048) {
          return CortexTypes::make_left(
              FString(TEXT("Max tokens must be between 1 and 2048")),
              FCortexConfig{});
        }
        return CortexTypes::make_right(FString(), config);
      })
      .add([](const FCortexConfig &config)
               -> CortexTypes::Either<FString, FCortexConfig> {
        if (config.Temperature < 0.0f || config.Temperature > 2.0f) {
          return CortexTypes::make_left(
              FString(TEXT("Temperature must be between 0.0 and 2.0")),
              FCortexConfig{});
        }
        return CortexTypes::make_right(FString(), config);
      });
}

// Helper to create a pipeline for cortex completion
inline CortexTypes::Pipeline<FCortex>
cortexCompletionPipeline(const FCortex &cortex) {
  return func::pipe(cortex);
}

// Helper to create a curried cortex creation function
inline auto curriedCortexCreation()
    -> decltype(func::curry<1>(
        std::function<CortexTypes::CortexCreationResult(FCortexConfig)>())) {
  std::function<CortexTypes::CortexCreationResult(FCortexConfig)> Creator =
      [](FCortexConfig config) -> CortexTypes::CortexCreationResult {
    try {
      FCortex cortex = CortexOps::Create(config);
      return CortexTypes::make_right(FString(), cortex);
    } catch (const std::exception &e) {
      return CortexTypes::make_left(FString(e.what()), FCortex{});
    }
  };
  return func::curry<1>(Creator);
}
} // namespace CortexHelpers

// Factory namespace for consistency with other modules
namespace CortexFactory {
inline FCortex Create(const FCortexConfig &Config) {
  return CortexOps::Create(Config);
}
} // namespace CortexFactory
