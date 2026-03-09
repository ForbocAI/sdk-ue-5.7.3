#pragma once

#include "../Core/AsyncHttp.h"
#include "../Core/JsonInterop.h"
#include "../Core/rtk.hpp"
#include "../RuntimeConfig.h"
#include "../Types.h"
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

struct FSDKState;

namespace APISlice {

using namespace rtk;

struct FAPIState {
  FString LastEndpoint;
  FString Status;
  FString Error;

  FAPIState() : Status(TEXT("idle")) {}
};

extern rtk::ApiSlice<FSDKState> ForbocAiApi;

namespace Detail {

template <typename T> inline FString ToJson(const T &Value) {
  FString Json;
  FJsonObjectConverter::UStructToJsonObjectString(Value, Json);
  return Json;
}

inline FString Encode(const FString &Value) {
  return FGenericPlatformHttp::UrlEncode(Value);
}

inline FString ToJsonString(const TSharedRef<FJsonObject> &Object) {
  FString Json;
  const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
  FJsonSerializer::Serialize(Object, Writer);
  return Json;
}

inline FString BuildCortexCompletePayload(const FCortexCompleteRequest &Request) {
  const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
  Payload->SetStringField(TEXT("prompt"), Request.Prompt);

  if (Request.MaxTokens >= 0) {
    Payload->SetNumberField(TEXT("maxTokens"), Request.MaxTokens);
  }

  if (Request.Temperature >= 0.0f) {
    Payload->SetNumberField(TEXT("temperature"), Request.Temperature);
  }

  if (Request.Stop.Num() > 0) {
    TArray<TSharedPtr<FJsonValue>> StopValues;
    StopValues.Reserve(Request.Stop.Num());
    for (const FString &Value : Request.Stop) {
      StopValues.Add(MakeShared<FJsonValueString>(Value));
    }
    Payload->SetArrayField(TEXT("stop"), StopValues);
  }

  if (!Request.JsonSchemaJson.IsEmpty()) {
    TSharedPtr<FJsonObject> JsonSchema;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(Request.JsonSchemaJson);
    if (FJsonSerializer::Deserialize(Reader, JsonSchema) && JsonSchema.IsValid()) {
      Payload->SetObjectField(TEXT("jsonSchema"), JsonSchema);
    }
  }

  return ToJsonString(Payload);
}

template <typename Arg, typename Result>
inline ThunkAction<Result, FSDKState>
MakeEndpoint(const FString &EndpointName, const Arg &ArgValue,
             std::function<func::AsyncResult<func::HttpResult<Result>>(
                 const Arg &)>
                 RequestBuilder,
             const TArray<FApiEndpointTag> &ProvidesTags =
                 TArray<FApiEndpointTag>(),
             const TArray<FApiEndpointTag> &InvalidatesTags =
                 TArray<FApiEndpointTag>()) {
  ApiEndpoint<Arg, Result> Endpoint;
  Endpoint.EndpointName = EndpointName;
  Endpoint.ProvidesTags = ProvidesTags;
  Endpoint.InvalidatesTags = InvalidatesTags;
  Endpoint.RequestBuilder = RequestBuilder;
  return ForbocAiApi.injectEndpoint(Endpoint)(ArgValue);
}

template <typename Result>
inline ThunkAction<Result, FSDKState>
MakeGet(const FString &EndpointName, const FString &Url,
        const TArray<FApiEndpointTag> &Tags = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<rtk::FEmptyPayload, Result>(
      EndpointName, rtk::FEmptyPayload{},
      [Url](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Get<Result>(Url, SDKConfig::GetApiKey());
      },
      Tags);
}

template <typename Request, typename Result>
inline ThunkAction<Result, FSDKState>
MakePost(const FString &EndpointName, const FString &Url,
         const Request &RequestValue,
         const TArray<FApiEndpointTag> &Invalidates = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<Request, Result>(
      EndpointName, RequestValue,
      [Url](const Request &Arg) {
        return func::AsyncHttp::Post<Result>(Url, ToJson(Arg),
                                             SDKConfig::GetApiKey());
      },
      TArray<FApiEndpointTag>(), Invalidates);
}

template <typename Result>
inline ThunkAction<Result, FSDKState>
MakeDelete(const FString &EndpointName, const FString &Url,
           const TArray<FApiEndpointTag> &Invalidates = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<rtk::FEmptyPayload, Result>(
      EndpointName, rtk::FEmptyPayload{},
      [Url](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Delete<Result>(Url, SDKConfig::GetApiKey());
      },
      TArray<FApiEndpointTag>(), Invalidates);
}

template <typename Result>
inline func::AsyncResult<func::HttpResult<Result>>
DecodeHttpResult(func::AsyncResult<func::HttpResult<FString>> RawResult,
                 std::function<bool(const FString &, Result &)> Decoder) {
  return func::AsyncResult<func::HttpResult<Result>>::create(
      [RawResult, Decoder](
          std::function<void(func::HttpResult<Result>)> Resolve,
          std::function<void(std::string)> Reject) {
        RawResult
            .then([Decoder, Resolve](const func::HttpResult<FString> &Http) {
              if (!Http.bSuccess) {
                Resolve(func::HttpResult<Result>::Failure(Http.error,
                                                          Http.ResponseCode));
                return;
              }

              Result Parsed;
              if (Decoder(Http.data, Parsed)) {
                Resolve(func::HttpResult<Result>::Success(Parsed,
                                                          Http.ResponseCode));
                return;
              }

              Resolve(func::HttpResult<Result>::Failure(
                  "JSON deserialization failed", Http.ResponseCode));
            })
            .catch_([Resolve](std::string Error) {
              Resolve(func::HttpResult<Result>::Failure(Error));
            })
            .execute();
      });
}

template <typename Request, typename Result>
inline ThunkAction<Result, FSDKState>
MakePostWithCodec(const FString &EndpointName, const FString &Url,
                  const Request &RequestValue,
                  std::function<FString(const Request &)> Encoder,
                  std::function<bool(const FString &, Result &)> Decoder,
                  const TArray<FApiEndpointTag> &Invalidates =
                      TArray<FApiEndpointTag>()) {
  return MakeEndpoint<Request, Result>(
      EndpointName, RequestValue,
      [Url, Encoder, Decoder](const Request &Arg) {
        return DecodeHttpResult<Result>(
            func::AsyncHttp::Post<FString>(Url, Encoder(Arg),
                                           SDKConfig::GetApiKey()),
            Decoder);
      },
      TArray<FApiEndpointTag>(), Invalidates);
}

template <typename Result>
inline ThunkAction<Result, FSDKState>
MakeGetWithCodec(const FString &EndpointName, const FString &Url,
                 std::function<bool(const FString &, Result &)> Decoder,
                 const TArray<FApiEndpointTag> &Tags = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<rtk::FEmptyPayload, Result>(
      EndpointName, rtk::FEmptyPayload{},
      [Url, Decoder](const rtk::FEmptyPayload &) {
        return DecodeHttpResult<Result>(
            func::AsyncHttp::Get<FString>(Url, SDKConfig::GetApiKey()), Decoder);
      },
      Tags);
}

template <typename Result>
inline ThunkAction<Result, FSDKState>
MakePostRawWithCodec(const FString &EndpointName, const FString &Url,
                     const FString &PayloadJson,
                     std::function<bool(const FString &, Result &)> Decoder) {
  return MakeEndpoint<FString, Result>(
      EndpointName, PayloadJson,
      [Url, Decoder](const FString &Arg) {
        return DecodeHttpResult<Result>(
            func::AsyncHttp::Post<FString>(Url, Arg, SDKConfig::GetApiKey()),
            Decoder);
      });
}

inline TSharedRef<FJsonObject> EncodeProcessTapeObject(const FNPCProcessTape &Tape) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("observation"), Tape.Observation);
  JsonInterop::SetFieldFromJsonString(Root, TEXT("context"), Tape.ContextJson,
                                      false);
  Root->SetObjectField(TEXT("npcState"),
                       JsonInterop::StateToObject(Tape.NpcState));
  if (!Tape.Persona.IsEmpty()) {
    Root->SetStringField(TEXT("persona"), Tape.Persona);
  }
  if (Tape.bHasActor) {
    const TSharedRef<FJsonObject> Actor = MakeShared<FJsonObject>();
    Actor->SetStringField(TEXT("npcId"), Tape.Actor.NpcId);
    Actor->SetStringField(TEXT("persona"), Tape.Actor.Persona);
    Actor->SetObjectField(TEXT("data"),
                          JsonInterop::StateToObject(Tape.Actor.Data));
    Root->SetObjectField(TEXT("actor"), Actor);
  }

