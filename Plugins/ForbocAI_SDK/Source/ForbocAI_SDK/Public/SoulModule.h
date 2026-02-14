#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include <functional>

/**
 * Soul Operations - IPFS/Serialization Logic.
 */
namespace SoulOps {
/**
 * Creates a Soul from an Agent.
 * Pure function.
 */
FORBOCAI_SDK_API FSoul FromAgent(const FAgentState &State,
                                 const TArray<FMemoryItem> &Memories,
                                 const FString &AgentId,
                                 const FString &Persona);

/**
 * Serializes Soul to JSON string.
 * Pure function.
 */
FORBOCAI_SDK_API FString Serialize(const FSoul &Soul);

/**
 * Deserializes Soul from JSON string.
 * Pure function (returns default/empty on failure).
 */
FORBOCAI_SDK_API FSoul Deserialize(const FString &Json);

/**
 * Validates a Soul structure.
 * Pure function.
 */
FORBOCAI_SDK_API FValidationResult Validate(const FSoul &Soul);
} // namespace SoulOps
