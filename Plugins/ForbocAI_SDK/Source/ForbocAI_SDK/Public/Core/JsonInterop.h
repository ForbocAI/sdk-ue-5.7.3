#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Types.h"

namespace JsonInterop {

inline bool ParseJsonObject(const FString &Json, TSharedPtr<FJsonObject> &OutObject) {
  if (Json.IsEmpty()) {
    OutObject = MakeShared<FJsonObject>();
    return true;
  }

  TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
  if (FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid()) {
    return true;
  }

  OutObject = MakeShared<FJsonObject>();
  return false;
}

inline bool ParseJsonArray(const FString &Json,
                           TArray<TSharedPtr<FJsonValue>> &OutArray) {
  TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
  return FJsonSerializer::Deserialize(Reader, OutArray);
}

inline FString StringifyObject(const TSharedPtr<FJsonObject> &Object) {
  FString Json;
  const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
  if (Object.IsValid()) {
    FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
  } else {
    Writer->WriteObjectStart();
    Writer->WriteObjectEnd();
    Writer->Close();
  }
  return Json;
}

inline FString StringifyArray(const TArray<TSharedPtr<FJsonValue>> &Array) {
  FString Json;
  const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
  FJsonSerializer::Serialize(Array, Writer);
  return Json;
}

inline FString StringifyValue(const TSharedPtr<FJsonValue> &Value) {
  if (!Value.IsValid()) {
    return TEXT("null");
  }

  if (Value->Type == EJson::Object) {
    return StringifyObject(Value->AsObject());
  }

  if (Value->Type == EJson::Array) {
    return StringifyArray(Value->AsArray());
  }

  if (Value->Type == EJson::String) {
    return FString::Printf(TEXT("\"%s\""), *Value->AsString());
  }

  if (Value->Type == EJson::Boolean) {
    return Value->AsBool() ? TEXT("true") : TEXT("false");
  }

  if (Value->Type == EJson::Number) {
    return FString::SanitizeFloat(Value->AsNumber());
  }

  return TEXT("null");
}

inline TSharedPtr<FJsonObject> ParseJsonObjectOrEmpty(const FString &Json) {
  TSharedPtr<FJsonObject> Object;
  ParseJsonObject(Json, Object);
  return Object.IsValid() ? Object : MakeShared<FJsonObject>();
}

inline FString JsonStringFromField(const TSharedPtr<FJsonObject> &Object,
                                   const FString &FieldName,
                                   const FString &Fallback = TEXT("{}")) {
  if (!Object.IsValid() || !Object->HasField(FieldName)) {
    return Fallback;
  }

  const TSharedPtr<FJsonValue> Value = Object->TryGetField(FieldName);
  if (!Value.IsValid() || Value->Type == EJson::Null) {
    return Fallback;
  }

  if (Value->Type == EJson::Object || Value->Type == EJson::Array) {
    return StringifyValue(Value);
  }

  if (Value->Type == EJson::String) {
    return Value->AsString();
  }

  return StringifyValue(Value);
}

inline void SetFieldFromJsonString(const TSharedRef<FJsonObject> &Object,
                                   const FString &FieldName,
                                   const FString &JsonString,
                                   bool bOmitWhenEmpty = true) {
  if (JsonString.IsEmpty()) {
    if (!bOmitWhenEmpty) {
      Object->SetObjectField(FieldName, MakeShared<FJsonObject>());
    }
    return;
  }

  TSharedPtr<FJsonObject> JsonObject;
  if (ParseJsonObject(JsonString, JsonObject) && JsonObject.IsValid()) {
    Object->SetObjectField(FieldName, JsonObject);
    return;
  }

  TArray<TSharedPtr<FJsonValue>> JsonArray;
  if (ParseJsonArray(JsonString, JsonArray)) {
    Object->SetArrayField(FieldName, JsonArray);
    return;
  }

  if (!bOmitWhenEmpty || !JsonString.IsEmpty()) {
    Object->SetStringField(FieldName, JsonString);
  }
}

inline FAgentState StateFromObject(const TSharedPtr<FJsonObject> &Object) {
  return TypeFactory::AgentState(StringifyObject(Object));
}

inline FAgentState StateFromField(const TSharedPtr<FJsonObject> &Object,
                                  const FString &FieldName) {
  return TypeFactory::AgentState(JsonStringFromField(Object, FieldName));
}

inline TSharedPtr<FJsonObject> StateToObject(const FAgentState &State) {
  return ParseJsonObjectOrEmpty(State.JsonData);
}

inline TSharedRef<FJsonObject> ActionToObject(const FAgentAction &Action) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  if (!Action.Type.IsEmpty()) {
    Object->SetStringField(TEXT("type"), Action.Type);
  }
  if (!Action.Target.IsEmpty()) {
    Object->SetStringField(TEXT("target"), Action.Target);
  }
  if (!Action.Reason.IsEmpty()) {
    Object->SetStringField(TEXT("reason"), Action.Reason);
  }
  if (!FMath::IsNearlyEqual(Action.Confidence, 1.0f) || !Action.Type.IsEmpty()) {
    Object->SetNumberField(TEXT("confidence"), Action.Confidence);
  }
  if (!Action.Signature.IsEmpty()) {
    Object->SetStringField(TEXT("signature"), Action.Signature);
  }
  SetFieldFromJsonString(Object, TEXT("payload"), Action.PayloadJson);
  return Object;
}

