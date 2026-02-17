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
// ==========================================================

/**
 * Memory Configuration — Immutable data.
 */
struct FMemoryConfig {
  /** The database file path. */
  const FString DatabasePath;
  /** Maximum number of memories to store. */
  const int32 MaxMemories;
  /** Vector dimension size. */
  const int32 VectorDimension;
  /** Whether to use GPU acceleration if available. */
  const bool UseGPU;
  /** Maximum results to return from recall. */
  const int32 MaxRecallResults;

  FMemoryConfig()
      : DatabasePath(TEXT("ForbocAI_Memory.db")), MaxMemories(10000),
        VectorDimension(384), UseGPU(false), MaxRecallResults(10) {}
};

/**
 * Memory Store — Immutable data.
 */
USTRUCT()
struct FMemoryStore {
  GENERATED_BODY()

  /** List of immutable memory items. */
  UPROPERTY()
  TArray<FMemoryItem> Items;

  /** Database connection handle. */
  void *DatabaseHandle;

  /** Whether the store is initialized. */
  UPROPERTY()
  bool bInitialized;

  FMemoryStore() : DatabaseHandle(nullptr), bInitialized(false) {}
};

/**
 * Memory Operations — Stateless free functions.
 */
namespace MemoryOps {

/**
 * Factory: Creates a memory store.
 * Pure function: Config -> Store
 * @param Config The memory configuration.
 * @return A new memory store.
 */
FORBOCAI_SDK_API FMemoryStore CreateStore(const FMemoryConfig &Config);

/**
 * Initializes the memory store and database.
 * Pure function: Store -> Result
 * @param Store The memory store to initialize.
 * @return A validation result indicating success or failure.
 */
FORBOCAI_SDK_API FValidationResult Initialize(FMemoryStore &Store);

/**
 * Stores a new memory.
 * Pure function: (Store, Text, Type, Importance) -> NewStore
 * @param Store The current memory store.
 * @param Text The memory content.
 * @param Type The memory type.
 * @param Importance The importance score.
 * @return A new memory store with the memory added.
 */
FORBOCAI_SDK_API FMemoryStore Store(const FMemoryStore &Store,
                                    const FString &Text, const FString &Type,
                                    float Importance);

/**
 * Stores a pre-built memory item.
 * Pure function: (Store, Item) -> NewStore
 * @param Store The current memory store.
 * @param Item The memory item to add.
 * @return A new memory store with the item added.
 */
FORBOCAI_SDK_API FMemoryStore Add(const FMemoryStore &Store,
                                  const FMemoryItem &Item);

/**
 * Performs semantic recall using vector search.
 * Pure function: (Store, Query, Limit) -> Results
 * @param Store The memory store to search.
 * @param Query The search query text.
 * @param Limit The maximum number of results to return.
 * @return The ranked memory results.
 */
FORBOCAI_SDK_API TArray<FMemoryItem>
Recall(const FMemoryStore &Store, const FString &Query, int32 Limit = -1);

/**
 * Generates a vector embedding for text.
 * Pure function: (Store, Text) -> Vector
 * @param Store The memory store.
 * @param Text The text to embed.
 * @return The vector embedding.
 */
FORBOCAI_SDK_API TArray<float> GenerateEmbedding(const FMemoryStore &Store,
                                                 const FString &Text);

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