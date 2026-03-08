#pragma once

#include "CoreMinimal.h"
#include "Types.h"

namespace MemoryInternal {

// Native Wrappers
namespace Native {
namespace Sqlite {
typedef void *Connection;
Connection Open(const FString &Path);
void Close(Connection Db);
bool Execute(Connection Db, const FString &Sql);
void Insert(Connection Db, const FMemoryItem &Item);
TArray<FMemoryItem> VssSearch(Connection Db, const TArray<float> &Vector,
                              int32 Limit);
} // namespace Sqlite
} // namespace Native

// SQLite-VSS Specifics
namespace SQLiteVSS {
MemoryTypes::MemoryStoreCreationResult OpenDatabase(const FString &Path);
void CloseDatabase(void *Handle);
MemoryTypes::MemoryStoreInitializationResult CreateTables(void *Handle);
MemoryTypes::MemoryStoreAddResult InsertMemory(void *Handle,
                                               const FMemoryItem &Item);
MemoryTypes::MemoryStoreRecallResult
VectorSearch(void *Handle, const FString &Query, int32 Limit);
MemoryTypes::MemoryStoreEmbeddingResult GenerateEmbedding(void *Handle,
                                                          const FString &Text);
} // namespace SQLiteVSS

// Helpers
FString GetDatabasePath(const FMemoryConfig &Config);
MemoryTypes::MemoryStoreCreationResult
ValidateConfig(const FMemoryConfig &Config);
FString GetMemoryStatistics(const FMemoryStore &Store);

} // namespace MemoryInternal
