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
 * @param State The agent's state.
 * @param Memories The agent's memories.
 * @param AgentId The agent's ID.
 * @param Persona The agent's persona.
 * @return A new FSoul instance.
 */
FORBOCAI_SDK_API FSoul FromAgent(const FAgentState &State,
                                 const TArray<FMemoryItem> &Memories,
                                 const FString &AgentId,
                                 const FString &Persona);

/**
 * Serializes Soul to JSON string.
 * Pure function.
 * @param Soul The Soul object to serialize.
 * @return A JSON string representation of the Soul.
 */
FORBOCAI_SDK_API FString Serialize(const FSoul &Soul);

/**
 * Deserializes Soul from JSON string.
 * Pure function (returns default/empty on failure).
 * @param Json The JSON string to deserialize.
 * @return A new FSoul instance.
 */
FORBOCAI_SDK_API FSoul Deserialize(const FString &Json);

/**
 * Validates a Soul structure.
 * Pure function.
 * @param Soul The Soul object to validate.
 * @return A validation result.
 */
FORBOCAI_SDK_API FValidationResult Validate(const FSoul &Soul);

/**
 * Triggers a Soul export to Arweave via the API.
 * Async function.
 * @param Soul The soul to export.
 * @param ApiUrl The endpoint URL.
 * @param OnComplete Callback with TXID or Error.
 */
FORBOCAI_SDK_API void ExportToArweave(const FSoul &Soul, const FString &ApiUrl,
                                      TFunction<void(FString)> OnComplete);
} // namespace SoulOps