  TArray<TSharedPtr<FJsonValue>> Memories;
  for (const FRecalledMemory &Memory : Tape.Memories) {
    Memories.Add(MakeShared<FJsonValueObject>(
        JsonInterop::RecalledMemoryToObject(Memory)));
  }
  Root->SetArrayField(TEXT("memories"), Memories);

  if (!Tape.Prompt.IsEmpty()) {
    Root->SetStringField(TEXT("prompt"), Tape.Prompt);
  }
  if (!Tape.Prompt.IsEmpty() || !Tape.GeneratedOutput.IsEmpty()) {
    Root->SetObjectField(TEXT("constraints"),
                         JsonInterop::CortexConfigToObject(Tape.Constraints));
  }
  if (!Tape.GeneratedOutput.IsEmpty()) {
    Root->SetStringField(TEXT("generatedOutput"), Tape.GeneratedOutput);
  }
  if (!Tape.RulesetId.IsEmpty()) {
    Root->SetStringField(TEXT("rulesetId"), Tape.RulesetId);
  }
  Root->SetBoolField(TEXT("vectorQueried"), Tape.bVectorQueried);
  return Root;
}

inline bool DecodeProcessTapeObject(const TSharedPtr<FJsonObject> &Object,
                                    FNPCProcessTape &Tape) {
  if (!Object.IsValid()) {
    return false;
  }

  Tape.Observation = Object->GetStringField(TEXT("observation"));
  Tape.ContextJson = JsonInterop::JsonStringFromField(Object, TEXT("context"));
  Tape.NpcState = JsonInterop::StateFromField(Object, TEXT("npcState"));
  Tape.Persona = Object->GetStringField(TEXT("persona"));
  Tape.bHasActor = Object->HasTypedField<EJson::Object>(TEXT("actor"));
  if (Tape.bHasActor) {
    const TSharedPtr<FJsonObject> Actor = Object->GetObjectField(TEXT("actor"));
    Tape.Actor.NpcId = Actor->GetStringField(TEXT("npcId"));
    Tape.Actor.Persona = Actor->GetStringField(TEXT("persona"));
    Tape.Actor.Data = JsonInterop::StateFromField(Actor, TEXT("data"));
  }

  Tape.Memories.Empty();
  const TArray<TSharedPtr<FJsonValue>> *MemoryValues = nullptr;
  if (Object->TryGetArrayField(TEXT("memories"), MemoryValues) && MemoryValues) {
    for (const TSharedPtr<FJsonValue> &Value : *MemoryValues) {
      if (Value.IsValid() && Value->Type == EJson::Object) {
        Tape.Memories.Add(JsonInterop::RecalledMemoryFromObject(Value->AsObject()));
      }
    }
  }

  Tape.Prompt = Object->GetStringField(TEXT("prompt"));
  if (Object->HasTypedField<EJson::Object>(TEXT("constraints"))) {
    Tape.Constraints = JsonInterop::CortexConfigFromObject(
        Object->GetObjectField(TEXT("constraints")));
  }
  Tape.GeneratedOutput = Object->GetStringField(TEXT("generatedOutput"));
  Tape.RulesetId = Object->GetStringField(TEXT("rulesetId"));
  bool bVectorQueried = false;
  if (Object->TryGetBoolField(TEXT("vectorQueried"), bVectorQueried)) {
    Tape.bVectorQueried = bVectorQueried;
  }
  return true;
}

