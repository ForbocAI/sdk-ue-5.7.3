#include "NativeEngine.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

#if WITH_FORBOC_NATIVE
#include "llama.h"
// #include "sqlite_vss.h" // Native sqlite-vss integration is not wired yet.
#endif

namespace {

constexpr int32 FallbackEmbeddingDimensions = 384;

struct FMockLlamaContext {
  FString ModelPath;
  int32 Dimensions = FallbackEmbeddingDimensions;
};

struct FMockSqliteHandle {
  FString Path;
};

FCriticalSection GMockSqliteMutex;
TMap<FString, TMap<FString, FMemoryItem>> GMockSqliteRowsByPath;

uint64 StableHashString(const FString &Value) {
  uint64 Hash = 1469598103934665603ull;
  for (int32 Index = 0; Index < Value.Len(); ++Index) {
    const TCHAR LowerCh = FChar::ToLower(Value[Index]);
    Hash ^= static_cast<uint64>(LowerCh);
    Hash *= 1099511628211ull;
  }
  return Hash;
}

TArray<FString> TokenizeLowercase(const FString &Text) {
  const FString LowerText = Text.ToLower();
  TArray<FString> Tokens;
  FString Current;

  for (int32 Index = 0; Index < LowerText.Len(); ++Index) {
    const TCHAR Ch = LowerText[Index];
    if (FChar::IsAlnum(Ch)) {
      Current.AppendChar(Ch);
      continue;
    }

    if (!Current.IsEmpty()) {
      Tokens.Add(Current);
      Current.Empty();
    }
  }

  if (!Current.IsEmpty()) {
    Tokens.Add(Current);
  }

  if (Tokens.Num() == 0 && !LowerText.IsEmpty()) {
    Tokens.Add(LowerText);
  }

  return Tokens;
}

void NormalizeVector(TArray<float> &Vector) {
  float MagnitudeSquared = 0.0f;
  for (float Value : Vector) {
    MagnitudeSquared += Value * Value;
  }

  if (MagnitudeSquared <= KINDA_SMALL_NUMBER) {
    return;
  }

  const float InverseMagnitude = 1.0f / FMath::Sqrt(MagnitudeSquared);
  for (float &Value : Vector) {
    Value *= InverseMagnitude;
  }
}

TArray<float> BuildDeterministicEmbedding(const FString &Text,
                                          int32 Dimensions =
                                              FallbackEmbeddingDimensions) {
  TArray<float> Vector;
  Vector.Init(0.0f, Dimensions);

  if (Text.IsEmpty() || Dimensions <= 0) {
    return Vector;
  }

  const TArray<FString> Tokens = TokenizeLowercase(Text);
  for (int32 TokenIndex = 0; TokenIndex < Tokens.Num(); ++TokenIndex) {
    const FString &Token = Tokens[TokenIndex];
    const uint64 TokenHash = StableHashString(Token);
    const int32 SlotA =
        static_cast<int32>(TokenHash % static_cast<uint64>(Dimensions));
    const int32 SlotB = static_cast<int32>(
        ((TokenHash >> 32) ^ TokenHash) % static_cast<uint64>(Dimensions));
    const float Weight = 1.0f + (static_cast<float>(TokenIndex % 5) * 0.1f);

    Vector[SlotA] += Weight;
    if (SlotB != SlotA) {
      Vector[SlotB] += Weight * 0.5f;
    }
  }

  const int32 WholeTextSlot =
      static_cast<int32>(StableHashString(Text) % static_cast<uint64>(Dimensions));
  Vector[WholeTextSlot] += 0.25f;

  NormalizeVector(Vector);
  return Vector;
}

float CosineSimilarity(const TArray<float> &A, const TArray<float> &B) {
  const int32 Count = FMath::Min(A.Num(), B.Num());
  if (Count <= 0) {
    return 0.0f;
  }

  float Dot = 0.0f;
  float NormA = 0.0f;
  float NormB = 0.0f;

  for (int32 Index = 0; Index < Count; ++Index) {
    Dot += A[Index] * B[Index];
    NormA += A[Index] * A[Index];
    NormB += B[Index] * B[Index];
  }

  if (NormA <= KINDA_SMALL_NUMBER || NormB <= KINDA_SMALL_NUMBER) {
    return 0.0f;
  }

  return Dot / FMath::Sqrt(NormA * NormB);
}

FString MakeStableMemoryId(const FMemoryItem &Item) {
  const FString Seed = FString::Printf(TEXT("%s|%s|%lld"), *Item.Type, *Item.Text,
                                       static_cast<long long>(Item.Timestamp));
  return FString::Printf(TEXT("mem_%016llx"),
                         static_cast<unsigned long long>(StableHashString(Seed)));
}

FMockSqliteHandle *AsMockSqliteHandle(Native::Sqlite::DB Database) {
  return reinterpret_cast<FMockSqliteHandle *>(Database);
}

FMemoryItem PrepareStoredItem(const FMemoryItem &Item) {
  FMemoryItem StoredItem = Item;

  if (StoredItem.Id.IsEmpty()) {
    StoredItem.Id = MakeStableMemoryId(StoredItem);
  }

  if (StoredItem.Embedding.IsEmpty()) {
    StoredItem.Embedding = BuildDeterministicEmbedding(StoredItem.Text);
  }

  StoredItem.Similarity = 0.0f;
  return StoredItem;
}

} // namespace

