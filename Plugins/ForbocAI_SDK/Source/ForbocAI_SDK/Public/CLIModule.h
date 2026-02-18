#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "Core/functional_core.hpp"

// Functional Core Type Aliases for CLI operations
namespace CLITypes {
using func::AsyncResult;
using func::Either;
using func::Maybe;
using func::TestResult;
using func::ValidationPipeline;

using func::make_left;
using func::make_right;
} // namespace CLITypes

/**
 * CLI Operations - Functional implementation of CLI commands.
 */
namespace CLIOps {

/**
 * Prints status of the system (Doctor).
 * @param ApiUrl The API endpoint to check.
 * @return A TestResult indicating success or failure.
 */
FORBOCAI_SDK_API CLITypes::TestResult<void> Doctor(const FString &ApiUrl);

/**
 * Lists all agents.
 * @param ApiUrl The API endpoint.
 * @return A TestResult indicating success or failure.
 */
FORBOCAI_SDK_API CLITypes::TestResult<void> ListAgents(const FString &ApiUrl);

/**
 * Creates an agent.
 * @param ApiUrl The API endpoint.
 * @param Persona The persona of the new agent.
 * @return A TestResult indicating success or failure.
 */
FORBOCAI_SDK_API CLITypes::TestResult<void> CreateAgent(const FString &ApiUrl,
                                                        const FString &Persona);

/**
 * Sends a single message to an agent.
 * @param ApiUrl The API endpoint.
 * @param AgentId The ID of the target agent.
 * @param Input The input message/command.
 * @return A TestResult indicating success or failure.
 */
FORBOCAI_SDK_API CLITypes::TestResult<void> ProcessAgent(const FString &ApiUrl,
                                                         const FString &AgentId,
                                                         const FString &Input);

/**
 * Exports a soul.
 * @param ApiUrl The API endpoint.
 * @param AgentId The ID of the agent to export.
 * @return A TestResult indicating success or failure.
 */
FORBOCAI_SDK_API CLITypes::TestResult<void> ExportSoul(const FString &ApiUrl,
                                                       const FString &AgentId);

} // namespace CLIOps
