#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include <functional>

// ==========================================================
// Soul Module — IPFS/Serialization Logic (Strict FP)
// ==========================================================
// Strict functional programming implementation for Soul
// serialization, validation, and export.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

// Functional Core Type Aliases for Soul operations
namespace SoulTypes {
using func::AsyncResult;
using func::ConfigBuilder;
using func::Curried;
using func::Either;
using func::Lazy;
using func::Maybe;
using func::Pipeline;
using func::TestResult;
using func::ValidationPipeline;

using func::make_left;
using func::make_right;

// Type aliases for Soul operations
using SoulCreationResult = Either<FString, FSoul>;
using SoulSerializationResult = Either<FString, FString>;
using SoulDeserializationResult = Either<FString, FSoul>;
using SoulValidationResult = Either<FString, FSoul>;
using SoulExportResult = AsyncResult<FString>;
} // namespace SoulTypes

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
 * @return A Result containing the new FSoul instance or error.
 */
FORBOCAI_SDK_API SoulTypes::SoulCreationResult
FromAgent(const FAgentState &State, const TArray<FMemoryItem> &Memories,
          const FString &AgentId, const FString &Persona);

/**
 * Serializes Soul to JSON string.
 * Pure function.
 * @param Soul The Soul object to serialize.
 * @return A Result containing the JSON string or error.
 */
FORBOCAI_SDK_API SoulTypes::SoulSerializationResult
Serialize(const FSoul &Soul);

/**
 * Deserializes Soul from JSON string.
 * Pure function.
 * @param Json The JSON string to deserialize.
 * @return A Result containing the new FSoul instance or error.
 */
FORBOCAI_SDK_API SoulTypes::SoulDeserializationResult
Deserialize(const FString &Json);

/**
 * Validates a Soul structure.
 * Pure function.
 * @param Soul The Soul object to validate.
 * @return A validation result containing the Soul if valid, or error.
 */
FORBOCAI_SDK_API SoulTypes::SoulValidationResult Validate(const FSoul &Soul);

/**
 * Triggers a Soul export to Arweave via the API.
 * Async function returning an AsyncResult.
 * @param Soul The soul to export.
 * @param ApiUrl The endpoint URL.
 * @return An AsyncResult that can be chained with .then() and .catch_().
 */
FORBOCAI_SDK_API SoulTypes::SoulExportResult
ExportToArweave(const FSoul &Soul, const FString &ApiUrl);

} // namespace SoulOps

// Functional Core Helper Functions for Soul operations
namespace SoulHelpers {
// Helper to create a lazy soul from agent data
inline SoulTypes::Lazy<FSoul>
createLazySoul(const FAgentState &state, const TArray<FMemoryItem> &memories,
               const FString &id, const FString &persona) {
  return func::lazy([state, memories, id, persona]() -> FSoul {
    return SoulOps::FromAgent(state, memories, id, persona).right;
  });
}

// Helper to create a validation pipeline for Soul
inline SoulTypes::ValidationPipeline<FSoul, FString> soulValidationPipeline() {
  return func::validationPipeline<FSoul, FString>()
      .add([](const FSoul &soul) -> SoulTypes::Either<FString, FSoul> {
        if (soul.Id.IsEmpty()) {
          return SoulTypes::make_left(FString(TEXT("Missing Soul ID")), FSoul{});
        }
        return SoulTypes::make_right(FString(), soul);
      })
      .add([](const FSoul &soul) -> SoulTypes::Either<FString, FSoul> {
        if (soul.Persona.IsEmpty()) {
          return SoulTypes::make_left(FString(TEXT("Missing Persona")), FSoul{});
        }
        return SoulTypes::make_right(FString(), soul);
      });
}

// Helper to create a pipeline for soul serialization
inline SoulTypes::Pipeline<FSoul> soulSerializationPipeline(const FSoul &soul) {
  return func::pipe(soul);
}

// Helper to create a curried soul creation function
inline SoulTypes::Curried<
    4, std::function<SoulTypes::SoulCreationResult(
           FAgentState, TArray<FMemoryItem>, FString, FString)>>
curriedSoulCreation() {
  return func::curry<4>([](FAgentState state, TArray<FMemoryItem> memories,
                           FString id,
                           FString persona) -> SoulTypes::SoulCreationResult {
    return SoulOps::FromAgent(state, memories, id, persona);
  });
}
} // namespace SoulHelpers