namespace Native {
namespace Llama {

Context LoadModel(const FString &Path) {
#if WITH_FORBOC_NATIVE
  auto Utf8Path = StringCast<UTF8CHAR>(*Path);
  return reinterpret_cast<Context>(llama::load_model(Utf8Path.Get()));
#else
  if (Path.IsEmpty()) {
    return nullptr;
  }

  if (FPaths::FileExists(Path) || Path.Contains(TEXT("test_model.bin"))) {
    return reinterpret_cast<Context>(new FMockLlamaContext{Path});
  }

  return nullptr;
#endif
}

void FreeModel(Context Ctx) {
  if (!Ctx) {
    return;
  }

#if WITH_FORBOC_NATIVE
  llama::free_model(reinterpret_cast<llama::context *>(Ctx));
#else
  delete reinterpret_cast<FMockLlamaContext *>(Ctx);
#endif
}

TArray<float> Embed(Context Ctx, const FString &Text) {
  static_cast<void>(Ctx);
  return BuildDeterministicEmbedding(Text, FallbackEmbeddingDimensions);
}

FString Infer(Context Ctx, const FString &Prompt, int32 MaxTokens) {
  if (!Ctx) {
    return TEXT("Error: Model not loaded");
  }

#if WITH_FORBOC_NATIVE
  auto Utf8Prompt = StringCast<UTF8CHAR>(*Prompt);
  auto Result = llama::infer(reinterpret_cast<llama::context *>(Ctx),
                             Utf8Prompt.Get(), MaxTokens);
  return FString(UTF8_TO_TCHAR(Result));
#else
  FPlatformProcess::Sleep(0.1f);
  return FString::Printf(TEXT("Simulated local response for: %s"),
                         *Prompt.Left(30));
#endif
}

} // namespace Llama

namespace Sqlite {

DB Open(const FString &Path) {
  const FString NormalizedPath =
      Path.IsEmpty() ? TEXT(":memory:") : FPaths::ConvertRelativePathToFull(Path);

  {
    FScopeLock Lock(&GMockSqliteMutex);
    GMockSqliteRowsByPath.FindOrAdd(NormalizedPath);
  }

  return reinterpret_cast<DB>(new FMockSqliteHandle{NormalizedPath});
}

void Close(DB Database) {
  if (!Database) {
    return;
  }

  delete AsMockSqliteHandle(Database);
}

TArray<FMemoryItem> SearchRows(DB Database, const TArray<float> &Vector,
                               int32 TopK) {
  TArray<FMemoryItem> Results;
  const FMockSqliteHandle *Handle = AsMockSqliteHandle(Database);
  if (!Handle) {
    return Results;
  }

  {
    FScopeLock Lock(&GMockSqliteMutex);
    if (const TMap<FString, FMemoryItem> *Rows =
            GMockSqliteRowsByPath.Find(Handle->Path)) {
      Results.Reserve(Rows->Num());
      for (const TPair<FString, FMemoryItem> &Entry : *Rows) {
        FMemoryItem Item = Entry.Value;
        if (Item.Embedding.IsEmpty()) {
          Item.Embedding = BuildDeterministicEmbedding(Item.Text);
        }
        Item.Similarity = CosineSimilarity(Vector, Item.Embedding);
        Results.Add(MoveTemp(Item));
      }
    }
  }

  Results.Sort([](const FMemoryItem &A, const FMemoryItem &B) {
    if (!FMath::IsNearlyEqual(A.Similarity, B.Similarity)) {
      return A.Similarity > B.Similarity;
    }
    if (!FMath::IsNearlyEqual(A.Importance, B.Importance)) {
      return A.Importance > B.Importance;
    }
    return A.Id < B.Id;
  });

  if (TopK > 0 && Results.Num() > TopK) {
    Results.SetNum(TopK, EAllowShrinking::No);
  }

  return Results;
}

bool Upsert(DB Database, const FMemoryItem &Item) {
  FMockSqliteHandle *Handle = AsMockSqliteHandle(Database);
  if (!Handle) {
    return false;
  }

  FMemoryItem StoredItem = PrepareStoredItem(Item);

  FScopeLock Lock(&GMockSqliteMutex);
  GMockSqliteRowsByPath.FindOrAdd(Handle->Path).FindOrAdd(StoredItem.Id) =
      MoveTemp(StoredItem);
  return true;
}

TArray<FMemoryItem> Search(DB Database, const TArray<float> &Vector,
                           int32 TopK) {
  return SearchRows(Database, Vector, TopK);
}

bool Upsert(DB Database, const FMemoryItem &Item,
            const TArray<float> &Vector) {
  FMemoryItem StoredItem = Item;
  StoredItem.Embedding = Vector;
  return Upsert(Database, StoredItem);
}

} // namespace Sqlite
} // namespace Native
