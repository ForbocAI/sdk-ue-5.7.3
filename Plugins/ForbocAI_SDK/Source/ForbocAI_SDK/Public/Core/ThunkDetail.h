#pragma once

#include "API/APISlice.h"
#include "Async/Async.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Misc/Paths.h"
#include "NativeEngine.h"
#include "RuntimeStore.h"
#include "Core/JsonInterop.h"
#include "Serialization/JsonSerializer.h"

namespace rtk {
namespace detail {

// ---------------------------------------------------------------------------
// Native handle singletons
// ---------------------------------------------------------------------------

inline Native::Llama::Context &NodeCortexHandle() {
  static Native::Llama::Context Handle = nullptr;
  return Handle;
}

/** Embedding model (all-MiniLM-L6-v2). Separate from inference model. */
inline Native::Llama::Context &NodeEmbeddingHandle() {
  static Native::Llama::Context Handle = nullptr;
  return Handle;
}

inline Native::Sqlite::DB &NodeMemoryHandle() {
  static Native::Sqlite::DB Handle = nullptr;
  return Handle;
}

// ---------------------------------------------------------------------------
// Async helpers
// ---------------------------------------------------------------------------

template <typename T>
inline func::AsyncResult<T> ResolveAsync(const T &Value) {
  return func::AsyncResult<T>::create(
      [Value](std::function<void(T)> Resolve,
              std::function<void(std::string)> Reject) { Resolve(Value); });
}

template <typename T>
inline func::AsyncResult<T> RejectAsync(const FString &Error) {
  const std::string Utf8Error = TCHAR_TO_UTF8(*Error);
  return func::AsyncResult<T>::create(
      [Utf8Error](std::function<void(T)> Resolve,
                  std::function<void(std::string)> Reject) {
        Reject(Utf8Error);
      });
}

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------

inline FString SafeJson(const FString &Json) {
  return Json.IsEmpty() ? TEXT("{}") : Json;
}

inline FString SafeStateJson(const FAgentState &State) {
  return SafeJson(State.JsonData);
}

inline bool HasStatePayload(const FAgentState &State) {
  return !State.JsonData.IsEmpty() && State.JsonData != TEXT("{}");
}

inline FString JsonObjectToString(const TSharedPtr<FJsonObject> &Object) {
  return JsonInterop::StringifyObject(Object);
}

// ---------------------------------------------------------------------------
// Protocol serializers
// ---------------------------------------------------------------------------

inline FString SerializeIdentifyActorResult(const FNPCActorInfo &Actor) {
  const TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
  ActorObject->SetStringField(TEXT("npcId"), Actor.NpcId);
  ActorObject->SetStringField(TEXT("persona"), Actor.Persona);
  ActorObject->SetObjectField(TEXT("data"), JsonInterop::StateToObject(Actor.Data));

  const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("type"), TEXT("IdentifyActorResult"));
  Root->SetObjectField(TEXT("actor"), ActorObject);
  return JsonObjectToString(Root);
}

inline FString SerializeInferenceResult(const FString &GeneratedOutput) {
  const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("type"), TEXT("ExecuteInferenceResult"));
  Root->SetStringField(TEXT("generatedOutput"), GeneratedOutput);
  return JsonObjectToString(Root);
}

inline FString SerializeQueryVectorResult(const TArray<FMemoryItem> &Memories) {
  TArray<TSharedPtr<FJsonValue>> MemoryValues;
  for (int32 Index = 0; Index < Memories.Num(); ++Index) {
    const FMemoryItem &Memory = Memories[Index];
    const TSharedPtr<FJsonObject> MemoryObject = MakeShared<FJsonObject>();
    MemoryObject->SetStringField(TEXT("text"), Memory.Text);
    MemoryObject->SetStringField(TEXT("type"), Memory.Type);
    MemoryObject->SetNumberField(TEXT("importance"), Memory.Importance);
    MemoryObject->SetNumberField(TEXT("similarity"), Memory.Similarity);
    MemoryValues.Add(MakeShared<FJsonValueObject>(MemoryObject));
  }

  const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("type"), TEXT("QueryVectorResult"));
  Root->SetArrayField(TEXT("memories"), MemoryValues);
  return JsonObjectToString(Root);
}

// ---------------------------------------------------------------------------
// Node memory helpers
// ---------------------------------------------------------------------------

inline FString GetLocalInfrastructureDir() {
  return FPaths::ProjectDir() + TEXT("local_infrastructure/");
}

inline FString DefaultNodeMemoryPath() {
  return GetLocalInfrastructureDir() +
         TEXT("vectors/forbocai_vectors.db");
}

inline FString DefaultEmbeddingModelPath() {
  return GetLocalInfrastructureDir() + TEXT("models/all-MiniLM-L6-v2-Q4_K_M.gguf");
}

inline FString &NodeMemoryPathStorage() {
  static FString Path = DefaultNodeMemoryPath();
  return Path;
}

inline Native::Sqlite::DB EnsureNodeMemoryDatabase() {
  return NodeMemoryHandle();
}

