#include "Memory/MemoryModuleInternal.h"
#include "Misc/Paths.h"

namespace MemoryInternal {

namespace Native {
namespace Sqlite {

Connection Open(const FString &Path) { return ::Native::Sqlite::Open(Path); }

void Close(Connection Db) { ::Native::Sqlite::Close(Db); }

bool Execute(Connection Db, const FString &Sql) {
  return Db != nullptr && !Sql.IsEmpty();
}

void Insert(Connection Db, const FMemoryItem &Item) {
  ::Native::Sqlite::Upsert(Db, Item);
}

} // namespace Sqlite
} // namespace Native

namespace SQLiteVSS {

FDatabaseOpenResult OpenDatabase(const FString &Path) {
  try {
    void *Handle = Native::Sqlite::Open(Path);
    if (Handle == nullptr) {
      return {true, TEXT("Failed to open memory database"), nullptr};
    }

    return {false, FString(), Handle};
  } catch (const std::exception &e) {
    return {true, FString(e.what()), nullptr};
  }
}

void CloseDatabase(void *Handle) {
  if (Handle != nullptr) {
    Native::Sqlite::Close(Handle);
  }
}

FDatabaseMutationResult CreateTables(void *Handle) {
  try {
    const FString Sql = TEXT(
        "CREATE TABLE IF NOT EXISTS items("
        "id TEXT PRIMARY KEY,"
        "text TEXT,"
        "type TEXT,"
        "importance REAL,"
        "timestamp INTEGER,"
        "similarity REAL,"
        "embedding BLOB);");

    const bool bSuccess = Native::Sqlite::Execute(Handle, Sql);
    return FDatabaseMutationResult{
        !bSuccess,
        bSuccess ? FString()
                 : FString(TEXT("Failed to create memory tables")),
        bSuccess};
  } catch (const std::exception &e) {
    return FDatabaseMutationResult{true, FString(e.what()), false};
  }
}

FDatabaseMutationResult InsertMemory(void *Handle, const FMemoryItem &Item) {
  try {
    if (Handle == nullptr) {
      return FDatabaseMutationResult{
          true, TEXT("Memory database is not open"), false};
    }

    Native::Sqlite::Insert(Handle, Item);
    return FDatabaseMutationResult{false, FString(), true};
  } catch (const std::exception &e) {
    return FDatabaseMutationResult{true, FString(e.what()), false};
  }
}

} // namespace SQLiteVSS

FString GetDatabasePath(const FMemoryConfig &Config) {
  if (Config.DatabasePath.IsEmpty()) {
    return FString();
  }

  // G6: Reject paths with directory traversal sequences
  if (Config.DatabasePath.Contains(TEXT(".."))) {
    UE_LOG(LogTemp, Error,
           TEXT("ForbocAI: Rejected database path containing '..': %s"),
           *Config.DatabasePath);
    return FString();
  }

  if (FPaths::IsRelative(Config.DatabasePath)) {
    return FPaths::Combine(FPaths::ProjectDir(), TEXT("local_infrastructure"),
                           Config.DatabasePath);
  }

  return Config.DatabasePath;
}

MemoryTypes::MemoryStoreInitializationResult
ValidateConfig(const FMemoryConfig &Config) {
  try {
    if (Config.DatabasePath.IsEmpty()) {
      return MemoryTypes::MemoryStoreInitializationResult{
          true, TEXT("Database path cannot be empty"), false};
    }
    if (Config.MaxMemories <= 0) {
      return MemoryTypes::MemoryStoreInitializationResult{
          true, TEXT("Max memories must be greater than 0"), false};
    }
    if (Config.VectorDimension != 384) {
      return MemoryTypes::MemoryStoreInitializationResult{
          true, TEXT("Vector dimension must be 384"), false};
    }
    if (Config.MaxRecallResults <= 0) {
      return MemoryTypes::MemoryStoreInitializationResult{
          true, TEXT("Max recall results must be greater than 0"), false};
    }

    return MemoryTypes::MemoryStoreInitializationResult{
        false, FString(), true};
  } catch (const std::exception &e) {
    return MemoryTypes::MemoryStoreInitializationResult{
        true, FString(e.what()), false};
  }
}

FString GetMemoryStatistics(const FMemoryStore &Store) {
  FString Statistics = FString::Printf(TEXT("Memory Statistics\n"));
  Statistics +=
      FString::Printf(TEXT("  Total Memories: %d\n"), Store.Items.Num());
  Statistics += FString::Printf(TEXT("  Database: %s\n"),
                                Store.bInitialized ? TEXT("Connected")
                                                   : TEXT("Disconnected"));
  Statistics +=
      FString::Printf(TEXT("  Max Memories: %d\n"), Store.Config.MaxMemories);
  Statistics += FString::Printf(TEXT("  Vector Dimension: %d\n"),
                                Store.Config.VectorDimension);
  Statistics += FString::Printf(TEXT("  Use GPU: %s\n"),
                                Store.Config.UseGPU ? TEXT("Yes") : TEXT("No"));
  Statistics += FString::Printf(TEXT("  Max Recall Results: %d\n"),
                                Store.Config.MaxRecallResults);

  return Statistics;
}

} // namespace MemoryInternal
