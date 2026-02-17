#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "CortexModule.generated.h"
#include "ForbocAI_SDK_Types.h"

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

/**
 * Cortex Engine Handle — Opaque handle to the inference engine.
 * Note: Not a USTRUCT because void* is not supported by reflection.
 */
struct FCortex {
  void *EngineHandle;
  FCortex() : EngineHandle(nullptr) {}
};

/**
 * Cortex Configuration — Immutable data.
 */
struct FCortexConfig {
  /** The model identifier to use. */
  const FString Model;
  /** Whether to use GPU acceleration if available. */
  const bool UseGPU;
  /** Maximum tokens to generate per request. */
  const int32 MaxTokens;
  /** Temperature for generation (0.0 - 1.0). */
  const float Temperature;
  /** Top-k sampling parameter. */
  const int32 TopK;
  /** Top-p sampling parameter. */
  const float TopP;

  FCortexConfig()
      : Model(TEXT("smalll2-135m")), UseGPU(false), MaxTokens(512),
        Temperature(0.7f), TopK(40), TopP(0.9f) {}
};

/**
 * Cortex Response — Immutable data.
 */
USTRUCT()
struct FCortexResponse {
  GENERATED_BODY()

  /** The generated text response. */
  UPROPERTY()
  FString Text;

  /** The estimated token count. */
  UPROPERTY()
  int32 TokenCount;

  /** Whether the generation completed successfully. */
  UPROPERTY()
  bool bSuccess;

  /** Error message if generation failed. */
  UPROPERTY()
  FString ErrorMessage;

  FCortexResponse() : TokenCount(0), bSuccess(false), ErrorMessage(TEXT("")) {}
};

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
 * Initializes the Cortex engine and loads the model.
 * Pure function: Cortex -> Result
 * @param Cortex The Cortex instance to initialize.
 * @return A validation result indicating success or failure.
 */
FORBOCAI_SDK_API CortexTypes::CortexInitResult Init(FCortex &Cortex);

/**
 * Generates text completion from prompt.
 * Pure function: (Cortex, Prompt, Context) -> Response
 * @param Cortex The Cortex instance to use.
 * @param Prompt The input prompt text.
 * @param Context Optional context data.
 * @return The generated response.
 */
FORBOCAI_SDK_API CortexTypes::CortexCompletionResult
Complete(const FCortex &Cortex, const FString &Prompt,
         const TMap<FString, FString> &Context = TMap<FString, FString>());

/**
 * Streams text generation character by character.
 * Pure function: (Cortex, Prompt, Context) -> Stream
 * @param Cortex The Cortex instance to use.
 * @param Prompt The input prompt text.
 * @param Context Optional context data.
 * @return A stream of generated text chunks.
 */
FORBOCAI_SDK_API CortexTypes::CortexStreamResult CompleteStream(
    const FCortex &Cortex, const FString &Prompt,
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
inline CortexTypes::ValidationPipeline<FCortexConfig>
cortexConfigValidationPipeline() {
  return func::validationPipeline<FCortexConfig>()
      .add([](const FCortexConfig &config)
               -> CortexTypes::Either<FString, FCortexConfig> {
        if (config.Model.IsEmpty()) {
          return CortexTypes::make_left(FString(TEXT("Model cannot be empty")));
        }
        return CortexTypes::make_right(config);
      })
      .add([](const FCortexConfig &config)
               -> CortexTypes::Either<FString, FCortexConfig> {
        if (config.MaxTokens < 1 || config.MaxTokens > 2048) {
          return CortexTypes::make_left(
              FString(TEXT("Max tokens must be between 1 and 2048")));
        }
        return CortexTypes::make_right(config);
      })
      .add([](const FCortexConfig &config)
               -> CortexTypes::Either<FString, FCortexConfig> {
        if (config.Temperature < 0.0f || config.Temperature > 2.0f) {
          return CortexTypes::make_left(
              FString(TEXT("Temperature must be between 0.0 and 2.0")));
        }
        return CortexTypes::make_right(config);
      });
}

// Helper to create a pipeline for cortex completion
inline CortexTypes::Pipeline<FCortex>
cortexCompletionPipeline(const FCortex &cortex) {
  return func::pipe(cortex);
}

// Helper to create a curried cortex creation function
inline CortexTypes::Curried<
    1, std::function<CortexTypes::CortexCreationResult(FCortexConfig)>>
curriedCortexCreation() {
  return func::curry<1>(
      [](FCortexConfig config) -> CortexTypes::CortexCreationResult {
        try {
          FCortex cortex = CortexOps::Create(config);
          return CortexTypes::make_right(FString(), cortex);
        } catch (const std::exception &e) {
          return CortexTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace CortexHelpers

// Factory namespace for consistency with other modules
namespace CortexFactory {
inline FCortex Create(const FCortexConfig &Config) {
  return CortexOps::Create(Config);
}
} // namespace CortexFactory
