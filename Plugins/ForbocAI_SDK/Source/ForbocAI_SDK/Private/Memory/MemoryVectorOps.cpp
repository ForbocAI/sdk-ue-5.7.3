#include "Memory/MemoryModuleInternal.h"

namespace MemoryInternal {

namespace Native {
namespace Sqlite {

TArray<FMemoryItem> VssSearch(Connection Db, const TArray<float> &Vector,
                              int32 Limit) {
  return ::Native::Sqlite::SearchRows(Db, Vector, Limit);
}

} // namespace Sqlite
} // namespace Native

namespace SQLiteVSS {

MemoryTypes::MemoryStoreRecallResult
VectorSearch(void *Handle, const FString &Query, int32 Limit) {
  try {
    if (Handle == nullptr) {
      return MemoryTypes::MemoryStoreRecallResult{
          true, TEXT("Memory database is not open"), TArray<FMemoryItem>()};
    }

    const MemoryTypes::MemoryStoreEmbeddingResult EmbeddingResult =
        GenerateEmbedding(Handle, Query);
    if (EmbeddingResult.isLeft) {
      return MemoryTypes::MemoryStoreRecallResult{true, EmbeddingResult.left,
                                                  TArray<FMemoryItem>()};
    }

    const TArray<FMemoryItem> Results =
        Native::Sqlite::VssSearch(Handle, EmbeddingResult.right, Limit);
    return MemoryTypes::MemoryStoreRecallResult{false, FString(), Results};
  } catch (const std::exception &e) {
    return MemoryTypes::MemoryStoreRecallResult{
        true, FString(e.what()), TArray<FMemoryItem>()};
  }
}

MemoryTypes::MemoryStoreEmbeddingResult GenerateEmbedding(void *Handle,
                                                          const FString &Text) {
  try {
    const TArray<float> Vector = ::Native::Llama::Embed(Handle, Text);
    return MemoryTypes::MemoryStoreEmbeddingResult{false, FString(), Vector};
  } catch (const std::exception &e) {
    return MemoryTypes::MemoryStoreEmbeddingResult{
        true, FString(e.what()), TArray<float>()};
  }
}

} // namespace SQLiteVSS

} // namespace MemoryInternal
