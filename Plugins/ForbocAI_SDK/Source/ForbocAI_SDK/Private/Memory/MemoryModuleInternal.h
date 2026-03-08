#pragma once

#include "Memory/MemoryModule.h"
#include "NativeEngine.h"

namespace MemoryInternal {

// Native Wrappers
namespace Native {
namespace Sqlite {
using Connection = ::Native::Sqlite::DB;
Connection Open(const FString &Path);
void Close(Connection Db);
bool Execute(Connection Db, const FString &Sql);
void Insert(Connection Db, const FMemoryItem &Item);
TArray<FMemoryItem> VssSearch(Connection Db, const TArray<float> &Vector,
                              int32 Limit);
} // namespace Sqlite
} // namespace Native

struct FDatabaseOpenResult {
  bool isLeft = false;
  FString left;
  Native::Sqlite::Connection right = nullptr;

  operator MemoryTypes::MemoryStoreInitializationResult() const {
    return MemoryTypes::MemoryStoreInitializationResult{
        isLeft, left, !isLeft && right != nullptr};
  }
};

using FDatabaseMutationResult = MemoryTypes::MemoryStoreInitializationResult;

// SQLite-VSS Specifics
namespace SQLiteVSS {
FDatabaseOpenResult OpenDatabase(const FString &Path);
void CloseDatabase(void *Handle);
FDatabaseMutationResult CreateTables(void *Handle);
FDatabaseMutationResult InsertMemory(void *Handle, const FMemoryItem &Item);
MemoryTypes::MemoryStoreRecallResult
VectorSearch(void *Handle, const FString &Query, int32 Limit);
MemoryTypes::MemoryStoreEmbeddingResult GenerateEmbedding(void *Handle,
                                                          const FString &Text);
} // namespace SQLiteVSS

// Helpers
FString GetDatabasePath(const FMemoryConfig &Config);
MemoryTypes::MemoryStoreInitializationResult
ValidateConfig(const FMemoryConfig &Config);
FString GetMemoryStatistics(const FMemoryStore &Store);

} // namespace MemoryInternal
