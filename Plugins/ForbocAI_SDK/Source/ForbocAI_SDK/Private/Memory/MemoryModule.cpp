#include "Memory/MemoryModule.h"
#include "Core/functional_core.hpp"
#include "HAL/PlatformFilemanager.h"
#include "Memory/MemoryModuleInternal.h"
#include "Memory/MemorySlice.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"

// ==========================================================
// Memory Module Implementation — Full Embedding-Based Memory
// ==========================================================

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
    auto configValidation = MemoryInternal::ValidateConfig(Store.Config);
    if (configValidation.isLeft) {
      return configValidation;
    }

    // Get database path
    const FString DatabasePath = MemoryInternal::GetDatabasePath(Store.Config);

    // Open database connection
    auto openResult = MemoryInternal::SQLiteVSS::OpenDatabase(DatabasePath);
    if (openResult.isLeft) {
      return openResult;
    }
    Store.DatabaseHandle = openResult.right;

    // Create tables
    auto tableResult =
        MemoryInternal::SQLiteVSS::CreateTables(Store.DatabaseHandle);
    if (tableResult.isLeft) {
      MemoryInternal::SQLiteVSS::CloseDatabase(Store.DatabaseHandle);
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

  TSharedPtr<TPromise<MemoryTypes::MemoryStoreAddResult>> Promise =
      MakeShared<TPromise<MemoryTypes::MemoryStoreAddResult>>();

  FMemoryItem Item;
  Item.Id = FString::Printf(TEXT("mem_%s"), *FGuid::NewGuid().ToString());
  Item.Text = Text;
  Item.Type = Type;
  Item.Importance = FMath::Clamp(Importance, 0.0f, 1.0f);
  Item.Timestamp = FDateTime::Now().ToUnixTimestamp();

  auto SDKStore = ConfigureSDKStore();

  // Dispatch the store thunk
  auto ThunkResult = rtk::nodeMemoryStoreThunk(Item)(
      [SDKStore](const rtk::AnyAction &a) { return SDKStore.dispatch(a); },
      [SDKStore]() { return SDKStore.getState(); });

  // Chain result back to the promise
  func::AsyncChain::then<FMemoryItem, void>(
      ThunkResult,
      [Promise, Store](FMemoryItem SavedItem) {
        FMemoryStore NewStore = Store;
        NewStore.Items.Add(SavedItem);
        Promise->SetValue(MemoryTypes::make_right(FString(), NewStore));
      })
      .catch_([Promise](std::string Error) {
        Promise->SetValue(MemoryTypes::make_left(FString(Error.c_str())));
      });

  return Promise->GetFuture();
}

// Add implementation
TFuture<MemoryTypes::MemoryStoreAddResult>
MemoryOps::Add(const FMemoryStore &Store, const FMemoryItem &Item) {
  return MemoryOps::Store(Store, Item.Text, Item.Type, Item.Importance);
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

  TSharedPtr<TPromise<MemoryTypes::MemoryStoreRecallResult>> Promise =
      MakeShared<TPromise<MemoryTypes::MemoryStoreRecallResult>>();

  auto SDKStore = ConfigureSDKStore();

  // Dispatch the recall thunk
  auto ThunkResult = rtk::nodeMemoryRecallThunk(Query)(
      [SDKStore](const rtk::AnyAction &a) { return SDKStore.dispatch(a); },
      [SDKStore]() { return SDKStore.getState(); });

  // Chain result back to the promise
  func::AsyncChain::then<TArray<FMemoryItem>, void>(
      ThunkResult,
      [Promise](TArray<FMemoryItem> Results) {
        Promise->SetValue(MemoryTypes::make_right(FString(), Results));
      })
      .catch_([Promise](std::string Error) {
        Promise->SetValue(MemoryTypes::make_left(FString(Error.c_str())));
      });

  return Promise->GetFuture();
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
                   return MemoryInternal::SQLiteVSS::GenerateEmbedding(
                       Store.DatabaseHandle, Text);
                 } catch (const std::exception &e) {
                   return MemoryTypes::make_left(FString(e.what()));
                 }
               });
}

// Statistics implementation
FString MemoryOps::GetStatistics(const FMemoryStore &Store) {
  return MemoryInternal::GetMemoryStatistics(Store);
}

// Cleanup implementation
void MemoryOps::Cleanup(FMemoryStore &Store) {
  if (Store.bInitialized && Store.DatabaseHandle != nullptr) {
    MemoryInternal::SQLiteVSS::CloseDatabase(Store.DatabaseHandle);
    Store.DatabaseHandle = nullptr;
    Store.bInitialized = false;
  }
}

// Functional helper implementations
namespace MemoryHelpers {
MemoryTypes::Lazy<FMemoryStore>
createLazyMemoryStore(const FMemoryConfig &config) {
  return func::lazy([config]() -> FMemoryStore {
    return MemoryFactory::CreateStore(config);
  });
}

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

MemoryTypes::Pipeline<FMemoryStore>
memoryStoreCreationPipeline(const FMemoryStore &store) {
  return func::pipe(store);
}

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