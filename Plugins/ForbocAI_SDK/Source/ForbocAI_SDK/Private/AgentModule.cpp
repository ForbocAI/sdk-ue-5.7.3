#include "AgentModule.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

// ==========================================
// AGENT FACTORY — Pure construction functions
// ==========================================

FAgent AgentFactory::Create(const FAgentConfig &Config) {
  const FString NewId =
      FString::Printf(TEXT("agent_%s"), *FGuid::NewGuid().ToString());
  const FString Url = Config.ApiUrl.IsEmpty() ? TEXT("http://localhost:8080")
                                              : Config.ApiUrl;

  return FAgent{NewId, Config.Persona, Config.InitialState,
                TArray<FMemoryItem>(), Url};
}

FAgent AgentFactory::FromSoul(const FSoul &Soul, const FString &ApiUrl) {
  const FString Url =
      ApiUrl.IsEmpty() ? TEXT("http://localhost:8080") : ApiUrl;

  return FAgent{Soul.Id, Soul.Persona, Soul.State, Soul.Memories, Url};
}

// ==========================================
// AGENT OPERATIONS — Pure free functions
// ==========================================

FAgent AgentOps::WithState(const FAgent &Agent,
                           const FAgentState &NewState) {
  return FAgent{Agent.Id, Agent.Persona, NewState, Agent.Memories,
                Agent.ApiUrl};
}

FAgent AgentOps::WithMemories(const FAgent &Agent,
                              const TArray<FMemoryItem> &NewMemories) {
  return FAgent{Agent.Id, Agent.Persona, Agent.State, NewMemories,
                Agent.ApiUrl};
}

FAgentState AgentOps::CalculateNewState(const FAgentState &Current,
                                        const FAgentState &Updates) {
  // Build new inventory by merging
  TArray<FString> NewInventory = Current.Inventory;
  for (const FString &Item : Updates.Inventory) {
    NewInventory.AddUnique(Item);
  }

  // Build new skills by merging
  TMap<FString, float> NewSkills = Current.Skills;
  for (const auto &Pair : Updates.Skills) {
    NewSkills.Add(Pair.Key, Pair.Value);
  }

  // Build new relationships by merging
  TMap<FString, float> NewRelationships = Current.Relationships;
  for (const auto &Pair : Updates.Relationships) {
    NewRelationships.Add(Pair.Key, Pair.Value);
  }

  // Determine mood
  const FString NewMood =
      Updates.Mood.IsEmpty() ? Current.Mood : Updates.Mood;

  return TypeFactory::AgentState(NewMood, MoveTemp(NewInventory),
                                 MoveTemp(NewSkills),
                                 MoveTemp(NewRelationships));
}

FAgentResponse AgentOps::Process(const FAgent &Agent, const FString &Input,
                                 const TMap<FString, FString> &Context) {
  // Pure transformation logic (simulation)
  // In production, this calls the Cortex/API endpoint.
  const FString Directive = TEXT("Respond normally.");
  const FString Instruction = TEXT("IDLE");

  const FString Dialogue = FString::Printf(
      TEXT("Processed: '%s' (Mood: %s)"), *Input, *Agent.State.Mood);
  const FString Thought =
      FString::Printf(TEXT("Directive: %s"), *Directive);

  return FAgentResponse{Dialogue, TypeFactory::Action(Instruction, Directive),
                        Thought};
}

FSoul AgentOps::Export(const FAgent &Agent) {
  return TypeFactory::Soul(Agent.Id, TEXT("1.0.0"), TEXT("Agent"),
                           Agent.Persona, Agent.State, Agent.Memories);
}
