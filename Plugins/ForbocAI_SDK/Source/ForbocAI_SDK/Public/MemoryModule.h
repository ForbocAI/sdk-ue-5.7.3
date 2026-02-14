#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include <functional>

/**
 * Memory Store - Pure Data.
 * Represents the database of memories.
 */
struct FMemoryStore {
  const TArray<FMemoryItem> Items;
  // In a real implementation, this would hold the vector index or reference

  FMemoryStore() {}
  FMemoryStore(const TArray<FMemoryItem> &InItems) : Items(InItems) {}

  // Functional add
  FMemoryStore Add(const FMemoryItem &Item) const {
    TArray<FMemoryItem> NewItems = Items;
    NewItems.Add(Item);
    return FMemoryStore(NewItems);
  }
};

/**
 * Memory Operations.
 */
namespace MemoryOps {
/**
 * Calculates embedding for text (simulated for now).
 * Pure function: Text -> Vector
 */
FORBOCAI_SDK_API TArray<float> GenerateEmbedding(const FString &Text);

/**
 * Stores a memory.
 * Pure function: Returns new store.
 */
FORBOCAI_SDK_API FMemoryStore Store(const FMemoryStore &CurrentStore,
                                    const FString &Text, const FString &Type,
                                    float Importance);

/**
 * Recalls memories relevant to query.
 * Pure function: Store + Query -> Ranked Items
 */
FORBOCAI_SDK_API TArray<FMemoryItem>
Recall(const FMemoryStore &Store, const FString &Query, int32 Limit = 5);
} // namespace MemoryOps
