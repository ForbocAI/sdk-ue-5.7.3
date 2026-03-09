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
#include "SDKStore.h"
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
  FString Output;
  TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
  FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
  return Output;
}

// ---------------------------------------------------------------------------
// Protocol serializers
// ---------------------------------------------------------------------------

inline FString SerializeIdentifyActorResult(const FNPCActorInfo &Actor) {
  const TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
  ActorObject->SetStringField(TEXT("npcId"), Actor.NpcId);
  ActorObject->SetStringField(TEXT("persona"), Actor.Persona);
  ActorObject->SetStringField(TEXT("data"), SafeStateJson(Actor.Data));

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

inline FString DefaultNodeMemoryPath() {
  return FPaths::ProjectSavedDir() + TEXT("ForbocAI_Memory.sqlite");
}

inline Native::Sqlite::DB EnsureNodeMemoryDatabase() {
  Native::Sqlite::DB &Handle = NodeMemoryHandle();
  if (!Handle) {
    Handle = Native::Sqlite::Open(DefaultNodeMemoryPath());
  }
  return Handle;
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

inline func::AsyncResult<FArweaveUploadResult>
UploadSignedSoul(const FArweaveUploadInstruction &Instruction,
                 const FString &SignedPayload) {
  return func::AsyncResult<FArweaveUploadResult>::create(
      [Instruction, SignedPayload](
          std::function<void(FArweaveUploadResult)> Resolve,
          std::function<void(std::string)> Reject) {
        const FString Url = !Instruction.UploadUrl.IsEmpty()
                                ? Instruction.UploadUrl
                                : Instruction.GatewayUrl;
        if (Url.IsEmpty()) {
          Reject("Missing Arweave upload URL");
          return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
            FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb(TEXT("POST"));
        Request->SetHeader(TEXT("Content-Type"),
                           Instruction.ContentType.IsEmpty()
                               ? TEXT("application/octet-stream")
                               : Instruction.ContentType);
        if (!Instruction.AuiAuthHeader.IsEmpty()) {
          Request->SetHeader(TEXT("Authorization"), Instruction.AuiAuthHeader);
        }
        if (!Instruction.TagsJson.IsEmpty()) {
          Request->SetHeader(TEXT("X-Forboc-Tags"), Instruction.TagsJson);
        }
        Request->SetContentAsString(SignedPayload);
        Request->OnProcessRequestComplete().BindLambda(
            [Instruction, Resolve, Reject](FHttpRequestPtr Req,
                                           FHttpResponsePtr Res,
                                           bool bWasSuccessful) {
              if (!bWasSuccessful || !Res.IsValid()) {
                Reject("Arweave upload failed");
                return;
              }

              FArweaveUploadResult Result;
              Result.ResponseJson = Res->GetContentAsString();
              Result.Status = TEXT("uploaded");
              Result.TxId = Res->GetHeader(TEXT("x-id"));
              if (Result.TxId.IsEmpty()) {
                Result.TxId = LexToString(GetTypeHash(Result.ResponseJson));
              }
              Result.ArweaveUrl = !Instruction.GatewayUrl.IsEmpty()
                                      ? Instruction.GatewayUrl + TEXT("/") +
                                            Result.TxId
                                      : TEXT("");
              Resolve(Result);
            });
        Request->ProcessRequest();
      });
}

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
        Request->OnProcessRequestComplete().BindLambda(
            [Instruction, Resolve, Reject](FHttpRequestPtr Req,
                                           FHttpResponsePtr Res,
                                           bool bWasSuccessful) {
              if (!bWasSuccessful || !Res.IsValid()) {
                Reject("Arweave download failed");
                return;
              }

              FArweaveDownloadResult Result;
              Result.TxId = Instruction.TxId;
              Result.Payload = Res->GetContentAsString();
              Result.Status = TEXT("downloaded");
              Result.ResponseJson = Result.Payload;
              Resolve(Result);
            });
        Request->ProcessRequest();
      });
}

} // namespace detail
} // namespace rtk
