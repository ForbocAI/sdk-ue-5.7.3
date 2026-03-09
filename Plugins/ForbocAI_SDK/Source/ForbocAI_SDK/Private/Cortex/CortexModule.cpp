#include "Cortex/CortexModule.h"
#include "Cortex/CortexSlice.h"
#include "Core/functional_core.hpp"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RuntimeStore.h"
#include "Types.h"

#if WITH_FORBOC_NATIVE
#include "llama.h"
#endif

#include "NativeEngine.h"

// ==========================================
// Cortex Operations — Stateless free functions
// ==========================================

FCortex CortexOps::Create(const FCortexConfig &Config) {
  FCortex cortex;
  cortex.Config = Config;
  cortex.EngineHandle = nullptr;
  cortex.Id = TEXT("local-llama");
  cortex.bReady = false;
  return cortex;
}

TFuture<CortexTypes::CortexInitResult> CortexOps::Init(FCortex &Cortex) {
  // Validate configuration
  auto configValidation =
      CortexHelpers::cortexConfigValidationPipeline().run(Cortex.Config);
  if (configValidation.isLeft) {
    TPromise<CortexTypes::CortexInitResult> Promise;
    Promise.SetValue(CortexTypes::make_left(configValidation.left, false));
    return Promise.GetFuture();
  }

  auto SDKStore = ConfigureSDKStore();
  SDKStore.dispatch(CortexSlice::Actions::CortexInitPending(Cortex.Config.Model));

  return Async(
      EAsyncExecution::Thread,
      [&Cortex, SDKStore]() -> CortexTypes::CortexInitResult {
        try {
          Cortex.EngineHandle = Native::Llama::LoadModel(Cortex.Config.Model);
          Cortex.bReady = (Cortex.EngineHandle != nullptr);

          FCortexStatus Status;
          Status.Id = Cortex.Id;
          Status.Model = Cortex.Config.Model;
          Status.bReady = Cortex.bReady;
          Status.Engine = ECortexEngine::NodeLlamaCpp;
          Status.DownloadProgress = Cortex.bReady ? 1.0f : 0.0f;

          if (Cortex.bReady) {
            SDKStore.dispatch(CortexSlice::Actions::CortexInitFulfilled(Status));
            return CortexTypes::make_right(FString(), true);
          } else {
            Status.Error = TEXT("Init failed");
            SDKStore.dispatch(
                CortexSlice::Actions::CortexInitRejected(Status.Error));
            return CortexTypes::make_left(
                FString(TEXT("Inference initialization failed")), false);
          }
        } catch (const std::exception &e) {
          SDKStore.dispatch(
              CortexSlice::Actions::CortexInitRejected(FString(e.what())));
          return CortexTypes::make_left(FString(e.what()), false);
        }
      });
}

TFuture<CortexTypes::CortexCompletionResult>
CortexOps::Complete(const FCortex &Cortex, const FString &Prompt,
                    const TMap<FString, FString> &Context) {
  if (Cortex.EngineHandle == nullptr) {
    TPromise<CortexTypes::CortexCompletionResult> Promise;
    Promise.SetValue(
        CortexTypes::make_left(FString(TEXT("Cortex engine not initialized")),
                               FCortexResponse{}));
    return Promise.GetFuture();
  }

  // Offload SLM inference to a background thread to prevent hitching
  return Async(
      EAsyncExecution::Thread,
      [Cortex, Prompt]() -> CortexTypes::CortexCompletionResult {
        try {
          auto SDKStore = ConfigureSDKStore();
          SDKStore.dispatch(
              CortexSlice::Actions::CortexCompletePending(Prompt));

          FCortexResponse Response;
          Response.Id = FString::Printf(TEXT("res_%llu"),
                                        FDateTime::Now().ToUnixTimestamp());
          Response.Text = Native::Llama::Infer(
              Cortex.EngineHandle, Prompt, Cortex.Config.MaxTokens);
          Response.Stats = TEXT("Inference completed");

          SDKStore.dispatch(
              CortexSlice::Actions::CortexCompleteFulfilled(Response));
          return CortexTypes::make_right(FString(), Response);
        } catch (const std::exception &e) {
          return CortexTypes::make_left(FString(e.what()), FCortexResponse{});
        }
      });
}

CortexTypes::CortexStreamResult
CortexOps::CompleteStream(const FCortex &Cortex, const FString &Prompt,
                          const TMap<FString, FString> &Context) {
  static_cast<void>(Context);
  try {
    if (Cortex.EngineHandle == nullptr) {
      return CortexTypes::make_left(
          FString(TEXT("Cortex engine not initialized")), TArray<FString>{});
    }

    const FString Generated =
        Native::Llama::Infer(Cortex.EngineHandle, Prompt, Cortex.Config);
    TArray<FString> chunks;
    if (Generated.IsEmpty()) {
      chunks.Add(TEXT(""));
      return CortexTypes::make_right(FString(), chunks);
    }

    constexpr int32 ChunkSize = 64;
    for (int32 Offset = 0; Offset < Generated.Len(); Offset += ChunkSize) {
      chunks.Add(Generated.Mid(Offset, ChunkSize));
    }

    return CortexTypes::make_right(FString(), chunks);
  } catch (const std::exception &e) {
    return CortexTypes::make_left(FString(e.what()), TArray<FString>{});
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