inline FString EncodeNpcProcessRequest(const FNPCProcessRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetObjectField(TEXT("tape"), EncodeProcessTapeObject(Request.Tape));
  if (Request.bHasLastResult) {
    JsonInterop::SetFieldFromJsonString(Root, TEXT("lastResult"),
                                        Request.LastResultJson, false);
  }
  return ToJsonString(Root);
}

inline bool DecodeInstructionObject(const TSharedPtr<FJsonObject> &Object,
                                    FNPCInstruction &Instruction) {
  if (!Object.IsValid()) {
    return false;
  }

  const FString Type = Object->GetStringField(TEXT("type"));
  if (Type == TEXT("IdentifyActor")) {
    Instruction.Type = ENPCInstructionType::IdentifyActor;
    return true;
  }

  if (Type == TEXT("QueryVector")) {
    Instruction.Type = ENPCInstructionType::QueryVector;
    Instruction.Query = Object->GetStringField(TEXT("query"));
    double Number = 0.0;
    if (Object->TryGetNumberField(TEXT("limit"), Number)) {
      Instruction.Limit = static_cast<int32>(Number);
    }
    if (Object->TryGetNumberField(TEXT("threshold"), Number)) {
      Instruction.Threshold = static_cast<float>(Number);
    }
    return true;
  }

  if (Type == TEXT("ExecuteInference")) {
    Instruction.Type = ENPCInstructionType::ExecuteInference;
    Instruction.Prompt = Object->GetStringField(TEXT("prompt"));
    if (Object->HasTypedField<EJson::Object>(TEXT("constraints"))) {
      Instruction.Constraints = JsonInterop::CortexConfigFromObject(
          Object->GetObjectField(TEXT("constraints")));
    }
    return true;
  }

  if (Type == TEXT("Finalize")) {
    Instruction.Type = ENPCInstructionType::Finalize;
    bool bValid = true;
    if (Object->TryGetBoolField(TEXT("valid"), bValid)) {
      Instruction.bValid = bValid;
    }
    Instruction.Signature = Object->GetStringField(TEXT("signature"));
    Instruction.StateTransform =
        JsonInterop::StateFromField(Object, TEXT("stateTransform"));
    Instruction.Dialogue = Object->GetStringField(TEXT("dialogue"));
    Instruction.bHasAction = Object->HasTypedField<EJson::Object>(TEXT("action"));
    if (Instruction.bHasAction) {
      Instruction.Action =
          JsonInterop::ActionFromObject(Object->GetObjectField(TEXT("action")));
    }

    const TArray<TSharedPtr<FJsonValue>> *MemoryValues = nullptr;
    if (Object->TryGetArrayField(TEXT("memoryStore"), MemoryValues) &&
        MemoryValues) {
      Instruction.MemoryStore.Empty(MemoryValues->Num());
      for (const TSharedPtr<FJsonValue> &Value : *MemoryValues) {
        if (Value.IsValid() && Value->Type == EJson::Object) {
          Instruction.MemoryStore.Add(
              JsonInterop::MemoryStoreInstructionFromObject(Value->AsObject()));
        }
      }
    }
    return true;
  }

  return false;
}

inline bool DecodeNpcProcessResponse(const FString &Json,
                                     FNPCProcessResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  if (!Root->HasTypedField<EJson::Object>(TEXT("instruction")) ||
      !Root->HasTypedField<EJson::Object>(TEXT("tape"))) {
    return false;
  }

  return DecodeInstructionObject(Root->GetObjectField(TEXT("instruction")),
                                 Response.Instruction) &&
         DecodeProcessTapeObject(Root->GetObjectField(TEXT("tape")),
                                 Response.Tape);
}

inline FString EncodeDirectiveRequest(const FDirectiveRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("observation"), Request.Observation);
  Root->SetObjectField(TEXT("npcState"),
                       JsonInterop::StateToObject(Request.NpcState));
  JsonInterop::SetFieldFromJsonString(Root, TEXT("context"), Request.ContextJson,
                                      false);
  return ToJsonString(Root);
}

