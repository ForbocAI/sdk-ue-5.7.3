#include "AgentModule.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

// ==========================================
// AGENT FACTORY
// ==========================================

FAgent AgentFactory::Create(const FAgentConfig &Config) {
  // Generate new ID
  FString NewId =
      FString::Printf(TEXT("agent_%s"), *FGuid::NewGuid().ToString());

  // Return new Immutable Agent
  return FAgent(NewId, Config.Persona, Config.InitialState,
                TArray<FMemoryItem>(), // Empty memories
                Config.ApiUrl.IsEmpty() ? TEXT("http://localhost:8080")
                                        : Config.ApiUrl);
}

FAgent AgentFactory::FromSoul(const FSoul &Soul, const FString &ApiUrl) {
  return FAgent(Soul.Id, Soul.Persona, Soul.State, Soul.Memories,
                ApiUrl.IsEmpty() ? TEXT("http://localhost:8080") : ApiUrl);
}

// ==========================================
// AGENT OPERATIONS
// ==========================================

FAgentState AgentOps::CalculateNewState(const FAgentState &Current,
                                        const FAgentState &Updates) {
  // Deep Copy Current
  FAgentState NewState = Current;

  // Apply updates immutably (effectively merging)
  if (!Updates.Mood.IsEmpty()) {
    NewState.Mood = Updates.Mood;
  }

  // Inventory: Append updates
  for (const FString &Item : Updates.Inventory) {
    NewState.Inventory.AddUnique(Item);
  }

  // Skills: Merge
  for (const auto &Pair : Updates.Skills) {
    NewState.Skills.Add(Pair.Key, Pair.Value);
  }

  // Relationships: Merge
  for (const auto &Pair : Updates.Relationships) {
    NewState.Relationships.Add(Pair.Key, Pair.Value);
  }

  return NewState;
}

FAgentResponse AgentOps::Process(const FAgent &Agent, const FString &Input,
                                 const TMap<FString, FString> &Context) {
  // 1. Prepare Context (Pure Transformation)
  // In a real implementation, we would call the Cortex/API here.
  // Since we don't have the heavy local libs in this C++ port (as per
  // instructions), we will simulate the logic or call the HTTP endpoint.

  // For now, implementing the pure transformation logic:

  FString Directive = TEXT("Respond normally.");
  FString Instruction = TEXT("IDLE");

  // TODO: Implement HTTP Call to configured API URL for Directive (The "Mind")
  // Note: In C++ UE, HTTP is async. This function signature implies blocking or
  // sync logic. For a strict FP implementation, we'd pass a continuation or
  // return a Future.

  // 2. Generate Response
  FAgentResponse Response;
  Response.Dialogue = FString::Printf(TEXT("Processed: '%s' (Mood: %s)"),
                                      *Input, *Agent.State.Mood);
  Response.Action = FAgentAction(Instruction, Directive);
  Response.Thought = FString::Printf(TEXT("Directive: %s"), *Directive);

  return Response;
}

FSoul AgentOps::Export(const FAgent &Agent) {
  FSoul Soul;
  Soul.Id = Agent.Id;
  Soul.Version = TEXT("1.0.0");
  Soul.Name = TEXT("Agent"); // Default
  Soul.Persona = Agent.Persona;
  Soul.State = Agent.State;
  Soul.Memories = Agent.Memories;

  return Soul;
}
