#include "SoulModule.h"
#include "JsonObjectConverter.h"

FSoul SoulOps::FromAgent(const FAgentState &State,
                         const TArray<FMemoryItem> &Memories,
                         const FString &AgentId, const FString &Persona) {
  FSoul Soul;
  Soul.Id = AgentId;
  Soul.Version = TEXT("1.0.0");
  Soul.Name = TEXT("Agent Soul");
  Soul.Persona = Persona;
  Soul.State = State;
  Soul.Memories = Memories;

  return Soul;
}

FString SoulOps::Serialize(const FSoul &Soul) {
  FString JsonString;
  FJsonObjectConverter::UStructToJsonObjectString(Soul, JsonString);
  return JsonString;
}

FSoul SoulOps::Deserialize(const FString &Json) {
  FSoul Soul;
  if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Soul, 0, 0)) {
    // Return empty/invalid soul on failure
    return FSoul();
  }
  return Soul;
}

FValidationResult SoulOps::Validate(const FSoul &Soul) {
  if (Soul.Id.IsEmpty()) {
    return FValidationResult(false, TEXT("Missing Soul ID"));
  }
  if (Soul.Persona.IsEmpty()) {
    return FValidationResult(false, TEXT("Missing Persona"));
  }

  return FValidationResult(true, TEXT("Valid Soul"));
}
