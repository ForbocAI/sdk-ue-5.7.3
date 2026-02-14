#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

/**
 * CLI Operations - Functional implementation of CLI commands.
 */
namespace CLIOps {
/**
 * Prints status of the system (Doctor).
 */
FORBOCAI_SDK_API void Doctor(const FString &ApiUrl);

/**
 * Lists all agents.
 */
FORBOCAI_SDK_API void ListAgents(const FString &ApiUrl);

/**
 * Creates an agent.
 */
FORBOCAI_SDK_API void CreateAgent(const FString &ApiUrl,
                                  const FString &Persona);

/**
 * Sends a single message to an agent.
 */
FORBOCAI_SDK_API void ProcessAgent(const FString &ApiUrl,
                                   const FString &AgentId,
                                   const FString &Input);

/**
 * Exports a soul.
 */
FORBOCAI_SDK_API void ExportSoul(const FString &ApiUrl, const FString &AgentId);
} // namespace CLIOps