inline FAgentAction ActionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FAgentAction Action;
  if (!Object.IsValid()) {
    return Action;
  }

  Action.Type = Object->GetStringField(TEXT("type"));
  Action.Target = Object->GetStringField(TEXT("target"));
  Action.Reason = Object->GetStringField(TEXT("reason"));

  double Confidence = 1.0;
  if (Object->TryGetNumberField(TEXT("confidence"), Confidence)) {
    Action.Confidence = static_cast<float>(Confidence);
  }
  Action.Signature = Object->GetStringField(TEXT("signature"));
  Action.PayloadJson = JsonStringFromField(Object, TEXT("payload"), TEXT("{}"));
  return Action;
}

inline TSharedRef<FJsonObject>
MemoryStoreInstructionToObject(const FMemoryStoreInstruction &Instruction) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  Object->SetStringField(TEXT("text"), Instruction.Text);
  Object->SetStringField(TEXT("type"), Instruction.Type);
  Object->SetNumberField(TEXT("importance"), Instruction.Importance);
  return Object;
}

inline FMemoryStoreInstruction
MemoryStoreInstructionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FMemoryStoreInstruction Instruction;
  if (!Object.IsValid()) {
    return Instruction;
  }

  Instruction.Text = Object->GetStringField(TEXT("text"));
  Instruction.Type = Object->GetStringField(TEXT("type"));
  double Importance = Instruction.Importance;
  if (Object->TryGetNumberField(TEXT("importance"), Importance)) {
    Instruction.Importance = static_cast<float>(Importance);
  }
  return Instruction;
}

inline TSharedRef<FJsonObject> RecalledMemoryToObject(const FRecalledMemory &Memory) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  Object->SetStringField(TEXT("text"), Memory.Text);
  Object->SetStringField(TEXT("type"), Memory.Type);
  Object->SetNumberField(TEXT("importance"), Memory.Importance);
  Object->SetNumberField(TEXT("similarity"), Memory.Similarity);
  return Object;
}

inline FRecalledMemory RecalledMemoryFromObject(const TSharedPtr<FJsonObject> &Object) {
  FRecalledMemory Memory;
  if (!Object.IsValid()) {
    return Memory;
  }

  Memory.Text = Object->GetStringField(TEXT("text"));
  Memory.Type = Object->GetStringField(TEXT("type"));
  double Importance = Memory.Importance;
  double Similarity = Memory.Similarity;
  if (Object->TryGetNumberField(TEXT("importance"), Importance)) {
    Memory.Importance = static_cast<float>(Importance);
  }
  if (Object->TryGetNumberField(TEXT("similarity"), Similarity)) {
    Memory.Similarity = static_cast<float>(Similarity);
  }
  return Memory;
}

inline TSharedRef<FJsonObject> MemoryItemToObject(const FMemoryItem &Memory) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  if (!Memory.Id.IsEmpty()) {
    Object->SetStringField(TEXT("id"), Memory.Id);
  }
  Object->SetStringField(TEXT("text"), Memory.Text);
  Object->SetStringField(TEXT("type"), Memory.Type);
  Object->SetNumberField(TEXT("importance"), Memory.Importance);
  Object->SetNumberField(TEXT("similarity"), Memory.Similarity);
  Object->SetNumberField(TEXT("timestamp"), static_cast<double>(Memory.Timestamp));
  return Object;
}

