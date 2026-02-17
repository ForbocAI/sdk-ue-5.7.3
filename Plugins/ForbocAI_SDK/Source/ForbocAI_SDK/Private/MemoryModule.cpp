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
  // Use local sqlite-vss for embedding generation
  // This is a placeholder implementation - would need to integrate with sqlite-vss
  TArray<float> Vector;
  Vector.Init(0.0f, 384);
  
  // TODO: Implement real embedding generation using sqlite-vss
  // For now, return a random vector as placeholder
  for (int i = 0; i < 384; i++) {
    Vector[i] = FMath::FRand();
  }
  
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
  // Use local sqlite-vss for vector search
  // This is a placeholder implementation - would need to integrate with sqlite-vss
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
