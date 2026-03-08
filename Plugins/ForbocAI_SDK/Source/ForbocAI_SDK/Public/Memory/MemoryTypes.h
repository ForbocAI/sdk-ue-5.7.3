#pragma once

#include "CoreMinimal.h"
#include "MemoryTypes.generated.h"

/**
 * Memory Item — Immutable data.
 */
USTRUCT(BlueprintType)
struct FMemoryItem {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Text;

  UPROPERTY(BlueprintReadOnly)
  TArray<float> Embedding;

  UPROPERTY(BlueprintReadOnly)
  int64 Timestamp;

  UPROPERTY(BlueprintReadOnly)
  FString Type;

  UPROPERTY(BlueprintReadOnly)
  float Importance;

  UPROPERTY(BlueprintReadOnly)
  float Similarity;

  FMemoryItem() : Importance(0.5f), Timestamp(0), Similarity(0.0f) {}
};

/**
 * Memory Configuration — Immutable data.
 */
USTRUCT(BlueprintType)
struct FMemoryConfig {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString DatabasePath;

  UPROPERTY(BlueprintReadOnly)
  int32 MaxMemories;

  UPROPERTY(BlueprintReadOnly)
  int32 VectorDimension;

  UPROPERTY(BlueprintReadOnly)
  bool UseGPU;

  UPROPERTY(BlueprintReadOnly)
  int32 MaxRecallResults;

  FMemoryConfig()
      : DatabasePath(TEXT("ForbocAI_Memory.db")), MaxMemories(10000),
        VectorDimension(384), UseGPU(false), MaxRecallResults(10) {}
};

/**
 * Memory Store — Immutable data.
 */
USTRUCT(BlueprintType)
struct FMemoryStore {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FMemoryConfig Config;

  UPROPERTY(BlueprintReadOnly)
  TArray<FMemoryItem> Items;

  /** Database connection handle. */
  void *DatabaseHandle;

  UPROPERTY(BlueprintReadOnly)
  bool bInitialized;

  FMemoryStore() : DatabaseHandle(nullptr), bInitialized(false) {}
};

/**
 * Memory Recall Request
 */
USTRUCT(BlueprintType)
struct FMemoryRecallRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Query;

  UPROPERTY(BlueprintReadOnly)
  int32 Limit;

  UPROPERTY(BlueprintReadOnly)
  float Threshold;

  FMemoryRecallRequest() : Limit(10), Threshold(0.7f) {}
};

namespace TypeFactory {

inline FMemoryItem MemoryItem(FString Id, FString Text, FString Type,
                              float Importance, int64 Timestamp) {
  FMemoryItem M;
  M.Id = MoveTemp(Id);
  M.Text = MoveTemp(Text);
  M.Type = MoveTemp(Type);
  M.Importance = Importance;
  M.Timestamp = Timestamp;
  return M;
}

inline FMemoryConfig
MemoryConfig(FString DatabasePath = TEXT("ForbocAI_Memory.db"),
             int32 MaxMemories = 10000, int32 VectorDimension = 384,
             bool UseGPU = false, int32 MaxRecallResults = 10) {
  FMemoryConfig C;
  C.DatabasePath = MoveTemp(DatabasePath);
  C.MaxMemories = MaxMemories;
  C.VectorDimension = VectorDimension;
  C.UseGPU = UseGPU;
  C.MaxRecallResults = MaxRecallResults;
  return C;
}

} // namespace TypeFactory