inline bool DecodeDirectiveResponse(const FString &Json,
                                    FDirectiveResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid() ||
      !Root->HasTypedField<EJson::Object>(TEXT("memoryRecall"))) {
    return false;
  }

  const TSharedPtr<FJsonObject> Recall = Root->GetObjectField(TEXT("memoryRecall"));
  Response.MemoryRecall.Query = Recall->GetStringField(TEXT("query"));
  double Number = 0.0;
  if (Recall->TryGetNumberField(TEXT("limit"), Number)) {
    Response.MemoryRecall.Limit = static_cast<int32>(Number);
  }
  if (Recall->TryGetNumberField(TEXT("threshold"), Number)) {
    Response.MemoryRecall.Threshold = static_cast<float>(Number);
  }
  return true;
}

inline FString EncodeContextRequest(const FContextRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("observation"), Request.Observation);
  Root->SetObjectField(TEXT("npcState"),
                       JsonInterop::StateToObject(Request.NpcState));
  Root->SetStringField(TEXT("persona"), Request.Persona);

  TArray<TSharedPtr<FJsonValue>> Memories;
  for (const FRecalledMemory &Memory : Request.Memories) {
    Memories.Add(MakeShared<FJsonValueObject>(
        JsonInterop::RecalledMemoryToObject(Memory)));
  }
  Root->SetArrayField(TEXT("memories"), Memories);
  return ToJsonString(Root);
}

inline bool DecodeContextResponse(const FString &Json,
                                  FContextResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  Response.Prompt = Root->GetStringField(TEXT("prompt"));
  if (Root->HasTypedField<EJson::Object>(TEXT("constraints"))) {
    Response.Constraints = JsonInterop::CortexConfigFromObject(
        Root->GetObjectField(TEXT("constraints")));
  }
  return true;
}

inline FString EncodeVerdictRequest(const FVerdictRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("generatedOutput"), Request.GeneratedOutput);
  Root->SetStringField(TEXT("observation"), Request.Observation);
  Root->SetObjectField(TEXT("npcState"),
                       JsonInterop::StateToObject(Request.NpcState));
  return ToJsonString(Root);
}

inline bool DecodeVerdictResponse(const FString &Json,
                                  FVerdictResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  bool bValid = Response.bValid;
  if (Root->TryGetBoolField(TEXT("valid"), bValid)) {
    Response.bValid = bValid;
  }
  Response.Signature = Root->GetStringField(TEXT("signature"));
  Response.StateDelta = JsonInterop::StateFromField(Root, TEXT("stateDelta"));
  Response.Dialogue = Root->GetStringField(TEXT("dialogue"));
  Response.bHasAction = Root->HasTypedField<EJson::Object>(TEXT("action"));
  if (Response.bHasAction) {
    Response.Action =
        JsonInterop::ActionFromObject(Root->GetObjectField(TEXT("action")));
  }

  const TArray<TSharedPtr<FJsonValue>> *MemoryValues = nullptr;
  if (Root->TryGetArrayField(TEXT("memoryStore"), MemoryValues) &&
      MemoryValues) {
    Response.MemoryStore.Empty(MemoryValues->Num());
    for (const TSharedPtr<FJsonValue> &Value : *MemoryValues) {
      if (Value.IsValid() && Value->Type == EJson::Object) {
        Response.MemoryStore.Add(
            JsonInterop::MemoryStoreInstructionFromObject(Value->AsObject()));
      }
    }
  }
  return true;
}

inline FString EncodeBridgeValidateRequest(const FBridgeValidateRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetObjectField(TEXT("action"), JsonInterop::ActionToObject(Request.Action));
  Root->SetObjectField(TEXT("context"),
                       JsonInterop::ValidationContextToObject(Request.Context));
  return ToJsonString(Root);
}

inline bool DecodeValidationResult(const FString &Json,
                                   FValidationResult &Result) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }
  Result = JsonInterop::ValidationResultFromObject(Root);
  return true;
}

inline FString EncodeSoulExportPhase1Request(
    const FSoulExportPhase1Request &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("npcIdRef"), Request.NpcIdRef);
  Root->SetStringField(TEXT("persona"), Request.Persona);
  Root->SetObjectField(TEXT("npcState"),
                       JsonInterop::StateToObject(Request.NpcState));
  return ToJsonString(Root);
}

inline bool DecodeSoulExportPhase1Response(const FString &Json,
                                           FSoulExportPhase1Response &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid() ||
      !Root->HasTypedField<EJson::Object>(TEXT("se1Instruction"))) {
    return false;
  }

  Response.se1Instruction = JsonInterop::UploadInstructionFromObject(
      Root->GetObjectField(TEXT("se1Instruction")));
  Response.se1SignedPayload =
      JsonInterop::JsonStringFromField(Root, TEXT("se1SignedPayload"), TEXT(""));
  Response.se1Signature = Root->GetStringField(TEXT("se1Signature"));
  return true;
}

inline FString EncodeSoulExportConfirmRequest(
    const FSoulExportConfirmRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetObjectField(TEXT("secUploadResult"),
                       JsonInterop::UploadResultToObject(Request.secUploadResult));
  JsonInterop::SetFieldFromJsonString(Root, TEXT("secSignedPayload"),
                                      Request.secSignedPayload, false);
  Root->SetStringField(TEXT("secSignature"), Request.secSignature);
  return ToJsonString(Root);
}

inline bool DecodeSoulExportResponse(const FString &Json,
                                     FSoulExportResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  Response.TxId = Root->GetStringField(TEXT("txId"));
  Response.ArweaveUrl = Root->GetStringField(TEXT("arweaveUrl"));
  Response.Signature = Root->GetStringField(TEXT("signature"));
  if (Root->HasTypedField<EJson::Object>(TEXT("soul"))) {
    Response.Soul = JsonInterop::SoulFromObject(Root->GetObjectField(TEXT("soul")));
  }
  return true;
}

