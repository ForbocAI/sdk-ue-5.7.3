#include "SoulModule.h"
#include "JsonObjectConverter.h"

// ==========================================
// SOUL OPERATIONS — Pure free functions
// ==========================================

FSoul SoulOps::FromAgent(const FAgentState &State,
                         const TArray<FMemoryItem> &Memories,
                         const FString &AgentId, const FString &Persona) {
  return TypeFactory::Soul(AgentId, TEXT("1.0.0"), TEXT("Agent Soul"), Persona,
                           State, Memories);
}

FString SoulOps::Serialize(const FSoul &Soul) {
  FString JsonString;
  FJsonObjectConverter::UStructToJsonObjectString(Soul, JsonString);
  return JsonString;
}

FSoul SoulOps::Deserialize(const FString &Json) {
  FSoul Soul;
  if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Soul, 0, 0)) {
    return FSoul();
  }
  return Soul;
}

FValidationResult SoulOps::Validate(const FSoul &Soul) {
  if (Soul.Id.IsEmpty()) {
    return TypeFactory::Invalid(TEXT("Missing Soul ID"));
  }
  if (Soul.Persona.IsEmpty()) {
    return TypeFactory::Invalid(TEXT("Missing Persona"));
  }

  return TypeFactory::Valid(TEXT("Valid Soul"));
}
