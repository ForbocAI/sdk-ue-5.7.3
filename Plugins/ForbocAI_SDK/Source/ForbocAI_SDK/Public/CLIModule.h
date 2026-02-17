#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

/**
 * CLI Operations - Functional implementation of CLI commands.
 */
namespace CLIOps {
/**
 * Prints status of the system (Doctor).
 * @param ApiUrl The API endpoint to check.
 */
FORBOCAI_SDK_API void Doctor(const FString &ApiUrl);

/**
 * Lists all agents.
 * @param ApiUrl The API endpoint.
 */
FORBOCAI_SDK_API void ListAgents(const FString &ApiUrl);

/**
 * Creates an agent.
 * @param ApiUrl The API endpoint.
 * @param Persona The persona of the new agent.
 */
FORBOCAI_SDK_API void CreateAgent(const FString &ApiUrl,
                                  const FString &Persona);

/**
 * Sends a single message to an agent.
 * @param ApiUrl The API endpoint.
 * @param AgentId The ID of the target agent.
 * @param Input The input message/command.
 */
FORBOCAI_SDK_API void ProcessAgent(const FString &ApiUrl,
                                   const FString &AgentId,
                                   const FString &Input);

/**
 * Exports a soul.
 * @param ApiUrl The API endpoint.
 * @param AgentId The ID of the agent to export.
 */
FORBOCAI_SDK_API void ExportSoul(const FString &ApiUrl, const FString &AgentId);
} // namespace CLIOps
