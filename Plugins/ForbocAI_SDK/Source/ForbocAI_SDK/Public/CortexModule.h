#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "CortexModule.generated.h"

// ==========================================================
// Cortex Module — Local SLM Inference (UE SDK)
// ==========================================================
// Strict functional programming implementation for local
// Small Language Model inference using sqlite-vss.
// All operations are pure free functions.
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
FORBOCAI_SDK_API FValidationResult Init(FCortex &Cortex);

/**
 * Generates text completion from prompt.
 * Pure function: (Cortex, Prompt, Context) -> Response
 * @param Cortex The Cortex instance to use.
 * @param Prompt The input prompt text.
 * @param Context Optional context data.
 * @return The generated response.
 */
FORBOCAI_SDK_API FCortexResponse
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
FORBOCAI_SDK_API TArray<FString> CompleteStream(
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