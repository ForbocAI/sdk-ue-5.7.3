#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Types.h"

namespace JsonInterop {

/**
 * Parses a JSON object string, defaulting to an empty object on blank input.
 * User Story: As JSON-backed runtime parsing, I need safe object parsing so
 * blank or malformed payloads do not crash reducers and thunks.
 */
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

/**
 * Parses a JSON array string into UE JSON values.
 * User Story: As JSON-backed runtime parsing, I need array payloads decoded so
 * API responses and config payloads can be traversed safely.
 */
inline bool ParseJsonArray(const FString &Json,
                           TArray<TSharedPtr<FJsonValue>> &OutArray) {
  TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
  return FJsonSerializer::Deserialize(Reader, OutArray);
}

/**
 * Serializes a JSON object to a string, using `{}` when the pointer is null.
 * User Story: As JSON serialization flows, I need null-safe object encoding so
 * outbound payload construction stays predictable.
 */
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

/**
 * Serializes a JSON array to a string.
 * User Story: As JSON serialization flows, I need array values encoded so
 * runtime payloads can move between structs and HTTP bodies.
 */
inline FString StringifyArray(const TArray<TSharedPtr<FJsonValue>> &Array) {
  FString Json;
  const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
  FJsonSerializer::Serialize(Array, Writer);
  return Json;
}

/**
 * Serializes a generic JSON value to its string representation.
 * User Story: As JSON normalization flows, I need one value serializer so
 * mixed-type fields can be preserved without custom branching everywhere.
 */
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

/**
 * Returns true when an object contains a field whose value is not JSON null.
 * User Story: As defensive JSON readers, I need nullable field checks so
 * optional payload fields can be handled without unsafe assumptions.
 */
inline bool HasNonNullField(const TSharedPtr<FJsonObject> &Object,
                            const FString &FieldName) {
  if (!Object.IsValid() || !Object->HasField(FieldName)) {
    return false;
  }

  const TSharedPtr<FJsonValue> Value = Object->TryGetField(FieldName);
  return Value.IsValid() && Value->Type != EJson::Null;
}

/**
 * Reads a field as a string, falling back to serialized JSON or a default.
 * User Story: As JSON field readers, I need a tolerant string extractor so
 * scalar and structured fields can be consumed through one helper.
 */
inline FString OptionalStringFromField(const TSharedPtr<FJsonObject> &Object,
                                       const FString &FieldName,
                                       const FString &Fallback = TEXT("")) {
  if (!HasNonNullField(Object, FieldName)) {
    return Fallback;
  }

  const TSharedPtr<FJsonValue> Value = Object->TryGetField(FieldName);
  if (!Value.IsValid()) {
    return Fallback;
  }

  if (Value->Type == EJson::String) {
    return Value->AsString();
  }

  return StringifyValue(Value);
}

/**
 * Parses an object string and always returns a valid JSON object pointer.
 * User Story: As downstream JSON consumers, I need a guaranteed object handle
 * so later helpers can operate without repeated null checks.
 */
inline TSharedPtr<FJsonObject> ParseJsonObjectOrEmpty(const FString &Json) {
  TSharedPtr<FJsonObject> Object;
  ParseJsonObject(Json, Object);
  return Object.IsValid() ? Object : MakeShared<FJsonObject>();
}

/**
 * Reads a field as JSON text, preserving objects and arrays when present.
 * User Story: As payload passthrough flows, I need raw JSON field extraction so
 * nested state blobs can survive round-trips without lossy conversion.
 */
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

/**
 * Writes a JSON string field back as object, array, or plain string content.
 * User Story: As payload assembly flows, I need JSON text rehydrated into the
 * right field type so outbound request bodies keep their structure.
 */
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

/**
 * Wraps a JSON object as an agent-state payload.
 * User Story: As state-bridging code, I need JSON objects converted into
 * `FAgentState` values so state can move through strongly typed APIs.
 */
inline FAgentState StateFromObject(const TSharedPtr<FJsonObject> &Object) {
  return TypeFactory::AgentState(StringifyObject(Object));
}

/**
 * Reads a named JSON field and wraps it as an agent-state payload.
 * User Story: As state-bridging code, I need nested state fields lifted into
 * `FAgentState` values so reducers can reuse them without manual parsing.
 */
inline FAgentState StateFromField(const TSharedPtr<FJsonObject> &Object,
                                  const FString &FieldName) {
  return TypeFactory::AgentState(JsonStringFromField(Object, FieldName));
}

/**
 * Parses stored agent-state JSON back into an object.
 * User Story: As state inspection flows, I need `FAgentState` decoded back to
 * JSON objects so nested fields can be inspected and transformed.
 */
inline TSharedPtr<FJsonObject> StateToObject(const FAgentState &State) {
  return ParseJsonObjectOrEmpty(State.JsonData);
}

/**
 * Serializes an agent action into the API object shape.
 * User Story: As protocol request builders, I need agent actions encoded into
 * the API schema so remote validation and processing can consume them.
 */
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

/**
 * Deserializes an API action object, including legacy field aliases.
 * User Story: As protocol response readers, I need action payloads normalized
 * so legacy and current server fields map into one runtime shape.
 */
inline FAgentAction ActionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FAgentAction Action;
  if (!Object.IsValid()) {
    return Action;
  }

