#include "Cortex/CortexSlice.h"
#include "Types.h"
#include "SDKStore.h"

#include "Core/functional_core.hpp"
#include "Cortex/CortexModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_FORBOC_NATIVE
#include "llama.h"
#endif

#include "NativeEngine.h"

// ==========================================
// Cortex Operations — Stateless free functions
// ==========================================

FCortex CortexOps::Create(const FCortexConfig &Config) {
  FCortex cortex;
  cortex.EngineHandle = nullptr; // Initialize with null handle
  return cortex;
}

TFuture<CortexTypes::CortexInitResult> CortexOps::Init(FCortex &Cortex) {
  // Validate configuration
  auto configValidation = Internal::ValidateConfig(Cortex.Config);
  if (configValidation.isLeft) {
    TPromise<CortexTypes::CortexInitResult> Promise;
    Promise.SetValue(CortexTypes::make_left(configValidation.leftValue));
    return Promise.GetFuture();
  }

  auto SDKStore = ConfigureSDKStore();
  SDKStore.dispatch(CortexActions::CortexInitPending());

  return Async(
      EAsyncExecution::Thread,
      [&Cortex, SDKStore]() -> CortexTypes::CortexInitResult {
        try {
          // Native Initialization logic
          bool bSuccess = true;
          if (bSuccess) {
            SDKStore.dispatch(CortexActions::CortexInitFulfilled(Cortex));
            return CortexTypes::make_right(FString(), true);
          } else {
            SDKStore.dispatch(
                CortexActions::CortexInitRejected(TEXT("Init failed")));
            return CortexTypes::make_left(
                TEXT("Inference initialization failed"));
          }
        } catch (const std::exception &e) {
          SDKStore.dispatch(
              CortexActions::CortexInitRejected(FString(e.what())));
          return CortexTypes::make_left(FString(e.what()));
        }
      });
}

TFuture<CortexTypes::CortexCompletionResult>
CortexOps::Complete(const FCortex &Cortex, const FString &Prompt,
                    const TMap<FString, FString> &Context) {
  if (Cortex.EngineHandle == nullptr) {
    TPromise<CortexTypes::CortexCompletionResult> Promise;
    Promise.SetValue(
        CortexTypes::make_left(FString(TEXT("Cortex engine not initialized"))));
    return Promise.GetFuture();
  }

  // Offload SLM inference to a background thread to prevent hitching
  return Async(
      EAsyncExecution::Thread,
      [Cortex, Prompt]() -> CortexTypes::CortexCompletionResult {
        try {
          // Offload generation to background thread
          auto SDKStore = ConfigureSDKStore();
          SDKStore.dispatch(CortexActions::CortexCompletePending(Prompt));

          // Mock Inference
          FCortexResponse Response;
          Response.Id = FString::Printf(TEXT("res_%llu"),
                                        FDateTime::Now().ToUnixTimestamp());
          Response.Text =
              FString::Printf(TEXT("Generated response for: %s"), *Prompt);
          Response.Stats = TEXT("Inference took 150ms");

          SDKStore.dispatch(CortexActions::CortexCompleteFulfilled(Response));
          return CortexTypes::make_right(FString(), Response);
        } catch (const std::exception &e) {
          return CortexTypes::make_left(FString(e.what()));
        }
      });
}

CortexTypes::CortexStreamResult
CortexOps::CompleteStream(const FCortex &Cortex, const FString &Prompt,
                          const TMap<FString, FString> &Context) {
  try {
    // Stream text generation
    TArray<FString> chunks;
    chunks.Add(TEXT("Chunk 1: Starting generation..."));
    chunks.Add(TEXT("Chunk 2: Continuing generation..."));
    chunks.Add(TEXT("Chunk 3: Finalizing generation..."));

    return CortexTypes::make_right(FString(), chunks);
  } catch (const std::exception &e) {
    return CortexTypes::make_left(FString(e.what()));
  }
}

FString CortexOps::GetStatus(const FCortex &Cortex) {
  if (Cortex.EngineHandle == nullptr) {
    return TEXT("Cortex engine not initialized");
  }

  return TEXT("Cortex engine running");
}

void CortexOps::Shutdown(FCortex &Cortex) {
  if (Cortex.EngineHandle != nullptr) {
    Native::Llama::FreeModel(Cortex.EngineHandle);
    Cortex.EngineHandle = nullptr;
  }
}

// Functional helper implementations
namespace CortexHelpers {
// Implementation of lazy cortex creation
CortexTypes::Lazy<FCortex> createLazyCortex(const FCortexConfig &config) {
  return func::lazy(
      [config]() -> FCortex { return CortexFactory::Create(config); });
}

// Implementation of cortex config validation pipeline
CortexTypes::ValidationPipeline<FCortexConfig>
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

// Implementation of cortex completion pipeline
CortexTypes::Pipeline<FCortex> cortexCompletionPipeline(const FCortex &cortex) {
  return func::pipe(cortex);
}

// Implementation of curried cortex creation
CortexTypes::Curried<
    1, std::function<CortexTypes::CortexCreationResult(FCortexConfig)>>
curriedCortexCreation() {
  return func::curry<1>(
      [](FCortexConfig config) -> CortexTypes::CortexCreationResult {
        try {
          FCortex cortex = CortexFactory::Create(config);
          return CortexTypes::make_right(FString(), cortex);
        } catch (const std::exception &e) {
          return CortexTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace CortexHelpers