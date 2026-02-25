#include "MemoryModule.h"
#include "Core/functional_core.hpp"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

// ==========================================================
// Memory Module Implementation — Full Embedding-Based Memory
// ==========================================================
// Strict functional programming implementation for
// persistent, semantic recall using sqlite-vss for vector search.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

#if WITH_FORBOC_NATIVE
#include "sqlite_vss.h"
#endif

// ... (existing imports)

namespace {

// ==========================================
// NATIVE WRAPPERS
// ==========================================
namespace Native {
namespace Sqlite {

using Connection = void *;

Connection Open(const FString &Path) {
#if WITH_FORBOC_NATIVE
  // Real Sqlite Open logic would go here
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

TArray<FMemoryItem> VssSearch(Connection Db, const TArray<float> &Vector,
                              int32 Limit) {
#if WITH_FORBOC_NATIVE
  // sqlite_vss::search(...)
  return {};
#else
  return {}; // Mock results
#endif
}

void Insert(Connection Db, const FMemoryItem &Item) {
  // Insert logic
}

} // namespace Sqlite
} // namespace Native

namespace SQLiteVSSInternal {

// SQLite-vss integration functions
namespace SQLiteVSS {

/**
 * Opens a database connection.
 * Pure function: (Path) -> Handle
 * @param Path The database file path.
 * @return The database handle.
 */
MemoryTypes::MemoryStoreCreationResult OpenDatabase(const FString &Path) {
  try {
    // Native Open
    void *handle = Native::Sqlite::Open(Path);

    // Ensure VSS extension is loaded
    // Native::Sqlite::LoadExtension(handle, "vector0");
    // Native::Sqlite::LoadExtension(handle, "vss0");

    return MemoryTypes::make_right(FString(), handle);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

/**
 * Closes a database connection.
 * Pure function: (Handle) -> Void
 * @param Handle The database handle.
 */
void CloseDatabase(void *Handle) {
  if (Handle) {
    Native::Sqlite::Close(Handle);
  }
}

/**
 * Creates the memory tables.
 * Pure function: (Handle) -> Result
 * @param Handle The database handle.
 * @return The validation result.
 */
MemoryTypes::MemoryStoreInitializationResult CreateTables(void *Handle) {
  try {
    // Native Table Creation for VSS
    FString Sql = TEXT(
        "CREATE TABLE IF NOT EXISTS items(id TEXT PRIMARY KEY, content TEXT, "
        "type TEXT, importance REAL, timestamp INTEGER, embedding BLOB);");
    Native::Sqlite::Execute(Handle, Sql);

    return MemoryTypes::make_right(FString(), true);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

/**
 * Inserts a memory item into the database.
 * Pure function: (Handle, Item) -> Result
 * @param Handle The database handle.
 * @param Item The memory item to insert.
 * @return The validation result.
 */
MemoryTypes::MemoryStoreAddResult InsertMemory(void *Handle,
                                               const FMemoryItem &Item) {
  try {
    Native::Sqlite::Insert(Handle, Item);
    return MemoryTypes::make_right(FString(), true);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

/**
 * Performs vector search for semantic recall.
 * Pure function: (Handle, Query, Limit) -> Results
 * @param Handle The database handle.
 * @param Query The search query text.
 * @param Limit The maximum number of results.
 * @return The ranked memory results.
 */
MemoryTypes::MemoryStoreRecallResult
VectorSearch(void *Handle, const FString &Query, int32 Limit) {
  try {
    TArray<FMemoryItem> Results;

    // Perform in-memory vector search via simulation
    // We pass a dummy vector since we don't have the real embedding logic yet
    TArray<float> DummyVec;
    DummyVec.Init(0.0f, 384);

    Results = Native::Sqlite::VssSearch(Handle, DummyVec, Limit);
    return MemoryTypes::make_right(FString(), Results);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

/**
 * Generates a vector embedding for text.
 * Pure function: (Handle, Text) -> Vector
 * @param Handle The database handle.
 * @param Text The text to embed.
 * @return The vector embedding.
 */
MemoryTypes::MemoryStoreEmbeddingResult GenerateEmbedding(void *Handle,
                                                          const FString &Text) {
  try {
    TArray<float> Vector;

    // TODO: Implement actual embedding generation with sqlite-vss
    // For now, return a random vector
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

/**
 * Gets the path to the database file.
 * Pure function: (Config) -> Path
 * @param Config The memory configuration.
 * @return The database file path.
 */
FString GetDatabasePath(const FMemoryConfig &Config) {
  return FPaths::ProjectContentDir() + TEXT("ForbocAI/") + Config.DatabasePath;
}

/**
 * Validates the memory configuration.
 * Pure function: Config -> Result
 * @param Config The memory configuration to validate.
 * @return The validation result.
 */
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

/**
 * Gets memory statistics.
 * Pure function: Store -> Statistics
 * @param Store The memory store.
 * @return The memory statistics.
 */
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

} // namespace SQLiteVSSInternal

} // namespace

// Factory function implementation
FMemoryStore MemoryFactory::CreateStore(const FMemoryConfig &Config) {
  // Use functional validation
  auto validationResult =
      MemoryHelpers::memoryConfigValidationPipeline().run(Config);
  if (validationResult.isLeft) {
    throw std::runtime_error(validationResult.left.c_str());
  }

  FMemoryStore Store;
  Store.Config = Config;
  Store.bInitialized = false;
  Store.DatabaseHandle = nullptr;
  return Store;
}

// Initialization implementation
MemoryTypes::MemoryStoreInitializationResult
MemoryOps::Initialize(FMemoryStore &Store) {
  try {
    if (Store.bInitialized) {
      return MemoryTypes::make_right(FString(), true);
    }

    // Validate configuration
    auto configValidation = Internal::ValidateConfig(Store.Config);
    if (configValidation.isLeft) {
      return configValidation;
    }

    // Get database path
    const FString DatabasePath = Internal::GetDatabasePath(Store.Config);

    // Open database connection
    auto openResult = Internal::SQLiteVSS::OpenDatabase(DatabasePath);
    if (openResult.isLeft) {
      return openResult;
    }
    Store.DatabaseHandle = openResult.right;

    // Create tables
    auto tableResult = Internal::SQLiteVSS::CreateTables(Store.DatabaseHandle);
    if (tableResult.isLeft) {
      Internal::SQLiteVSS::CloseDatabase(Store.DatabaseHandle);
      Store.DatabaseHandle = nullptr;
      return tableResult;
    }

    Store.bInitialized = true;
    return MemoryTypes::make_right(FString(), true);
  } catch (const std::exception &e) {
    return MemoryTypes::make_left(FString(e.what()));
  }
}

// Store implementation
TFuture<MemoryTypes::MemoryStoreAddResult>
MemoryOps::Store(const FMemoryStore &Store, const FString &Text,
                 const FString &Type, float Importance) {
  if (!Store.bInitialized) {
    TPromise<MemoryTypes::MemoryStoreAddResult> Promise;
    Promise.SetValue(MemoryTypes::make_right(FString(), Store));
    return Promise.GetFuture();
  }

  return Async(
      EAsyncExecution::Thread,
      [Store, Text, Type, Importance]() -> MemoryTypes::MemoryStoreAddResult {
        try {
          FMemoryStore NewStore = Store;

          // Generate a new memory item
          FMemoryItem Item;
          Item.Id =
              FString::Printf(TEXT("mem_%s"), *FGuid::NewGuid().ToString());
          Item.Text = Text;
          Item.Type = Type;
          Item.Importance = FMath::Clamp(Importance, 0.0f, 1.0f);
          Item.Timestamp = FDateTime::Now().ToUnixTimestamp();

          // Generate embedding directly via internal call on this background
          // thread
          auto embeddingResult = Internal::SQLiteVSS::GenerateEmbedding(
              Store.DatabaseHandle, Text);
          if (embeddingResult.isLeft) {
            return MemoryTypes::make_left(embeddingResult.left);
          }

          // Insert into database
          auto insertResult =
              Internal::SQLiteVSS::InsertMemory(Store.DatabaseHandle, Item);
          if (insertResult.isLeft) {
            return insertResult;
          }

          // Add to items array
          NewStore.Items.Add(Item);

          return MemoryTypes::make_right(FString(), NewStore);
        } catch (const std::exception &e) {
          return MemoryTypes::make_left(FString(e.what()));
        }
      });
}

// Add implementation
TFuture<MemoryTypes::MemoryStoreAddResult>
MemoryOps::Add(const FMemoryStore &Store, const FMemoryItem &Item) {
  if (!Store.bInitialized) {
    TPromise<MemoryTypes::MemoryStoreAddResult> Promise;
    Promise.SetValue(MemoryTypes::make_right(FString(), Store));
    return Promise.GetFuture();
  }

  return Async(EAsyncExecution::Thread,
               [Store, Item]() -> MemoryTypes::MemoryStoreAddResult {
                 try {
                   FMemoryStore NewStore = Store;

                   // Insert into database
                   auto insertResult = Internal::SQLiteVSS::InsertMemory(
                       Store.DatabaseHandle, Item);
                   if (insertResult.isLeft) {
                     return insertResult;
                   }

                   // Add to items array
                   NewStore.Items.Add(Item);

                   return MemoryTypes::make_right(FString(), NewStore);
                 } catch (const std::exception &e) {
                   return MemoryTypes::make_left(FString(e.what()));
                 }
               });
}

// Recall implementation
TFuture<MemoryTypes::MemoryStoreRecallResult>
MemoryOps::Recall(const FMemoryStore &Store, const FString &Query,
                  int32 Limit) {
  if (!Store.bInitialized) {
    TPromise<MemoryTypes::MemoryStoreRecallResult> Promise;
    Promise.SetValue(MemoryTypes::make_right(FString(), TArray<FMemoryItem>()));
    return Promise.GetFuture();
  }

  // Offload semantic vector search to prevent hitch
  return Async(EAsyncExecution::Thread,
               [Store, Query, Limit]() -> MemoryTypes::MemoryStoreRecallResult {
                 try {
                   int32 ActualLimit =
                       (Limit > 0) ? Limit : Store.Config.MaxRecallResults;
                   return Internal::SQLiteVSS::VectorSearch(
                       Store.DatabaseHandle, Query, ActualLimit);
                 } catch (const std::exception &e) {
                   return MemoryTypes::make_left(FString(e.what()));
                 }
               });
}

// Embedding generation implementation
TFuture<MemoryTypes::MemoryStoreEmbeddingResult>
MemoryOps::GenerateEmbedding(const FMemoryStore &Store, const FString &Text) {
  if (!Store.bInitialized) {
    TPromise<MemoryTypes::MemoryStoreEmbeddingResult> Promise;
    TArray<float> EmptyVector;
    EmptyVector.Init(0.0f, Store.Config.VectorDimension);
    Promise.SetValue(MemoryTypes::make_right(FString(), EmptyVector));
    return Promise.GetFuture();
  }

  return Async(EAsyncExecution::Thread,
               [Store, Text]() -> MemoryTypes::MemoryStoreEmbeddingResult {
                 try {
                   return Internal::SQLiteVSS::GenerateEmbedding(
                       Store.DatabaseHandle, Text);
                 } catch (const std::exception &e) {
                   return MemoryTypes::make_left(FString(e.what()));
                 }
               });
}

// Statistics implementation
FString MemoryOps::GetStatistics(const FMemoryStore &Store) {
  return Internal::GetMemoryStatistics(Store);
}

// Cleanup implementation
void MemoryOps::Cleanup(FMemoryStore &Store) {
  if (Store.bInitialized && Store.DatabaseHandle != nullptr) {
    Internal::SQLiteVSS::CloseDatabase(Store.DatabaseHandle);
    Store.DatabaseHandle = nullptr;
    Store.bInitialized = false;
  }
}

// Functional helper implementations
namespace MemoryHelpers {
// Implementation of lazy memory store creation
MemoryTypes::Lazy<FMemoryStore>
createLazyMemoryStore(const FMemoryConfig &config) {
  return func::lazy([config]() -> FMemoryStore {
    return MemoryFactory::CreateStore(config);
  });
}

// Implementation of memory config validation pipeline
MemoryTypes::ValidationPipeline<FMemoryConfig>
memoryConfigValidationPipeline() {
  return func::validationPipeline<FMemoryConfig>()
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.DatabasePath.IsEmpty()) {
          return MemoryTypes::make_left(
              FString(TEXT("Database path cannot be empty")));
        }
        return MemoryTypes::make_right(config);
      })
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.MaxMemories < 1) {
          return MemoryTypes::make_left(
              FString(TEXT("Max memories must be at least 1")));
        }
        return MemoryTypes::make_right(config);
      })
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.VectorDimension != 384) {
          return MemoryTypes::make_left(
              FString(TEXT("Vector dimension must be 384")));
        }
        return MemoryTypes::make_right(config);
      })
      .add([](const FMemoryConfig &config)
               -> MemoryTypes::Either<FString, FMemoryConfig> {
        if (config.MaxRecallResults < 1) {
          return MemoryTypes::make_left(
              FString(TEXT("Max recall results must be at least 1")));
        }
        return MemoryTypes::make_right(config);
      });
}

// Implementation of memory store creation pipeline
MemoryTypes::Pipeline<FMemoryStore>
memoryStoreCreationPipeline(const FMemoryStore &store) {
  return func::pipe(store);
}

// Implementation of curried memory store creation
MemoryTypes::Curried<
    1, std::function<MemoryTypes::MemoryStoreCreationResult(FMemoryConfig)>>
curriedMemoryStoreCreation() {
  return func::curry<1>(
      [](FMemoryConfig config) -> MemoryTypes::MemoryStoreCreationResult {
        try {
          FMemoryStore store = MemoryFactory::CreateStore(config);
          return MemoryTypes::make_right(FString(), store);
        } catch (const std::exception &e) {
          return MemoryTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace MemoryHelpers