  /**
   * Normalize API field names: the Haskell API may return gaType/actionTarget/
   * actionReason instead of type/target/reason. Match TS postVerdict transform.
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  Action.Type = OptionalStringFromField(Object, TEXT("gaType"));
  if (Action.Type.IsEmpty()) {
    Action.Type = OptionalStringFromField(Object, TEXT("type"));
  }
  Action.Target = OptionalStringFromField(Object, TEXT("actionTarget"));
  if (Action.Target.IsEmpty()) {
    Action.Target = OptionalStringFromField(Object, TEXT("target"));
  }
  Action.Reason = OptionalStringFromField(Object, TEXT("actionReason"));
  if (Action.Reason.IsEmpty()) {
    Action.Reason = OptionalStringFromField(Object, TEXT("reason"));
  }

  double Confidence = 1.0;
  if (Object->TryGetNumberField(TEXT("confidence"), Confidence)) {
    Action.Confidence = static_cast<float>(Confidence);
  }
  Action.Signature = OptionalStringFromField(Object, TEXT("signature"));
  Action.PayloadJson = JsonStringFromField(Object, TEXT("payload"), TEXT("{}"));
  return Action;
}

/**
 * Serializes a memory-store instruction into API JSON.
 * User Story: As memory request builders, I need instructions encoded into the
 * API schema so remote memory writes carry the expected fields.
 */
inline TSharedRef<FJsonObject>
MemoryStoreInstructionToObject(const FMemoryStoreInstruction &Instruction) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  Object->SetStringField(TEXT("text"), Instruction.Text);
  Object->SetStringField(TEXT("type"), Instruction.Type);
  Object->SetNumberField(TEXT("importance"), Instruction.Importance);
  return Object;
}

/**
 * Deserializes a memory-store instruction from API JSON.
 * User Story: As memory response readers, I need store instructions decoded so
 * runtime flows can inspect or replay persisted write intents.
 */
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

/**
 * Serializes a recalled memory record into API JSON.
 * User Story: As recall payload builders, I need recalled memories encoded so
 * context assembly can include structured memory data.
 */
inline TSharedRef<FJsonObject> RecalledMemoryToObject(const FRecalledMemory &Memory) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  Object->SetStringField(TEXT("text"), Memory.Text);
  Object->SetStringField(TEXT("type"), Memory.Type);
  Object->SetNumberField(TEXT("importance"), Memory.Importance);
  Object->SetNumberField(TEXT("similarity"), Memory.Similarity);
  return Object;
}

/**
 * Deserializes a recalled memory record from API JSON.
 * User Story: As recall response readers, I need recalled memories decoded so
 * prompt builders can consume similarity-ranked memory data.
 */
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

/**
 * Serializes a stored memory item into API JSON.
 * User Story: As memory persistence flows, I need memory items encoded so
 * local and remote stores can share one interchange format.
 */
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

/**
 * Deserializes a stored memory item from API JSON.
 * User Story: As memory readers, I need stored items decoded so runtime state
 * can rebuild memory entities from transport payloads.
 */
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

/**
 * Serializes cortex generation config into API JSON.
 * User Story: As cortex request builders, I need generation config encoded so
 * remote and local completion requests share the same config shape.
 */
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

/**
 * Deserializes cortex generation config from API JSON.
 * User Story: As cortex response readers, I need generation config decoded so
 * returned settings can be reapplied or inspected in runtime state.
 */