inline FMemoryItem MemoryItemFromObject(const TSharedPtr<FJsonObject> &Object) {
  FMemoryItem Memory;
  if (!Object.IsValid()) {
    return Memory;
  }

  Memory.Id = Object->GetStringField(TEXT("id"));
  Memory.Text = Object->GetStringField(TEXT("text"));
  Memory.Type = Object->GetStringField(TEXT("type"));
  double Importance = Memory.Importance;
  double Similarity = Memory.Similarity;
  double Timestamp = static_cast<double>(Memory.Timestamp);
  if (Object->TryGetNumberField(TEXT("importance"), Importance)) {
    Memory.Importance = static_cast<float>(Importance);
  }
  if (Object->TryGetNumberField(TEXT("similarity"), Similarity)) {
    Memory.Similarity = static_cast<float>(Similarity);
  }
  if (Object->TryGetNumberField(TEXT("timestamp"), Timestamp)) {
    Memory.Timestamp = static_cast<int64>(Timestamp);
  }
  return Memory;
}

inline TSharedRef<FJsonObject> CortexConfigToObject(const FCortexConfig &Config) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  if (!Config.Model.IsEmpty()) {
    Object->SetStringField(TEXT("model"), Config.Model);
  }
  Object->SetBoolField(TEXT("useGPU"), Config.UseGPU);
  Object->SetNumberField(TEXT("maxTokens"), Config.MaxTokens);
  Object->SetNumberField(TEXT("temperature"), Config.Temperature);
  Object->SetNumberField(TEXT("topK"), Config.TopK);
  Object->SetNumberField(TEXT("topP"), Config.TopP);

  if (Config.Stop.Num() > 0) {
    TArray<TSharedPtr<FJsonValue>> StopValues;
    for (const FString &Value : Config.Stop) {
      StopValues.Add(MakeShared<FJsonValueString>(Value));
    }
    Object->SetArrayField(TEXT("stop"), StopValues);
  }

  SetFieldFromJsonString(Object, TEXT("jsonSchema"), Config.JsonSchemaJson);
  return Object;
}

inline FCortexConfig CortexConfigFromObject(const TSharedPtr<FJsonObject> &Object) {
  FCortexConfig Config;
  if (!Object.IsValid()) {
    return Config;
  }

  Config.Model = Object->GetStringField(TEXT("model"));
  bool bUseGpu = Config.UseGPU;
  if (Object->TryGetBoolField(TEXT("useGPU"), bUseGpu)) {
    Config.UseGPU = bUseGpu;
  }

  double NumericValue = 0.0;
  if (Object->TryGetNumberField(TEXT("maxTokens"), NumericValue)) {
    Config.MaxTokens = static_cast<int32>(NumericValue);
  }
  if (Object->TryGetNumberField(TEXT("temperature"), NumericValue)) {
    Config.Temperature = static_cast<float>(NumericValue);
  }
  if (Object->TryGetNumberField(TEXT("topK"), NumericValue)) {
    Config.TopK = static_cast<int32>(NumericValue);
  }
  if (Object->TryGetNumberField(TEXT("topP"), NumericValue)) {
    Config.TopP = static_cast<float>(NumericValue);
  }

  const TArray<TSharedPtr<FJsonValue>> *StopValues = nullptr;
  if (Object->TryGetArrayField(TEXT("stop"), StopValues) && StopValues) {
    Config.Stop.Empty(StopValues->Num());
    for (const TSharedPtr<FJsonValue> &Value : *StopValues) {
      if (Value.IsValid()) {
        Config.Stop.Add(Value->AsString());
      }
    }
  }

  Config.JsonSchemaJson = JsonStringFromField(Object, TEXT("jsonSchema"), TEXT(""));
  return Config;
}

inline TSharedRef<FJsonObject>
ValidationContextToObject(const FBridgeValidationContext &Context) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  SetFieldFromJsonString(Object, TEXT("npcState"), Context.NpcStateJson);
  SetFieldFromJsonString(Object, TEXT("worldState"), Context.WorldStateJson);
  SetFieldFromJsonString(Object, TEXT("constraints"), Context.ConstraintsJson);
  return Object;
}

