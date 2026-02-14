#include "MemoryModule.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"

TArray<float> MemoryOps::GenerateEmbedding(const FString &Text) {
  // Stub: Return random vector
  TArray<float> Vector;
  Vector.Init(0.0f, 384);
  return Vector;
}

FMemoryStore MemoryOps::Store(const FMemoryStore &CurrentStore,
                              const FString &Text, const FString &Type,
                              float Importance) {
  // Create Item
  FMemoryItem Item;
  Item.Id = FString::Printf(TEXT("mem_%s"), *FGuid::NewGuid().ToString());
  Item.Text = Text;
  Item.Type = Type;
  Item.Importance = Importance;
  Item.Timestamp = FDateTime::Now().ToUnixTimestamp();

  // Return new store with item added
  return CurrentStore.Add(Item);
}

TArray<FMemoryItem> MemoryOps::Recall(const FMemoryStore &Store,
                                      const FString &Query, int32 Limit) {
  // Stub: Return most recent items since we don't have real vector search
  TArray<FMemoryItem> Results = Store.Items;

  // Sort by timestamp descending (newest first)
  Results.Sort([](const FMemoryItem &A, const FMemoryItem &B) {
    return A.Timestamp > B.Timestamp;
  });

  if (Results.Num() > Limit) {
    Results.SetNum(Limit);
  }

  return Results;
}