inline FCortexConfig CortexConfigFromObject(const TSharedPtr<FJsonObject> &Object) {
  FCortexConfig Config;
  if (!Object.IsValid()) {
    return Config;
  }

  Config.Model = OptionalStringFromField(Object, TEXT("model"));
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

/**
 * Serializes bridge validation context payloads into API JSON.
 * User Story: As bridge validation requests, I need context encoded so the
 * validator can inspect NPC, world, and constraint state together.
 */
inline TSharedRef<FJsonObject>
ValidationContextToObject(const FBridgeValidationContext &Context) {
  const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
  SetFieldFromJsonString(Object, TEXT("npcState"), Context.NpcStateJson);
  SetFieldFromJsonString(Object, TEXT("worldState"), Context.WorldStateJson);
  SetFieldFromJsonString(Object, TEXT("constraints"), Context.ConstraintsJson);
  return Object;
}

/**
 * Deserializes a bridge validation response into SDK result state.
 * User Story: As bridge validation readers, I need response payloads decoded
 * so runtime code can surface corrected actions and rejection reasons.
 */
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
  Result.Reason = OptionalStringFromField(Object, TEXT("reason"));
  if (Object->HasTypedField<EJson::Object>(TEXT("correctedAction"))) {
    Result.CorrectedAction =
        ActionFromObject(Object->GetObjectField(TEXT("correctedAction")));
  }
  return Result;
}

/**
 * Deserializes an Arweave upload instruction from API JSON.
 * User Story: As soul export flows, I need upload instructions decoded so the
 * client can perform the correct signed upload handshake.
 */
inline FArweaveUploadInstruction
UploadInstructionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FArweaveUploadInstruction Instruction;
  if (!Object.IsValid()) {
    return Instruction;
  }

  Instruction.UploadUrl = OptionalStringFromField(Object, TEXT("auiEndpoint"));
  Instruction.ContentType =
      OptionalStringFromField(Object, TEXT("auiContentType"));
  Instruction.AuiAuthHeader =
      OptionalStringFromField(Object, TEXT("auiAuthHeader"));
  Instruction.PayloadJson = JsonStringFromField(Object, TEXT("auiPayload"), TEXT("{}"));
  return Instruction;
}

/**
 * Serializes an Arweave upload result into API JSON.
 * User Story: As soul export confirmation flows, I need upload results encoded
 * so the server can verify what happened during the upload step.
 */
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

/**
 * Deserializes an Arweave download instruction from API JSON.
 * User Story: As soul import flows, I need download instructions decoded so
 * the client can fetch the expected transaction from the right gateway.
 */
inline FArweaveDownloadInstruction
DownloadInstructionFromObject(const TSharedPtr<FJsonObject> &Object) {
  FArweaveDownloadInstruction Instruction;
  if (!Object.IsValid()) {
    return Instruction;
  }

  Instruction.GatewayUrl =
      OptionalStringFromField(Object, TEXT("adiGatewayUrl"));
  Instruction.TxId = OptionalStringFromField(Object, TEXT("adiExpectedTxId"));
  Instruction.ExpectedTxId = Instruction.TxId;
  return Instruction;
}

/**
 * Serializes an Arweave download result into API JSON.
 * User Story: As soul import confirmation flows, I need download results
 * encoded so later steps can validate the fetched payload.
 */
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

/**
 * Deserializes a soul payload from API JSON.
 * User Story: As soul import and listing flows, I need souls decoded so NPC
 * state and memories can be reconstructed from transport payloads.
 */
inline FSoul SoulFromObject(const TSharedPtr<FJsonObject> &Object) {
  FSoul Soul;
  if (!Object.IsValid()) {
    return Soul;
  }

  Soul.Id = OptionalStringFromField(Object, TEXT("id"));
  Soul.Version = OptionalStringFromField(Object, TEXT("version"));
  Soul.Name = OptionalStringFromField(Object, TEXT("name"));
  Soul.Persona = OptionalStringFromField(Object, TEXT("persona"));
  Soul.Signature = OptionalStringFromField(Object, TEXT("signature"));
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

/**
 * Deserializes an imported-NPC payload from API JSON.
 * User Story: As soul import flows, I need imported NPC results decoded so the
 * terminal and runtime can inspect the created NPC record.
 */
inline FImportedNpc ImportedNpcFromObject(const TSharedPtr<FJsonObject> &Object) {
  FImportedNpc Npc;
  if (!Object.IsValid()) {
    return Npc;
  }

  Npc.NpcId = OptionalStringFromField(Object, TEXT("npcId"));
  Npc.Persona = OptionalStringFromField(Object, TEXT("persona"));
  Npc.DataJson = JsonStringFromField(Object, TEXT("data"));
  return Npc;
}

} // namespace JsonInterop