inline FValidationResult
ValidationResultFromObject(const TSharedPtr<FJsonObject> &Object) {
  FValidationResult Result;
  if (!Object.IsValid()) {
    Result.bValid = false;
    Result.Reason = TEXT("Invalid validation response");
    return Result;
  }

  bool bValid = Result.bValid;
  if (Object->TryGetBoolField(TEXT("valid"), bValid)) {
    Result.bValid = bValid;
  }
  Result.Reason = Object->GetStringField(TEXT("reason"));
  if (Object->HasTypedField<EJson::Object>(TEXT("correctedAction"))) {
    Result.CorrectedAction =
        ActionFromObject(Object->GetObjectField(TEXT("correctedAction")));
  }
  return Result;
}

inline FArweaveUploadInstruction
UploadInstructionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FArweaveUploadInstruction Instruction;
  if (!Object.IsValid()) {
    return Instruction;
  }

  Instruction.UploadUrl = Object->GetStringField(TEXT("auiEndpoint"));
  Instruction.ContentType = Object->GetStringField(TEXT("auiContentType"));
  Instruction.AuiAuthHeader = Object->GetStringField(TEXT("auiAuthHeader"));
  Instruction.PayloadJson = JsonStringFromField(Object, TEXT("auiPayload"), TEXT("{}"));
  return Instruction;
}

inline TSharedRef<FJsonObject>
UploadResultToObject(const FArweaveUploadResult &Result) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  Object->SetStringField(TEXT("aurTxId"), Result.TxId);
  Object->SetNumberField(TEXT("aurStatus"),
                         Result.StatusCode > 0 ? Result.StatusCode : FCString::Atoi(*Result.Status));
  Object->SetBoolField(TEXT("aurSuccess"), Result.bSuccess);
  if (!Result.Error.IsEmpty()) {
    Object->SetStringField(TEXT("aurError"), Result.Error);
  }
  return Object;
}

inline FArweaveDownloadInstruction
DownloadInstructionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FArweaveDownloadInstruction Instruction;
  if (!Object.IsValid()) {
    return Instruction;
  }

  Instruction.GatewayUrl = Object->GetStringField(TEXT("adiGatewayUrl"));
  Instruction.TxId = Object->GetStringField(TEXT("adiExpectedTxId"));
  Instruction.ExpectedTxId = Instruction.TxId;
  return Instruction;
}

inline TSharedRef<FJsonObject>
DownloadResultToObject(const FArweaveDownloadResult &Result) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  SetFieldFromJsonString(
      Object, TEXT("adrBody"),
      !Result.BodyJson.IsEmpty() ? Result.BodyJson : Result.Payload, false);
  Object->SetNumberField(TEXT("adrStatus"), Result.StatusCode);
  Object->SetBoolField(TEXT("adrSuccess"), Result.bSuccess);
  if (!Result.Error.IsEmpty()) {
    Object->SetStringField(TEXT("adrError"), Result.Error);
  }
  return Object;
}

inline FSoul SoulFromObject(const TSharedPtr<FJsonObject> &Object) {
  FSoul Soul;
  if (!Object.IsValid()) {
    return Soul;
  }

  Soul.Id = Object->GetStringField(TEXT("id"));
  Soul.Version = Object->GetStringField(TEXT("version"));
  Soul.Name = Object->GetStringField(TEXT("name"));
  Soul.Persona = Object->GetStringField(TEXT("persona"));
  Soul.Signature = Object->GetStringField(TEXT("signature"));
  Soul.State = StateFromField(Object, TEXT("state"));

  const TArray<TSharedPtr<FJsonValue>> *MemoryValues = nullptr;
  if (Object->TryGetArrayField(TEXT("memories"), MemoryValues) && MemoryValues) {
    Soul.Memories.Empty(MemoryValues->Num());
    for (const TSharedPtr<FJsonValue> &Value : *MemoryValues) {
      if (Value.IsValid() && Value->Type == EJson::Object) {
        Soul.Memories.Add(MemoryItemFromObject(Value->AsObject()));
      }
    }
  }
  return Soul;
}

inline FImportedNpc ImportedNpcFromObject(const TSharedPtr<FJsonObject> &Object) {
  FImportedNpc Npc;
  if (!Object.IsValid()) {
    return Npc;
  }

  Npc.NpcId = Object->GetStringField(TEXT("npcId"));
  Npc.Persona = Object->GetStringField(TEXT("persona"));
  Npc.DataJson = JsonStringFromField(Object, TEXT("data"));
  return Npc;
}

} // namespace JsonInterop
