#pragma once

#include "Core/functional_core.hpp" // C++11 Functional Core Library
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

// ==========================================================
// Agent Module — Strict FP
// ==========================================================
// FAgent is a pure data struct with const members (immutable).
// NO constructors, NO member functions.
// Construction via AgentFactory namespace (factory functions).
// Operations via AgentOps namespace (free functions).
// ==========================================================

/**
 * Agent Entity — Pure immutable data.
 * All members are const: once created, an agent cannot be mutated.
 * Use AgentOps::WithState / AgentOps::WithMemories for
 * functional updates (returns a new FAgent).
 */
struct FAgent {
  /** Unique identifier for the agent. */
  const FString Id;
  /** The persona or description of the agent. */
  const FString Persona;
  /** The current state of the agent. */
  const FAgentState State;
  /** The list of memories associated with the agent. */
  const TArray<FMemoryItem> Memories;
  /** The API URL this agent communicates with. */
  const FString ApiUrl;
};

/**
 * Factory functions for creating Agents.
 */
namespace AgentFactory {

/**
 * Creates a new Agent from configuration.
 * Pure function: Config -> FAgent
 * @param Config The configuration to transform into an Agent.
 * @return A new immutable FAgent instance.
 */
FORBOCAI_SDK_API FAgent Create(const FAgentConfig &Config);

/**
 * Creates an Agent from a Soul.
 * Pure function: (Soul, ApiUrl) -> FAgent
 * @param Soul The Soul to rehydrate an Agent from.
 * @param ApiUrl The API URL to bind the Agent to.
 * @return A rehydrated FAgent instance.
 */
FORBOCAI_SDK_API FAgent FromSoul(const FSoul &Soul, const FString &ApiUrl);

} // namespace AgentFactory

/**
 * Operations on Agents — stateless free functions.
 */
namespace AgentOps {

/**
 * Pure: Returns a new Agent with updated State.
 * Functional update — original is not modified.
 * @param Agent The original Agent.
 * @param NewState The new state to apply.
 * @return A new FAgent with the updated state.
 */
FORBOCAI_SDK_API FAgent WithState(const FAgent &Agent,
                                  const FAgentState &NewState);

/**
 * Pure: Returns a new Agent with updated Memories.
 * Functional update — original is not modified.
 * @param Agent The original Agent.
 * @param NewMemories The new list of memories.
 * @return A new FAgent with updated memories.
 */
FORBOCAI_SDK_API FAgent WithMemories(const FAgent &Agent,
                                     const TArray<FMemoryItem> &NewMemories);

/**
 * Pure: Calculates the next state by merging updates into current.
 * @param Current The current state.
 * @param Updates The partial state updates to apply.
 * @return The merged new state.
 */
FORBOCAI_SDK_API FAgentState CalculateNewState(const FAgentState &Current,
                                               const FAgentState &Updates);

/**
 * Impure: Processes input and returns a response.
 * Side effects (API call) are isolated here.
 * In strict FP this would return an IO monad.
 * @param Agent The agent to process the input.
 * @param Input The input string from the user/world.
 * @param Context Additional context for the processing.
 * @return The response including dialogue and action.
 */
FORBOCAI_SDK_API FAgentResponse Process(const FAgent &Agent,
                                        const FString &Input,
                                        const TMap<FString, FString> &Context);

/**
 * Pure: Exports Agent data to a Soul.
 * @param Agent The agent to export.
 * @return A new FSoul containing the agent's data.
 */
FORBOCAI_SDK_API FSoul Export(const FAgent &Agent);

} // namespace AgentOps