inline FString EncodeSoulImportPhase1Request(
    const FSoulImportPhase1Request &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("txIdRef"), Request.TxIdRef);
  return ToJsonString(Root);
}

inline bool DecodeSoulImportPhase1Response(const FString &Json,
                                           FSoulImportPhase1Response &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid() ||
      !Root->HasTypedField<EJson::Object>(TEXT("si1Instruction"))) {
    return false;
  }

  Response.si1Instruction = JsonInterop::DownloadInstructionFromObject(
      Root->GetObjectField(TEXT("si1Instruction")));
  return true;
}

inline FString EncodeSoulImportConfirmRequest(
    const FSoulImportConfirmRequest &Request) {
  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("sicTxId"), Request.sicTxId);
  Root->SetObjectField(TEXT("sicDownloadResult"),
                       JsonInterop::DownloadResultToObject(
                           Request.sicDownloadResult));
  return ToJsonString(Root);
}

inline bool DecodeImportedNpc(const FString &Json, FImportedNpc &Npc) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }
  Npc = JsonInterop::ImportedNpcFromObject(Root);
  return true;
}

inline bool DecodeGhostRunResponse(const FString &Json,
                                   FGhostRunResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }
  Response.SessionId = Root->GetStringField(TEXT("sessionId"));
  Response.RunStatus = Root->GetStringField(TEXT("runStatus"));
  return true;
}

inline int64 JsonNumberOrStringToInt64(const TSharedPtr<FJsonObject> &Object,
                                       const FString &FieldName) {
  if (!Object.IsValid() || !Object->HasField(FieldName)) {
    return 0;
  }

  const TSharedPtr<FJsonValue> Value = Object->TryGetField(FieldName);
  if (!Value.IsValid()) {
    return 0;
  }
  if (Value->Type == EJson::Number) {
    return static_cast<int64>(Value->AsNumber());
  }
  if (Value->Type == EJson::String) {
    return FCString::Atoi64(*Value->AsString());
  }
  return 0;
}

inline bool DecodeGhostStatusResponse(const FString &Json,
                                      FGhostStatusResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  Response.GhostSessionId = Root->GetStringField(TEXT("ghostSessionId"));
  Response.GhostStatus = Root->GetStringField(TEXT("ghostStatus"));
  double Number = 0.0;
  if (Root->TryGetNumberField(TEXT("ghostProgress"), Number)) {
    Response.GhostProgress = static_cast<float>(Number);
  }
  Response.GhostStartedAt =
      JsonNumberOrStringToInt64(Root, TEXT("ghostStartedAt"));
  if (Root->TryGetNumberField(TEXT("ghostDuration"), Number)) {
    Response.GhostDuration = static_cast<int32>(Number);
  }

  Response.GhostErrors.Empty();
  if (Root->HasTypedField<EJson::Array>(TEXT("ghostErrors"))) {
    const TArray<TSharedPtr<FJsonValue>> *Values = nullptr;
    if (Root->TryGetArrayField(TEXT("ghostErrors"), Values) && Values) {
      for (const TSharedPtr<FJsonValue> &Value : *Values) {
        if (Value.IsValid()) {
          Response.GhostErrors.Add(Value->AsString());
        }
      }
    }
  } else if (Root->HasField(TEXT("ghostErrors"))) {
    const TSharedPtr<FJsonValue> Value = Root->TryGetField(TEXT("ghostErrors"));
    if (Value.IsValid()) {
      if (Value->Type == EJson::String) {
        if (!Value->AsString().IsEmpty()) {
          Response.GhostErrors.Add(Value->AsString());
        }
      } else if (Value->Type == EJson::Number) {
        const int32 Count = static_cast<int32>(Value->AsNumber());
        if (Count > 0) {
          Response.GhostErrors.Add(FString::FromInt(Count));
        }
      }
    }
  }

  return true;
}

