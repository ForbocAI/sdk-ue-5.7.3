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
 * Items is const: mutation returns a new store.
 */
struct FMemoryStore {
  const TArray<FMemoryItem> Items;
};

/**
 * Memory Operations — stateless free functions.
 */
namespace MemoryOps {

/**
 * Factory: Creates an empty memory store.
 * Pure function: () -> FMemoryStore
 */
FORBOCAI_SDK_API FMemoryStore CreateStore();

/**
 * Factory: Creates a memory store from existing items.
 * Pure function: Items -> FMemoryStore
 */
FORBOCAI_SDK_API FMemoryStore CreateStore(const TArray<FMemoryItem> &Items);

/**
 * Pure: Returns a new store with the item appended.
 * Functional update — original is not modified.
 */
FORBOCAI_SDK_API FMemoryStore Add(const FMemoryStore &Store,
                                  const FMemoryItem &Item);

/**
 * Pure: Calculates embedding for text (simulated).
 * Text -> Vector
 */
FORBOCAI_SDK_API TArray<float> GenerateEmbedding(const FString &Text);

/**
 * Pure: Stores a memory and returns new store.
 * (Store, Text, Type, Importance) -> Store
 */
FORBOCAI_SDK_API FMemoryStore Store(const FMemoryStore &CurrentStore,
                                    const FString &Text, const FString &Type,
                                    float Importance);

/**
 * Pure: Recalls memories relevant to query.
 * (Store, Query, Limit) -> Ranked Items
 */
FORBOCAI_SDK_API TArray<FMemoryItem>
Recall(const FMemoryStore &Store, const FString &Query, int32 Limit = 5);

} // namespace MemoryOps
