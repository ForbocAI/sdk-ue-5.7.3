#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include <functional>

// ==========================================================
// Memory Module — Strict FP
// ==========================================================
// FMemoryStore is a pure data struct with a const member.
// NO constructors, NO member functions.
// All operations are free functions in the MemoryOps namespace.
// ==========================================================

/**
 * Memory Store — Pure immutable data.
 * Items is const: mutation returns a new store (Persistent Data Structure).
 */
struct FMemoryStore {
  /** List of immutable memory items. */
  const TArray<FMemoryItem> Items;
};

/**
 * Memory Operations — stateless free functions.
 */
namespace MemoryOps {

/**
 * Factory: Creates an empty memory store.
 * Pure function: () -> FMemoryStore
 * @return A new empty FMemoryStore.
 */
FORBOCAI_SDK_API FMemoryStore CreateStore();

/**
 * Factory: Creates a memory store from existing items.
 * Pure function: Items -> FMemoryStore
 * @param Items The initial items for the store.
 * @return A new FMemoryStore containing the items.
 */
FORBOCAI_SDK_API FMemoryStore CreateStore(const TArray<FMemoryItem> &Items);

/**
 * Pure: Returns a new store with the item appended.
 * Functional update — original is not modified.
 * @param Store The current memory store.
 * @param Item The item to add.
 * @return A new FMemoryStore including the new item.
 */
FORBOCAI_SDK_API FMemoryStore Add(const FMemoryStore &Store,
                                  const FMemoryItem &Item);

/**
 * Pure: Calculates embedding for text (simulated).
 * Text -> Vector
 * @param Text The text to embed.
 * @return A vector of floating point numbers representing the embedding.
 */
FORBOCAI_SDK_API TArray<float> GenerateEmbedding(const FString &Text);

/**
 * Pure: Stores a memory and returns new store.
 * (Store, Text, Type, Importance) -> Store
 * @param CurrentStore The current memory store.
 * @param Text The content of the new memory.
 * @param Type The type of the new memory.
 * @param Importance The importance of the new memory.
 * @return A new FMemoryStore including the new memory.
 */
FORBOCAI_SDK_API FMemoryStore Store(const FMemoryStore &CurrentStore,
                                    const FString &Text, const FString &Type,
                                    float Importance);

/**
 * Pure: Recalls memories relevant to query.
 * (Store, Query, Limit) -> Ranked Items
 * @param Store The memory store to search.
 * @param Query The search query.
 * @param Limit Maximum number of results to return.
 * @return A list of relevant memory items.
 */
FORBOCAI_SDK_API TArray<FMemoryItem>
Recall(const FMemoryStore &Store, const FString &Query, int32 Limit = 5);

} // namespace MemoryOps
