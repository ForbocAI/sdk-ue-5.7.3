#pragma once

#include "CoreMinimal.h"
#include "Core/functional_core.hpp" // C++11 Functional Core Library
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
  const FString Id;
  const FString Persona;
  const FAgentState State;
  const TArray<FMemoryItem> Memories;
  const FString ApiUrl;
};

/**
 * Factory functions for creating Agents.
 */
namespace AgentFactory {

/**
 * Creates a new Agent from configuration.
 * Pure function: Config -> FAgent
 */
FORBOCAI_SDK_API FAgent Create(const FAgentConfig &Config);

/**
 * Creates an Agent from a Soul.
 * Pure function: (Soul, ApiUrl) -> FAgent
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
 */
FORBOCAI_SDK_API FAgent WithState(const FAgent &Agent,
                                  const FAgentState &NewState);

/**
 * Pure: Returns a new Agent with updated Memories.
 * Functional update — original is not modified.
 */
FORBOCAI_SDK_API FAgent WithMemories(const FAgent &Agent,
                                     const TArray<FMemoryItem> &NewMemories);

/**
 * Pure: Calculates the next state by merging updates into current.
 */
FORBOCAI_SDK_API FAgentState CalculateNewState(const FAgentState &Current,
                                               const FAgentState &Updates);

/**
 * Impure: Processes input and returns a response.
 * Side effects (API call) are isolated here.
 * In strict FP this would return an IO monad.
 */
FORBOCAI_SDK_API FAgentResponse Process(const FAgent &Agent,
                                        const FString &Input,
                                        const TMap<FString, FString> &Context);

/**
 * Pure: Exports Agent data to a Soul.
 */
FORBOCAI_SDK_API FSoul Export(const FAgent &Agent);

} // namespace AgentOps