inline bool DecodeGhostResultsResponse(const FString &Json,
                                       FGhostResultsResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  Response.ResultsSessionId = Root->GetStringField(TEXT("resultsSessionId"));
  double Number = 0.0;
  if (Root->TryGetNumberField(TEXT("resultsTotalTests"), Number)) {
    Response.ResultsTotalTests = static_cast<int32>(Number);
  }
  if (Root->TryGetNumberField(TEXT("resultsPassed"), Number)) {
    Response.ResultsPassed = static_cast<int32>(Number);
  }
  if (Root->TryGetNumberField(TEXT("resultsFailed"), Number)) {
    Response.ResultsFailed = static_cast<int32>(Number);
  }
  if (Root->TryGetNumberField(TEXT("resultsSkipped"), Number)) {
    Response.ResultsSkipped = static_cast<int32>(Number);
  }
  if (Root->TryGetNumberField(TEXT("resultsDuration"), Number)) {
    Response.ResultsDuration = static_cast<int64>(Number);
  }
  if (Root->TryGetNumberField(TEXT("resultsCoverage"), Number)) {
    Response.ResultsCoverage = static_cast<float>(Number);
  }

  Response.ResultsTests.Empty();
  const TArray<TSharedPtr<FJsonValue>> *Tests = nullptr;
  if (Root->TryGetArrayField(TEXT("resultsTests"), Tests) && Tests) {
    for (const TSharedPtr<FJsonValue> &Value : *Tests) {
      if (!Value.IsValid() || Value->Type != EJson::Object) {
        continue;
      }
      const TSharedPtr<FJsonObject> Test = Value->AsObject();
      FGhostResultRecord Record;
      Record.TestName = Test->GetStringField(TEXT("testName"));
      bool bPassed = false;
      if (Test->TryGetBoolField(TEXT("testPassed"), bPassed)) {
        Record.bTestPassed = bPassed;
      }
      if (Test->TryGetNumberField(TEXT("testDuration"), Number)) {
        Record.TestDuration = static_cast<int64>(Number);
      }
      Record.TestError = Test->GetStringField(TEXT("testError"));
      Record.TestScreenshot = Test->GetStringField(TEXT("testScreenshot"));
      Response.ResultsTests.Add(Record);
    }
  }

  Response.ResultsMetrics.Empty();
  const TArray<TSharedPtr<FJsonValue>> *MetricPairs = nullptr;
  if (Root->TryGetArrayField(TEXT("resultsMetrics"), MetricPairs) && MetricPairs) {
    for (const TSharedPtr<FJsonValue> &PairValue : *MetricPairs) {
      if (!PairValue.IsValid() || PairValue->Type != EJson::Array) {
        continue;
      }
      const TArray<TSharedPtr<FJsonValue>> Pair = PairValue->AsArray();
      if (Pair.Num() != 2 || !Pair[0].IsValid() || !Pair[1].IsValid()) {
        continue;
      }
      Response.ResultsMetrics.Add(Pair[0]->AsString(),
                                  static_cast<float>(Pair[1]->AsNumber()));
    }
  }

  return true;
}

inline bool DecodeGhostStopResponse(const FString &Json,
                                    FGhostStopResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }
  Response.StopStatus = Root->GetStringField(TEXT("stopStatus"));
  Response.StopSessionId = Root->GetStringField(TEXT("stopSessionId"));
  Response.bStopped =
      Response.StopStatus.Equals(TEXT("stopped"), ESearchCase::IgnoreCase);
  return true;
}

inline bool DecodeGhostHistoryResponse(const FString &Json,
                                       FGhostHistoryResponse &Response) {
  TSharedPtr<FJsonObject> Root;
  if (!JsonInterop::ParseJsonObject(Json, Root) || !Root.IsValid()) {
    return false;
  }

  const TArray<TSharedPtr<FJsonValue>> *Sessions = nullptr;
  if (!Root->TryGetArrayField(TEXT("sessions"), Sessions) || !Sessions) {
    return false;
  }

  Response.Sessions.Empty(Sessions->Num());
  for (const TSharedPtr<FJsonValue> &Value : *Sessions) {
    if (!Value.IsValid() || Value->Type != EJson::Object) {
      continue;
    }

    const TSharedPtr<FJsonObject> Session = Value->AsObject();
    FGhostHistoryEntry Entry;
    Entry.SessionId = Session->HasField(TEXT("sessionId"))
                          ? Session->GetStringField(TEXT("sessionId"))
                          : Session->GetStringField(TEXT("histSessionId"));
    Entry.TestSuite = Session->HasField(TEXT("testSuite"))
                          ? Session->GetStringField(TEXT("testSuite"))
                          : Session->GetStringField(TEXT("histTestSuite"));
    Entry.StartedAt = JsonNumberOrStringToInt64(
        Session, Session->HasField(TEXT("startedAt")) ? TEXT("startedAt")
                                                      : TEXT("histStartedAt"));
    Entry.CompletedAt = JsonNumberOrStringToInt64(
        Session, Session->HasField(TEXT("completedAt")) ? TEXT("completedAt")
                                                        : TEXT("histCompletedAt"));
    Entry.Status = Session->HasField(TEXT("status"))
                       ? Session->GetStringField(TEXT("status"))
                       : Session->GetStringField(TEXT("histStatus"));
    double PassRate = 0.0;
    if (Session->TryGetNumberField(TEXT("passRate"), PassRate) ||
        Session->TryGetNumberField(TEXT("histPassRate"), PassRate)) {
      Entry.PassRate = static_cast<float>(PassRate);
    }
    Response.Sessions.Add(Entry);
  }

  return true;
}

} // namespace Detail

namespace Endpoints {

inline auto getNPCs() {
  TArray<FApiEndpointTag> Tags;
  Tags.Add(FApiEndpointTag{TEXT("NPC"), TEXT("LIST")});
  return Detail::MakeGet<TArray<FAgent>>(TEXT("getNPCs"),
                                         SDKConfig::GetApiUrl() +
                                             TEXT("/npcs"),
                                         Tags);
}

inline auto getNPC(const FString &NpcId) {
  return Detail::MakeGet<FAgent>(TEXT("getNPC"),
                                 SDKConfig::GetApiUrl() + TEXT("/npcs/") +
                                     Detail::Encode(NpcId));
}

inline auto postNPC(const FAgentConfig &Config) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("NPC"), TEXT("LIST")});
  return Detail::MakePost<FAgentConfig, FAgent>(
      TEXT("postNPC"), SDKConfig::GetApiUrl() + TEXT("/npcs"), Config,
      Invalidates);
}

inline auto getApiStatus() {
  return Detail::MakeGet<FApiStatusResponse>(TEXT("getApiStatus"),
                                             SDKConfig::GetApiUrl() +
                                                 TEXT("/status"));
}

