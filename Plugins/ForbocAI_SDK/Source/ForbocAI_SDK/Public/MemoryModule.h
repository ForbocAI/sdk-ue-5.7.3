#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "MemoryModule.generated.h"

// ==========================================================
// Memory Module — Full Embedding-Based Memory (UE SDK)
// ==========================================================
// Strict functional programming implementation for
// persistent, semantic recall using sqlite-vss for vector search.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

// Functional Core Type Aliases for Memory operations
namespace MemoryTypes {
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

// Type aliases for Memory operations
using MemoryStoreResult = Either<FString, FMemoryStore>;
using MemoryStoreCreationResult = Either<FString, FMemoryStore>;
using MemoryStoreInitializationResult = Either<FString, bool>;
using MemoryStoreAddResult = Either<FString, FMemoryStore>;
using MemoryStoreRecallResult = Either<FString, TArray<FMemoryItem>>;
using MemoryStoreEmbeddingResult = Either<FString, TArray<float>>;
} // namespace MemoryTypes

// Functional Core Helper Functions for Memory operations
// MemoryHelpers moved to end of file to ensure type visibility

// Types (FMemoryConfig, FMemoryStore) are defined in ForbocAI_SDK_Types.h

/**
 * Memory Operations — Stateless free functions.
 */
namespace MemoryOps {

// MemoryOps::CreateStore removed in favor of MemoryFactory::CreateStore

/**
 * Initializes the memory store and database.
 * Pure function: Store -> Result
 * @param Store The memory store to initialize.
 * @return A validation result indicating success or failure.
 */
FORBOCAI_SDK_API MemoryTypes::MemoryStoreInitializationResult
Initialize(FMemoryStore &Store);

/**
 * Stores a new memory.
 * Pure function: (Store, Text, Type, Importance) -> NewStore
 * @param Store The current memory store.
 * @param Text The memory content.
 * @param Type The memory type.
 * @param Importance The importance score.
 * @return A new memory store with the memory added.
 */
FORBOCAI_SDK_API MemoryTypes::MemoryStoreAddResult
Store(const FMemoryStore &Store, const FString &Text, const FString &Type,
      float Importance);

/**
 * Stores a pre-built memory item.
 * Pure function: (Store, Item) -> NewStore
 * @param Store The current memory store.
 * @param Item The memory item to add.
 * @return A new memory store with the item added.
 */
FORBOCAI_SDK_API MemoryTypes::MemoryStoreAddResult
Add(const FMemoryStore &Store, const FMemoryItem &Item);

/**
 * Performs semantic recall using vector search.
 * Pure function: (Store, Query, Limit) -> Results
 * @param Store The memory store to search.
 * @param Query The search query text.
 * @param Limit The maximum number of results to return.
 * @return The ranked memory results.
 */
FORBOCAI_SDK_API MemoryTypes::MemoryStoreRecallResult
Recall(const FMemoryStore &Store, const FString &Query, int32 Limit = -1);

/**
 * Generates a vector embedding for text.
 * Pure function: (Store, Text) -> Vector
 * @param Store The memory store.
 * @param Text The text to embed.
 * @return The vector embedding.
 */
FORBOCAI_SDK_API MemoryTypes::MemoryStoreEmbeddingResult
GenerateEmbedding(const FMemoryStore &Store, const FString &Text);

/**
 * Gets the current memory statistics.
 * Pure function: Store -> Statistics
 * @param Store The memory store.
 * @return The memory statistics.
 */
FORBOCAI_SDK_API FString GetStatistics(const FMemoryStore &Store);

/**
 * Cleans up resources and closes the database.
 * Pure function: Store -> Void
 * @param Store The memory store to clean up.
 */
FORBOCAI_SDK_API void Cleanup(FMemoryStore &Store);

} // namespace MemoryOps

// Memory Factory
namespace MemoryFactory {
/**
 * Factory: Creates a memory store.
 * Pure function: Config -> Store
 * @param Config The memory configuration.
 * @return A new memory store.
 */
FORBOCAI_SDK_API FMemoryStore CreateStore(const FMemoryConfig &Config);
} // namespace MemoryFactory

// Memory Helpers (Functional)
namespace MemoryHelpers {
// Helper to create a lazy memory store
inline MemoryTypes::Lazy<FMemoryStore>
createLazyMemoryStore(const FMemoryConfig &config) {
  return func::lazy([config]() -> FMemoryStore {
    return MemoryFactory::CreateStore(config);
  });
}

// Helper to create a validation pipeline for memory configuration
inline MemoryTypes::ValidationPipeline<FMemoryConfig, FString>
memoryConfigValidationPipeline() {
  return func::validationPipeline<FMemoryConfig, FString>()
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.DatabasePath.IsEmpty()) {
          return MemoryTypes::make_left(
              FString(TEXT("Database path cannot be empty")));
        }
        return MemoryTypes::make_right(config);
      })
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.MaxMemories < 1) {
          return MemoryTypes::make_left(
              FString(TEXT("Max memories must be at least 1")));
        }
        return MemoryTypes::make_right(config);
      })
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.VectorDimension != 384) {
          return MemoryTypes::make_left(
              FString(TEXT("Vector dimension must be 384")));
        }
        return MemoryTypes::make_right(config);
      })
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.MaxRecallResults < 1) {
          return MemoryTypes::make_left(
              FString(TEXT("Max recall results must be at least 1")));
        }
        return MemoryTypes::make_right(config);
      });
}

// Helper to create a pipeline for memory store creation
inline MemoryTypes::Pipeline<FMemoryStore>
memoryStoreCreationPipeline(const FMemoryStore &store) {
  return func::pipe(store);
}

// Helper to create a curried memory store creation function
inline MemoryTypes::Curried<
    1, std::function<MemoryTypes::MemoryStoreCreationResult(FMemoryConfig)>>
curriedMemoryStoreCreation() {
  return func::curry<1>(
      [](FMemoryConfig config) -> MemoryTypes::MemoryStoreCreationResult {
        try {
          FMemoryStore store = MemoryFactory::CreateStore(config);
          return MemoryTypes::make_right(FString(), store);
        } catch (const std::exception &e) {
          return MemoryTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace MemoryHelpers
