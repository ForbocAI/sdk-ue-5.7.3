#include "Cortex/CortexModule.h"
#include <memory>
#include "Cortex/CortexSlice.h"
#include "Cortex/CortexThunks.h"
#include "Core/functional_core.hpp"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RuntimeStore.h"
#include "Types.h"

#include "NativeEngine.h"

// ==========================================
// Cortex Operations — Thin facade over cortex thunks
// G.5: Model init/inference routed through Cortex Slice + Thunks
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
  auto configValidation =
      CortexHelpers::cortexConfigValidationPipeline().run(Cortex.Config);
  if (configValidation.isLeft) {
    TPromise<CortexTypes::CortexInitResult> Promise;
    Promise.SetValue(CortexTypes::make_left(configValidation.left, false));
    return Promise.GetFuture();
  }

  auto PromisePtr =
      std::make_shared<TPromise<CortexTypes::CortexInitResult>>();
  auto Future = PromisePtr->GetFuture();
  FCortex *CortexPtr = &Cortex;

  auto Store = ConfigureStore();
  Store.dispatch(rtk::initNodeCortexThunk(Cortex.Config.Model))
      .then([CortexPtr, PromisePtr](const FCortexStatus &Status) mutable {
        CortexPtr->bReady = Status.bReady;
        PromisePtr->SetValue(CortexTypes::make_right(FString(), Status.bReady));
      })
      .catch_([PromisePtr](std::string Error) mutable {
        PromisePtr->SetValue(
            CortexTypes::make_left(FString(UTF8_TO_TCHAR(Error.c_str())),
                                   false));
      })
      .execute();

  return Future;
}

TFuture<CortexTypes::CortexCompletionResult>
CortexOps::Complete(const FCortex &Cortex, const FString &Prompt,
                    const TMap<FString, FString> &Context) {
  static_cast<void>(Context);
  auto PromisePtr =
      std::make_shared<TPromise<CortexTypes::CortexCompletionResult>>();
  auto Future = PromisePtr->GetFuture();

  auto Store = ConfigureStore();
  Store.dispatch(rtk::completeNodeCortexThunk(Prompt, Cortex.Config))
      .then([PromisePtr](const FCortexResponse &Response) mutable {
        PromisePtr->SetValue(CortexTypes::make_right(FString(), Response));
      })
      .catch_([PromisePtr](std::string Error) mutable {
        PromisePtr->SetValue(
            CortexTypes::make_left(FString(UTF8_TO_TCHAR(Error.c_str())),
                                   FCortexResponse{}));
      })
      .execute();

  return Future;
}

TFuture<CortexTypes::CortexCompletionResult>
CortexOps::CompleteStream(const FCortex &Cortex, const FString &Prompt,
                          const FOnCortexToken &OnToken,
                          const TMap<FString, FString> &Context) {
  static_cast<void>(Context);
  auto PromisePtr =
      std::make_shared<TPromise<CortexTypes::CortexCompletionResult>>();
  auto Future = PromisePtr->GetFuture();

  auto Store = ConfigureStore();
  Store.dispatch(rtk::streamNodeCortexThunk(Prompt, Cortex.Config, OnToken))
      .then([PromisePtr](const FCortexResponse &Response) mutable {
        PromisePtr->SetValue(CortexTypes::make_right(FString(), Response));
      })
      .catch_([PromisePtr](std::string Error) mutable {
        PromisePtr->SetValue(
            CortexTypes::make_left(FString(UTF8_TO_TCHAR(Error.c_str())),
                                   FCortexResponse{}));
      })
      .execute();

  return Future;
}

FString CortexOps::GetStatus(const FCortex &Cortex) {
  if (!Cortex.bReady) {
    return TEXT("Cortex engine not initialized");
  }
  return TEXT("Cortex engine running");
}

void CortexOps::Shutdown(FCortex &Cortex) {
  (void)Cortex;
  // Shutdown is handled by initNodeCortexThunk (frees previous handle before
  // loading new model). No explicit shutdown thunk; NodeCortexHandle persists.
}