inline FMemoryItem MakeMemoryItem(const FMemoryStoreInstruction &Instruction) {
  FMemoryItem Item;
  Item.Id = FGuid::NewGuid().ToString();
  Item.Text = Instruction.Text;
  Item.Type = Instruction.Type.IsEmpty() ? TEXT("observation")
                                         : Instruction.Type;
  Item.Importance = Instruction.Importance;
  Item.Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
  return Item;
}

// ---------------------------------------------------------------------------
// Response builder
// ---------------------------------------------------------------------------

inline FAgentResponse BuildAgentResponse(const FNPCInstruction &Instruction) {
  FAgentResponse Response;
  Response.Dialogue = Instruction.Dialogue;
  Response.Thought = Instruction.Dialogue;
  if (Instruction.bHasAction) {
    Response.Action = Instruction.Action;
  }
  return Response;
}

// ---------------------------------------------------------------------------
// Arweave helpers
// ---------------------------------------------------------------------------

/**
 * Upload a signed soul payload to Arweave.
 *
 * Matches TS SDK handler_ArweaveUpload behaviour:
 *  - 3 attempts max (configurable via MaxRetries)
 *  - Exponential backoff between retries: 250ms * 2^(attempt-1)
 *  - 60-second HTTP timeout per attempt
 *  - txId derivation order:
 *      1. (UE-only enhancement) x-id response header
 *      2. Response JSON ".id" field        (same as TS)
 *      3. Generated hash fallback          (same as TS)
 *  - On exhausted retries, resolves with error result (does NOT reject)
 */
inline func::AsyncResult<FArweaveUploadResult>
UploadSignedSoul(const FArweaveUploadInstruction &Instruction,
                 const FString &SignedPayload,
                 int32 MaxRetries = 3) {
  return func::AsyncResult<FArweaveUploadResult>::create(
      [Instruction, SignedPayload, MaxRetries](
          std::function<void(FArweaveUploadResult)> Resolve,
          std::function<void(std::string)> Reject) {
        const FString Url = !Instruction.UploadUrl.IsEmpty()
                                ? Instruction.UploadUrl
                                : Instruction.GatewayUrl;
        const FString Payload =
            !Instruction.PayloadJson.IsEmpty() ? Instruction.PayloadJson
                                               : SignedPayload;
        if (Url.IsEmpty()) {
          Reject("Missing Arweave upload URL");
          return;
        }

        // Shared mutable attempt counter, accessed from the async task.
        struct FRetryState {
          int32 Attempt = 0;
          int32 MaxRetries = 3;
          FString Url;
          FString Payload;
          FArweaveUploadInstruction Instruction;
          std::function<void(FArweaveUploadResult)> Resolve;
          std::function<void(std::string)> Reject;
        };

        TSharedPtr<FRetryState> State = MakeShared<FRetryState>();
        State->MaxRetries = MaxRetries;
        State->Url = Url;
        State->Payload = Payload;
        State->Instruction = Instruction;
        State->Resolve = Resolve;
        State->Reject = Reject;

        // Forward-declared so the lambda can reference itself for retries.
        TSharedPtr<TFunction<void()>> TryOnce = MakeShared<TFunction<void()>>();
        *TryOnce = [State, TryOnce]() {
          State->Attempt += 1;

          TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
              FHttpModule::Get().CreateRequest();
          Request->SetURL(State->Url);
          Request->SetVerb(TEXT("POST"));
          Request->SetTimeout(60.0f); // 60-second timeout (matches TS SDK)
          Request->SetHeader(TEXT("Content-Type"),
                             State->Instruction.ContentType.IsEmpty()
                                 ? TEXT("application/octet-stream")
                                 : State->Instruction.ContentType);
          if (!State->Instruction.AuiAuthHeader.IsEmpty()) {
            Request->SetHeader(TEXT("Authorization"),
                               State->Instruction.AuiAuthHeader);
          }
          if (!State->Instruction.TagsJson.IsEmpty()) {
            Request->SetHeader(TEXT("X-Forboc-Tags"),
                               State->Instruction.TagsJson);
          }
          Request->SetContentAsString(State->Payload);

          Request->OnProcessRequestComplete().BindLambda(
              [State, TryOnce](FHttpRequestPtr Req, FHttpResponsePtr Res,
                               bool bWasSuccessful) {
                // --- Network-level failure (timeout, DNS, connection reset) ---
                if (!bWasSuccessful || !Res.IsValid()) {
                  if (State->Attempt < State->MaxRetries) {
                    const float BackoffSec =
                        0.250f *
                        FMath::Pow(2.0f,
                                   static_cast<float>(State->Attempt - 1));
                    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
                              [State, TryOnce, BackoffSec]() {
                                FPlatformProcess::Sleep(BackoffSec);
                                AsyncTask(ENamedThreads::GameThread,
                                          [TryOnce]() { (*TryOnce)(); });
                              });
                    return;
                  }
                  // All retries exhausted — resolve with error (not reject,
                  // matching TS SDK).
                  FArweaveUploadResult Result;
                  Result.StatusCode = 0;
                  Result.bSuccess = false;
                  Result.Status = TEXT("0");
                  Result.Error = TEXT("upload_request_failed:network_error");
                  Result.TxId = FString::Printf(
                      TEXT("ar_tx_failed_%lld"), FDateTime::UtcNow().ToUnixTimestamp());
                  State->Resolve(Result);
                  return;
                }

                // --- Got an HTTP response ---
                FArweaveUploadResult Result;
                Result.ResponseJson = Res->GetContentAsString();
                Result.StatusCode = Res->GetResponseCode();
                Result.bSuccess =
                    Result.StatusCode >= 200 && Result.StatusCode < 300;
                Result.Status = FString::FromInt(Result.StatusCode);
                Result.Error =
                    Result.bSuccess
                        ? TEXT("")
                        : FString::Printf(TEXT("upload_failed_status_%d"),
                                          Result.StatusCode);

                // Non-2xx: retry if attempts remain.
                if (!Result.bSuccess &&
                    State->Attempt < State->MaxRetries) {
                  const float BackoffSec =
                      0.250f *
                      FMath::Pow(2.0f,
                                 static_cast<float>(State->Attempt - 1));
                  AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
                            [State, TryOnce, BackoffSec]() {
                              FPlatformProcess::Sleep(BackoffSec);
                              AsyncTask(ENamedThreads::GameThread,
                                        [TryOnce]() { (*TryOnce)(); });
                            });
                  return;
                }

                // --- Derive txId ---
                // 1. UE-only enhancement: check x-id response header.
                Result.TxId = Res->GetHeader(TEXT("x-id"));
                // 2. Response JSON ".id" field (matches TS SDK).
                if (Result.TxId.IsEmpty()) {
                  TSharedPtr<FJsonObject> ResponseObject;
                  if (JsonInterop::ParseJsonObject(Result.ResponseJson,
                                                   ResponseObject) &&
                      ResponseObject.IsValid()) {
                    Result.TxId =
                        ResponseObject->GetStringField(TEXT("id"));
                  }
                }
                // 3. Generated fallback (matches TS SDK).
                if (Result.TxId.IsEmpty()) {
                  Result.TxId =
                      LexToString(GetTypeHash(Result.ResponseJson));
                }

                Result.ArweaveUrl =
                    !State->Instruction.GatewayUrl.IsEmpty()
                        ? State->Instruction.GatewayUrl + TEXT("/") +
                              Result.TxId
                        : TEXT("");
                State->Resolve(Result);
              });
          Request->ProcessRequest();
        };

        // Fire the first attempt.
        (*TryOnce)();
      });
}