inline auto getCortexModels() {
  return Detail::MakeGet<TArray<FCortexModelInfo>>(
      TEXT("getCortexModels"),
      SDKConfig::GetApiUrl() + TEXT("/cortex/models"));
}

inline auto postCortexInit(const FCortexInitRequest &Request) {
  return Detail::MakePost<FCortexInitRequest, FCortexInitResponse>(
      TEXT("postCortexInit"), SDKConfig::GetApiUrl() + TEXT("/cortex/init"),
      Request);
}

inline auto postCortexComplete(const FString &CortexId,
                               const FCortexCompleteRequest &Request) {
  return Detail::MakeEndpoint<FCortexCompleteRequest, FCortexResponse>(
      TEXT("postCortexComplete"),
      Request,
      [CortexId](const FCortexCompleteRequest &Arg) {
        return func::AsyncHttp::Post<FCortexResponse>(
            SDKConfig::GetApiUrl() + TEXT("/cortex/") +
                Detail::Encode(CortexId) + TEXT("/complete"),
            Detail::BuildCortexCompletePayload(Arg), SDKConfig::GetApiKey());
      });
}

inline auto postNpcProcess(const FString &NpcId,
                           const FNPCProcessRequest &Request) {
  return Detail::MakePostWithCodec<FNPCProcessRequest, FNPCProcessResponse>(
      TEXT("postNpcProcess"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/process"),
      Request, Detail::EncodeNpcProcessRequest,
      Detail::DecodeNpcProcessResponse);
}

inline auto postDirective(const FString &NpcId,
                          const FDirectiveRequest &Request) {
  return Detail::MakePostWithCodec<FDirectiveRequest, FDirectiveResponse>(
      TEXT("postDirective"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/directive"),
      Request, Detail::EncodeDirectiveRequest,
      Detail::DecodeDirectiveResponse);
}

inline auto postContext(const FString &NpcId, const FContextRequest &Request) {
  return Detail::MakePostWithCodec<FContextRequest, FContextResponse>(
      TEXT("postContext"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/context"),
      Request, Detail::EncodeContextRequest, Detail::DecodeContextResponse);
}

inline auto postVerdict(const FString &NpcId, const FVerdictRequest &Request) {
  return Detail::MakePostWithCodec<FVerdictRequest, FVerdictResponse>(
      TEXT("postVerdict"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/verdict"),
      Request, Detail::EncodeVerdictRequest, Detail::DecodeVerdictResponse);
}

inline auto postMemoryStore(const FString &NpcId,
                            const FRemoteMemoryStoreRequest &Request) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Memory"), NpcId});
  return Detail::MakePost<FRemoteMemoryStoreRequest, rtk::FEmptyPayload>(
      TEXT("postMemoryStore"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory"),
      Request, Invalidates);
}

inline auto getMemoryList(const FString &NpcId) {
  return Detail::MakeGet<TArray<FMemoryItem>>(
      TEXT("getMemoryList"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory"));
}

inline auto postMemoryRecall(const FString &NpcId,
                             const FRemoteMemoryRecallRequest &Request) {
  return Detail::MakePost<FRemoteMemoryRecallRequest, TArray<FMemoryItem>>(
      TEXT("postMemoryRecall"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory/recall"),
      Request);
}

inline auto deleteMemoryClear(const FString &NpcId) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Memory"), NpcId});
  return Detail::MakeDelete<rtk::FEmptyPayload>(
      TEXT("deleteMemoryClear"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory/clear"),
      Invalidates);
}

inline auto postBridgeValidate(const FString &NpcId,
                               const FString &PayloadJson) {
  const FString Url = NpcId.IsEmpty()
                          ? SDKConfig::GetApiUrl() + TEXT("/bridge/validate")
                          : SDKConfig::GetApiUrl() + TEXT("/bridge/validate/") +
                                Detail::Encode(NpcId);
  return Detail::MakePostRawWithCodec<FValidationResult>(
      TEXT("postBridgeValidate"), Url, PayloadJson,
      Detail::DecodeValidationResult);
}

inline auto postBridgeValidate(const FString &NpcId,
                               const FBridgeValidateRequest &Request) {
  const FString Url = NpcId.IsEmpty()
                          ? SDKConfig::GetApiUrl() + TEXT("/bridge/validate")
                          : SDKConfig::GetApiUrl() + TEXT("/bridge/validate/") +
                                Detail::Encode(NpcId);
  return Detail::MakePostWithCodec<FBridgeValidateRequest, FValidationResult>(
      TEXT("postBridgeValidate"), Url, Request,
      Detail::EncodeBridgeValidateRequest, Detail::DecodeValidationResult);
}

inline auto getBridgeRules() {
  return Detail::MakeGet<TArray<FBridgeRule>>(TEXT("getBridgeRules"),
                                              SDKConfig::GetApiUrl() +
                                                  TEXT("/bridge/rules"));
}

inline auto postBridgePreset(const FString &PresetName) {
  return Detail::MakeEndpoint<rtk::FEmptyPayload, FDirectiveRuleSet>(
      TEXT("postBridgePreset"), rtk::FEmptyPayload{},
      [PresetName](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Post<FDirectiveRuleSet>(
            SDKConfig::GetApiUrl() + TEXT("/rules/presets/") +
                Detail::Encode(PresetName),
            TEXT("{}"), SDKConfig::GetApiKey());
      });
}

inline auto getRulesets() {
  return Detail::MakeGet<TArray<FDirectiveRuleSet>>(
      TEXT("getRulesets"), SDKConfig::GetApiUrl() + TEXT("/rules"));
}

inline auto getRulePresets() {
  return Detail::MakeGet<TArray<FString>>(TEXT("getRulePresets"),
                                          SDKConfig::GetApiUrl() +
                                              TEXT("/rules/presets"));
}

inline auto postRuleRegister(const FDirectiveRuleSet &Ruleset) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Rule"), TEXT("LIST")});
  return Detail::MakePost<FDirectiveRuleSet, FDirectiveRuleSet>(
      TEXT("postRuleRegister"), SDKConfig::GetApiUrl() + TEXT("/rules"),
      Ruleset, Invalidates);
}

inline auto deleteRule(const FString &RulesetId) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Rule"), TEXT("LIST")});
  return Detail::MakeDelete<rtk::FEmptyPayload>(TEXT("deleteRule"),
                                           SDKConfig::GetApiUrl() +
                                               TEXT("/rules/") +
                                               Detail::Encode(RulesetId),
                                           Invalidates);
}

inline auto postGhostRun(const FGhostRunRequest &Request) {
  return Detail::MakePostWithCodec<FGhostRunRequest, FGhostRunResponse>(
      TEXT("postGhostRun"), SDKConfig::GetApiUrl() + TEXT("/ghost/run"),
      Request, Detail::ToJson<FGhostRunRequest>, Detail::DecodeGhostRunResponse);
}

inline auto postGhostRun(const FGhostConfig &Config) {
  return postGhostRun(
      TypeFactory::GhostRunRequest(Config.TestSuite, Config.Duration));
}

inline auto getGhostStatus(const FString &SessionId) {
  return Detail::MakeGetWithCodec<FGhostStatusResponse>(
      TEXT("getGhostStatus"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/status"),
      Detail::DecodeGhostStatusResponse);
}

inline auto getGhostResults(const FString &SessionId) {
  return Detail::MakeGetWithCodec<FGhostResultsResponse>(
      TEXT("getGhostResults"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/results"),
      Detail::DecodeGhostResultsResponse);
}

inline auto postGhostStop(const FString &SessionId) {
  return Detail::MakePostRawWithCodec<FGhostStopResponse>(
      TEXT("postGhostStop"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/stop"),
      TEXT("{}"), Detail::DecodeGhostStopResponse);
}

inline auto getGhostHistory(int32 Limit = 10) {
  return Detail::MakeGetWithCodec<FGhostHistoryResponse>(
      TEXT("getGhostHistory"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/history?limit=") +
          FString::FromInt(Limit),
      Detail::DecodeGhostHistoryResponse);
}

inline auto postSoulExport(const FString &NpcId,
                           const FSoulExportPhase1Request &Request) {
  return Detail::MakePostWithCodec<FSoulExportPhase1Request,
                                   FSoulExportPhase1Response>(
      TEXT("postSoulExport"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/soul/export"),
      Request, Detail::EncodeSoulExportPhase1Request,
      Detail::DecodeSoulExportPhase1Response);
}

inline auto postSoulExportConfirm(const FString &NpcId,
                                  const FSoulExportConfirmRequest &Request) {
  return Detail::MakePostWithCodec<FSoulExportConfirmRequest,
                                   FSoulExportResponse>(
      TEXT("postSoulExportConfirm"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/soul/confirm"),
      Request, Detail::EncodeSoulExportConfirmRequest,
      Detail::DecodeSoulExportResponse);
}

inline auto getSouls(int32 Limit = 50) {
  return Detail::MakeGet<FSoulListResponse>(
      TEXT("getSouls"), SDKConfig::GetApiUrl() + TEXT("/souls?limit=") +
                            FString::FromInt(Limit));
}

inline auto getSoulImport(const FString &TxId) {
  return Detail::MakeGet<FSoulImportPhase1Response>(
      TEXT("getSoulImport"),
      SDKConfig::GetApiUrl() + TEXT("/souls/") + Detail::Encode(TxId));
}

inline auto postSoulVerify(const FString &TxId) {
  return Detail::MakeEndpoint<rtk::FEmptyPayload, FSoulVerifyResult>(
      TEXT("postSoulVerify"), rtk::FEmptyPayload{},
      [TxId](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Post<FSoulVerifyResult>(
            SDKConfig::GetApiUrl() + TEXT("/souls/") + Detail::Encode(TxId) +
                TEXT("/verify"),
            TEXT("{}"), SDKConfig::GetApiKey());
      });
}

inline auto postNpcImport(const FSoulImportPhase1Request &Request) {
  return Detail::MakePostWithCodec<FSoulImportPhase1Request,
                                   FSoulImportPhase1Response>(
      TEXT("postNpcImport"), SDKConfig::GetApiUrl() + TEXT("/npcs/import"),
      Request, Detail::EncodeSoulImportPhase1Request,
      Detail::DecodeSoulImportPhase1Response);
}

inline auto postNpcImportConfirm(const FSoulImportConfirmRequest &Request) {
  return Detail::MakePostWithCodec<FSoulImportConfirmRequest, FImportedNpc>(
      TEXT("postNpcImportConfirm"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/import/confirm"), Request,
      Detail::EncodeSoulImportConfirmRequest, Detail::DecodeImportedNpc);
}

} // namespace Endpoints

inline Slice<FAPIState> CreateAPISlice() {
  return SliceBuilder<FAPIState>(TEXT("forbocApi"), FAPIState()).build();
}

} // namespace APISlice
