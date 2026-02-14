#pragma once

#include "CoreMinimal.h"
#include "Core/functional_core.hpp" // C++11 Functional Core Library
#include "ForbocAI_SDK_Types.h"

/**
 * Agent Entity - Pure Data Structure.
 * Contains all state required for an autonomous entity.
 */
struct FAgent {
  const FString Id;
  const FString Persona;
  const FAgentState State;
  const TArray<FMemoryItem> Memories;

  // Configuration constraints (immutable)
  const FString ApiUrl;

  // Constructors for immutability
  FAgent(FString InId, FString InPersona, FAgentState InState,
         TArray<FMemoryItem> InMemories, FString InApiUrl)
      : Id(InId), Persona(InPersona), State(InState), Memories(InMemories),
        ApiUrl(InApiUrl) {}

  // Copy with new State (Functional Update)
  FAgent WithState(const FAgentState &NewState) const {
    return FAgent(Id, Persona, NewState, Memories, ApiUrl);
  }

  // Copy with new Memories (Functional Update)
  FAgent WithMemories(const TArray<FMemoryItem> &NewMemories) const {
    return FAgent(Id, Persona, State, NewMemories, ApiUrl);
  }
};

/**
 * Factory for creating Agents.
 */
namespace AgentFactory {
/**
 * Creates a new Agent instance.
 * Pure function: Config -> FAgent
 */
FORBOCAI_SDK_API FAgent Create(const FAgentConfig &Config);

/**
 * Creates an Agent from a Soul.
 * Pure function: Soul -> FAgent
 */
FORBOCAI_SDK_API FAgent FromSoul(const FSoul &Soul, const FString &ApiUrl);
} // namespace AgentFactory

/**
 * Operations on Agents.
 * Stateless namespaces containing pure functions (or impure functions
 * explicitly marked).
 */
namespace AgentOps {
/**
 * Pure: Calculates the next state based on updates.
 */
FORBOCAI_SDK_API FAgentState CalculateNewState(const FAgentState &Current,
                                               const FAgentState &Updates);

/**
 * Impure/Async: Processes input and returns a response.
 * This involves Side Effects (API Usage), but logic remains separated.
 *
 * In a strict FP world, this might return an IO or Task Monad.
 * For UE compatibility, we use a callback or latent action style,
 * but here we expose the blocking logic for simplicity or wrapping.
 */
FORBOCAI_SDK_API FAgentResponse Process(const FAgent &Agent,
                                        const FString &Input,
                                        const TMap<FString, FString> &Context);

/**
 * Pure: Exports Agent to Soul.
 */
FORBOCAI_SDK_API FSoul Export(const FAgent &Agent);
} // namespace AgentOps
