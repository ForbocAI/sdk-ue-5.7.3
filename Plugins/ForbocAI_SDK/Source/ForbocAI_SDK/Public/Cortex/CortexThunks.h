#pragma once

#include "Core/ThunkDetail.h"
#include "Cortex/CortexSlice.h"
#include "Errors.h"
#include "SDKConfig.h"

namespace rtk {

// Forward declarations for cross-thunk references
inline ThunkAction<FCortexResponse, FSDKState>
completeNodeCortexThunk(const FString &Prompt,
                        const FCortexConfig &Config = FCortexConfig());

inline ThunkAction<FCortexResponse, FSDKState>
completeRemoteThunk(const FString &CortexId, const FString &Prompt,
                    const FCortexConfig &Config);

// ---------------------------------------------------------------------------
// Node cortex thunks (mirrors TS nodeCortexSlice.ts)
// ---------------------------------------------------------------------------

inline ThunkAction<FCortexStatus, FSDKState>
initNodeCortexThunk(const FString &ModelPath) {
  return [ModelPath](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexStatus> {
    Dispatch(CortexSlice::Actions::CortexInitPending(ModelPath));

    return func::AsyncResult<FCortexStatus>::create(
        [ModelPath, Dispatch](std::function<void(FCortexStatus)> Resolve,
                              std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [ModelPath, Dispatch, Resolve, Reject]() {
            Native::Llama::Context &Handle = detail::NodeCortexHandle();
            if (Handle) {
              Native::Llama::FreeModel(Handle);
              Handle = nullptr;
            }

            Handle = Native::Llama::LoadModel(ModelPath);

            FCortexStatus Status;
            Status.Id = TEXT("local-llama");
            Status.Model = ModelPath;
            Status.bReady = (Handle != nullptr);
            Status.Engine = ECortexEngine::NodeLlamaCpp;
            Status.DownloadProgress = Status.bReady ? 1.0f : 0.0f;
            Status.Error = Status.bReady ? TEXT("") : TEXT("Failed to load model");

            AsyncTask(ENamedThreads::GameThread, [Dispatch, Resolve, Reject, Status]() {
              if (Status.bReady) {
                Dispatch(CortexSlice::Actions::CortexInitFulfilled(Status));
                Resolve(Status);
              } else {
                Dispatch(CortexSlice::Actions::CortexInitRejected(Status.Error));
                Reject(TCHAR_TO_UTF8(*Status.Error));
              }
            });
          });
        });
  };
}

inline ThunkAction<FCortexResponse, FSDKState>
completeNodeCortexThunk(const FString &Prompt, const FCortexConfig &Config) {
  return [Prompt, Config](std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexResponse> {
    Dispatch(CortexSlice::Actions::CortexCompletePending(Prompt));

    return func::AsyncResult<FCortexResponse>::create(
        [Prompt, Config, Dispatch](std::function<void(FCortexResponse)> Resolve,
                                   std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Prompt, Config, Dispatch, Resolve, Reject]() {
            Native::Llama::Context Handle = detail::NodeCortexHandle();
            if (!Handle) {
              detail::NodeCortexHandle() =
                  Native::Llama::LoadModel(Config.Model.IsEmpty()
                                               ? TEXT("local-default")
                                               : Config.Model);
              Handle = detail::NodeCortexHandle();
            }

            if (!Handle) {
              const FString Error = TEXT("Local cortex is not initialized");
              AsyncTask(ENamedThreads::GameThread, [Dispatch, Reject, Error]() {
                Dispatch(CortexSlice::Actions::CortexCompleteRejected(Error));
                Reject(TCHAR_TO_UTF8(*Error));
              });
              return;
            }

            FCortexResponse Response;
            Response.Id = FGuid::NewGuid().ToString();
            Response.Text = Native::Llama::Infer(Handle, Prompt, Config);
            Response.Stats = TEXT("local-node");

            AsyncTask(ENamedThreads::GameThread, [Dispatch, Resolve, Response]() {
              Dispatch(CortexSlice::Actions::CortexCompleteFulfilled(Response));
              Resolve(Response);
            });
          });
        });
  };
}

