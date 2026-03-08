#include "Memory/MemoryModuleInternal.h"
#include "Misc/Paths.h"

namespace MemoryInternal {

namespace Native {
namespace Sqlite {

Connection Open(const FString &Path) {
#if WITH_FORBOC_NATIVE
  // Real Sqlite Open logic
  return nullptr;
#else
  return reinterpret_cast<Connection>(new int(101)); // Dummy handle
#endif
}

void Close(Connection Db) {
#if WITH_FORBOC_NATIVE
  // Real Sqlite Close logic
#else
  if (Db)
    delete reinterpret_cast<int *>(Db);
#endif
}

bool Execute(Connection Db, const FString &Sql) {
#if WITH_FORBOC_NATIVE
  return true;
#else
  return Db != nullptr;
#endif
}

void Insert(Connection Db, const FMemoryItem &Item) {
  // Insert logic
}

} // namespace Sqlite
} // namespace Native

namespace SQLiteVSS {

MemoryTypes::MemoryStoreCreationResult OpenDatabase(const FString &Path) {
  try {
    void *handle = Native::Sqlite::Open(Path);
    return MemoryTypes::make_right(FString(), handle);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

void CloseDatabase(void *Handle) {
  if (Handle) {
    Native::Sqlite::Close(Handle);
  }
}

MemoryTypes::MemoryStoreInitializationResult CreateTables(void *Handle) {
  try {
    FString Sql = TEXT(
        "CREATE TABLE IF NOT EXISTS items(id TEXT PRIMARY KEY, content TEXT, "
        "type TEXT, importance REAL, timestamp INTEGER, embedding BLOB);");
    Native::Sqlite::Execute(Handle, Sql);
    return MemoryTypes::make_right(FString(), true);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

MemoryTypes::MemoryStoreAddResult InsertMemory(void *Handle,
                                               const FMemoryItem &Item) {
  try {
    Native::Sqlite::Insert(Handle, Item);
    return MemoryTypes::make_right(FString(), true);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

} // namespace SQLiteVSS

FString GetDatabasePath(const FMemoryConfig &Config) {
  return FPaths::ProjectContentDir() + TEXT("ForbocAI/") + Config.DatabasePath;
}

MemoryTypes::MemoryStoreCreationResult
ValidateConfig(const FMemoryConfig &Config) {
  try {
    if (Config.DatabasePath.IsEmpty()) {
      return MemoryTypes::make_left(
          FString(TEXT("Database path cannot be empty")));
    }
    if (Config.MaxMemories <= 0) {
      return MemoryTypes::make_left(
          FString(TEXT("Max memories must be greater than 0")));
    }
    if (Config.VectorDimension != 384) {
      return MemoryTypes::make_left(
          FString(TEXT("Vector dimension must be 384")));
    }
    if (Config.MaxRecallResults <= 0) {
      return MemoryTypes::make_left(
          FString(TEXT("Max recall results must be greater than 0")));
    }
    return MemoryTypes::make_right(FString(), Config);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
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
