#include "MemoryModule.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"

// ==========================================
// MEMORY OPERATIONS — Pure free functions
// ==========================================

FMemoryStore MemoryOps::CreateStore() { return FMemoryStore{TArray<FMemoryItem>()}; }

FMemoryStore MemoryOps::CreateStore(const TArray<FMemoryItem> &Items) {
  return FMemoryStore{Items};
}

FMemoryStore MemoryOps::Add(const FMemoryStore &CurrentStore,
                            const FMemoryItem &Item) {
  TArray<FMemoryItem> NewItems = CurrentStore.Items;
  NewItems.Add(Item);
  return FMemoryStore{MoveTemp(NewItems)};
}

TArray<float> MemoryOps::GenerateEmbedding(const FString &Text) {
  // Stub: Return zero vector (placeholder for real embedding model)
  TArray<float> Vector;
  Vector.Init(0.0f, 384);
  return Vector;
}

FMemoryStore MemoryOps::Store(const FMemoryStore &CurrentStore,
                              const FString &Text, const FString &Type,
                              float Importance) {
  const FMemoryItem Item = TypeFactory::MemoryItem(
      FString::Printf(TEXT("mem_%s"), *FGuid::NewGuid().ToString()), Text, Type,
      Importance, FDateTime::Now().ToUnixTimestamp());

  return MemoryOps::Add(CurrentStore, Item);
}

TArray<FMemoryItem> MemoryOps::Recall(const FMemoryStore &InStore,
                                      const FString &Query, int32 Limit) {
  // Stub: Return most recent items (placeholder for vector search)
  TArray<FMemoryItem> Results = InStore.Items;

  // Sort by timestamp descending (newest first) — local copy only
  Results.Sort([](const FMemoryItem &A, const FMemoryItem &B) {
    return A.Timestamp > B.Timestamp;
  });

  if (Results.Num() > Limit) {
    Results.SetNum(Limit);
  }

  return Results;
}
