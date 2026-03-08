#include "Memory/MemoryModuleInternal.h"

namespace MemoryInternal {

namespace Native {
namespace Sqlite {

TArray<FMemoryItem> VssSearch(Connection Db, const TArray<float> &Vector,
                              int32 Limit) {
#if WITH_FORBOC_NATIVE
  // sqlite_vss::search(...)
  return {};
#else
  return {}; // Mock results
#endif
}

} // namespace Sqlite
} // namespace Native

namespace SQLiteVSS {

MemoryTypes::MemoryStoreRecallResult
VectorSearch(void *Handle, const FString &Query, int32 Limit) {
  try {
    TArray<FMemoryItem> Results;
    TArray<float> DummyVec;
    DummyVec.Init(0.0f, 384);
    Results = Native::Sqlite::VssSearch(Handle, DummyVec, Limit);
    return MemoryTypes::make_right(FString(), Results);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

MemoryTypes::MemoryStoreEmbeddingResult GenerateEmbedding(void *Handle,
                                                          const FString &Text) {
  try {
    TArray<float> Vector;
    Vector.Init(0.0f, 384);
    for (int i = 0; i < 384; i++) {
      Vector[i] = FMath::FRand();
    }
    return MemoryTypes::make_right(FString(), Vector);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

} // namespace SQLiteVSS

} // namespace MemoryInternal