/**
 * Download a soul payload from Arweave.
 *
 * Matches TS SDK handler_ArweaveDownload behaviour:
 *  - 60-second HTTP timeout
 *  - No retry (single attempt)
 *  - Returns structured result with status/success/error fields
 */
inline func::AsyncResult<FArweaveDownloadResult>
DownloadSoulPayload(const FArweaveDownloadInstruction &Instruction) {
  return func::AsyncResult<FArweaveDownloadResult>::create(
      [Instruction](std::function<void(FArweaveDownloadResult)> Resolve,
                    std::function<void(std::string)> Reject) {
        FString Url = Instruction.DownloadUrl;
        if (Url.IsEmpty() && !Instruction.GatewayUrl.IsEmpty() &&
            !Instruction.TxId.IsEmpty()) {
          Url = Instruction.GatewayUrl + TEXT("/") + Instruction.TxId;
        }

        if (Url.IsEmpty()) {
          Reject("Missing Arweave download URL");
          return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
            FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb(TEXT("GET"));
        Request->SetTimeout(60.0f); // 60-second timeout (matches TS SDK)
        Request->OnProcessRequestComplete().BindLambda(
            [Instruction, Resolve, Reject](FHttpRequestPtr Req,
                                           FHttpResponsePtr Res,
                                           bool bWasSuccessful) {
              if (!bWasSuccessful || !Res.IsValid()) {
                Reject("Arweave download failed");
                return;
              }

              FArweaveDownloadResult Result;
              Result.TxId = !Instruction.ExpectedTxId.IsEmpty()
                                ? Instruction.ExpectedTxId
                                : Instruction.TxId;
              Result.Payload = Res->GetContentAsString();
              Result.BodyJson = Result.Payload;
              Result.StatusCode = Res->GetResponseCode();
              Result.bSuccess =
                  Result.StatusCode >= 200 && Result.StatusCode < 300;
              Result.Status = FString::FromInt(Result.StatusCode);
              Result.Error = Result.bSuccess
                                 ? TEXT("")
                                 : FString::Printf(TEXT("download_failed_status_%d"),
                                                   Result.StatusCode);
              Result.ResponseJson = Result.Payload;
              Resolve(Result);
            });
        Request->ProcessRequest();
      });
}

} // namespace detail
} // namespace rtk
