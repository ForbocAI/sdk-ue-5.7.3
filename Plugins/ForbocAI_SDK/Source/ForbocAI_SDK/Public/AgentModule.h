#pragma once

#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Core/functional_core.hpp" // C++11 Functional Core Library
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "Templates/TFunction.h"

// ==========================================================
// Agent Module — Strict FP with Enhanced Functional Patterns
// ==========================================================
// FAgent is a pure data struct with const members (immutable).
// NO constructors, NO member functions.
// Construction via AgentFactory namespace (factory functions).
// Operations via AgentOps namespace (free functions).
// Enhanced with functional core patterns for better error handling
// and composition.
// ==========================================================

// Functional Core Type Aliases for Agent operations
namespace AgentTypes {
using func::AsyncResult;
using func::ConfigBuilder;
using func::Curried;
using func::Either;
using func::Lazy;
using func::Maybe;
using func::Pipeline;
using func::TestResult;
using func::ValidationPipeline;

using func::just;
using func::make_left;
using func::make_right;
using func::nothing;

// Type aliases for Agent operations
using AgentCreationResult = Either<FString, FAgent>;
using AgentValidationResult = Either<FString, FAgentState>;
using AgentProcessResult = Either<FString, FAgentResponse>;
using AgentExportResult = Either<FString, FSoul>;
} // namespace AgentTypes

// FAgent is defined in ForbocAI_SDK_Types.h

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
 * @return An AsyncResult containing the response.
 */
FORBOCAI_SDK_API AgentTypes::AsyncResult<FAgentResponse>
Process(const FAgent &Agent, const FString &Input,
        const TMap<FString, FString> &Context);

/**
 * Pure: Exports Agent data to a Soul.
 * @param Agent The agent to export.
 * @return A new FSoul containing the agent's data.
 */
FORBOCAI_SDK_API FSoul Export(const FAgent &Agent);

} // namespace AgentOps

// Functional Core Helper Functions for Agent operations
namespace AgentHelpers {
// Helper to create a lazy agent
inline AgentTypes::Lazy<FAgent> createLazyAgent(const FAgentConfig &config) {
  return func::lazy(
      [config]() -> FAgent { return AgentFactory::Create(config).right; });
}

// Helper to create a validation pipeline for agent state
inline AgentTypes::ValidationPipeline<FAgentState, FString>
agentStateValidationPipeline() {
  return func::validationPipeline<FAgentState, FString>()
      .add([](const FAgentState &state)
               -> AgentTypes::Either<FString, FAgentState> {
        if (state.JsonData.IsEmpty() || state.JsonData == TEXT("{}")) {
          return AgentTypes::make_left(FString(TEXT("Empty agent state")), FAgentState{});
        }
        return AgentTypes::make_right(FString(), state);
      })
      .add([](const FAgentState &state)
               -> AgentTypes::Either<FString, FAgentState> {
        return AgentTypes::make_right(FString(), state);
      });
}

// Helper to create a pipeline for agent processing
inline AgentTypes::Pipeline<FAgent>
agentProcessingPipeline(const FAgent &agent) {
  return func::pipe(agent);
}

// Helper to create a curried agent creation function
inline AgentTypes::Curried<
    1, std::function<AgentTypes::AgentCreationResult(FAgentConfig)>>
curriedAgentCreation() {
  return func::curry<1>(
      [](FAgentConfig config) -> AgentTypes::AgentCreationResult {
        try {
          auto result = AgentFactory::Create(config);
          return result;
        } catch (const std::exception &e) {
          return AgentTypes::make_left(FString(e.what()), FAgent{});
        }
      });
}
} // namespace AgentHelpers