inline ThunkAction<TArray<float>, FSDKState>
generateNodeEmbeddingThunk(const FString &Text) {
  return [Text](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<float>> {
    return func::AsyncResult<TArray<float>>::create(
        [Text](std::function<void(TArray<float>)> Resolve,
               std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Text, Resolve]() {
            TArray<float> Embedding =
                Native::Llama::Embed(detail::NodeCortexHandle(), Text);
            AsyncTask(ENamedThreads::GameThread,
                      [Resolve, Embedding]() { Resolve(Embedding); });
          });
        });
  };
}

// ---------------------------------------------------------------------------
// Remote cortex thunks (mirrors TS cortexSlice.ts)
// ---------------------------------------------------------------------------

inline ThunkAction<TArray<FCortexModelInfo>, FSDKState>
listCortexModelsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FCortexModelInfo>> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<TArray<FCortexModelInfo>>(ApiKeyError.value);
    }
    return APISlice::Endpoints::getCortexModels()(Dispatch, GetState);
  };
}

inline ThunkAction<FCortexStatus, FSDKState>
initRemoteCortexThunk(const FString &Model = TEXT("api-integrated"),
                      const FString &AuthKey = TEXT("")) {
  return [Model, AuthKey](std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexStatus> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FCortexStatus>(ApiKeyError.value);
    }

    Dispatch(CortexSlice::Actions::CortexInitPending(Model));

    return func::AsyncChain::then<FCortexInitResponse, FCortexStatus>(
        APISlice::Endpoints::postCortexInit(
            TypeFactory::CortexInitRequest(Model, AuthKey))(Dispatch, GetState),
        [Model, Dispatch](const FCortexInitResponse &Response) {
          FCortexStatus Status;
          Status.Id = Response.CortexId;
          Status.Model = Model;
          Status.bReady = Response.State.Equals(TEXT("ready"),
                                                ESearchCase::IgnoreCase);
          Status.Engine = ECortexEngine::Remote;
          Status.DownloadProgress = Status.bReady ? 1.0f : 0.0f;
          Status.Error = Status.bReady ? TEXT("") : Response.State;
          if (Status.bReady) {
            Dispatch(CortexSlice::Actions::CortexInitFulfilled(Status));
            return detail::ResolveAsync(Status);
          }
          Dispatch(CortexSlice::Actions::CortexInitRejected(Status.Error));
          return detail::RejectAsync<FCortexStatus>(
              Status.Error.IsEmpty() ? TEXT("Remote cortex init failed")
                                     : Status.Error);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(CortexSlice::Actions::CortexInitRejected(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FCortexResponse, FSDKState>
completeRemoteThunk(const FString &CortexId, const FString &Prompt) {
  return completeRemoteThunk(CortexId, Prompt, FCortexConfig());
}

inline ThunkAction<FCortexResponse, FSDKState>
completeRemoteThunk(const FString &CortexId, const FString &Prompt,
                    const FCortexConfig &Config) {
  return [CortexId, Prompt, Config](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexResponse> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FCortexResponse>(ApiKeyError.value);
    }

    Dispatch(CortexSlice::Actions::CortexCompletePending(Prompt));
    return func::AsyncChain::then<FCortexResponse, FCortexResponse>(
        APISlice::Endpoints::postCortexComplete(
            CortexId,
            TypeFactory::CortexCompleteRequest(
                Prompt, Config.MaxTokens, Config.Temperature, Config.Stop,
                Config.JsonSchemaJson))(Dispatch, GetState),
        [Dispatch](const FCortexResponse &Response) {
          Dispatch(CortexSlice::Actions::CortexCompleteFulfilled(Response));
          return detail::ResolveAsync(Response);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(CortexSlice::Actions::CortexCompleteRejected(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

} // namespace rtk
