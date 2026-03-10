#include "Memory/MemoryModule.h"
#include "Core/functional_core.hpp"
#include "HAL/PlatformFilemanager.h"
#include "Memory/MemoryModuleInternal.h"
#include "Memory/MemorySlice.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RuntimeStore.h"
#include "Serialization/JsonSerializer.h"
#include "Memory/MemoryThunks.h"

// ==========================================================
// Memory Module Implementation — Full Embedding-Based Memory
// ==========================================================

// Factory function implementation
FMemoryStore MemoryFactory::CreateStore(const FMemoryConfig &Config) {
  // Use functional validation
  auto validationResult =
      MemoryHelpers::memoryConfigValidationPipeline().run(Config);
  if (validationResult.isLeft) {
    throw std::runtime_error(TCHAR_TO_UTF8(*validationResult.left));
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
    return MemoryTypes::make_left(FString(e.what()), false);
  }
}

// Store implementation
TFuture<MemoryTypes::MemoryStoreAddResult>
MemoryOps::Store(const FMemoryStore &Store, const FString &Text,
                 const FString &Type, float Importance) {
  if (!Store.bInitialized) {
    TPromise<MemoryTypes::MemoryStoreAddResult> Promise;
    Promise.SetValue(MemoryTypes::make_left(
        FString(TEXT("Memory store not initialized")), FMemoryStore{}));
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

  auto RuntimeStore = ConfigureStore();

  // Dispatch the store thunk
  auto ThunkResult = rtk::nodeMemoryStoreThunk(Item)(
      [RuntimeStore](const rtk::AnyAction &a) {
        return RuntimeStore.dispatch(a);
      },
      [RuntimeStore]() { return RuntimeStore.getState(); });

  // Chain result back to the promise
  ThunkResult
      .then([Promise, Store](FMemoryItem SavedItem) {
        FMemoryStore NewStore = Store;
        NewStore.Items.Add(SavedItem);
        Promise->SetValue(MemoryTypes::make_right(FString(), NewStore));
      })
      .catch_([Promise](std::string Error) {
        Promise->SetValue(
            MemoryTypes::make_left(FString(Error.c_str()), FMemoryStore{}));
      })
      .execute();

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
    Promise.SetValue(MemoryTypes::make_left(
        FString(TEXT("Memory store not initialized")), TArray<FMemoryItem>{}));
    return Promise.GetFuture();
  }

  TSharedPtr<TPromise<MemoryTypes::MemoryStoreRecallResult>> Promise =
      MakeShared<TPromise<MemoryTypes::MemoryStoreRecallResult>>();

  auto RuntimeStore = ConfigureStore();
  FMemoryRecallRequest RecallRequest;
  RecallRequest.Query = Query;
  RecallRequest.Limit = Limit < 0 ? Store.Config.MaxRecallResults : Limit;
  RecallRequest.Threshold = 0.0f;

  // Dispatch the recall thunk
  auto ThunkResult = rtk::nodeMemoryRecallThunk(RecallRequest)(
      [RuntimeStore](const rtk::AnyAction &a) {
        return RuntimeStore.dispatch(a);
      },
      [RuntimeStore]() { return RuntimeStore.getState(); });

  // Chain result back to the promise
  ThunkResult
      .then([Promise](TArray<FMemoryItem> Results) {
        Promise->SetValue(MemoryTypes::make_right(FString(), Results));
      })
      .catch_([Promise](std::string Error) {
        Promise->SetValue(MemoryTypes::make_left(FString(Error.c_str()),
                                                 TArray<FMemoryItem>{}));
      })
      .execute();

  return Promise->GetFuture();
}

// Embedding generation implementation
TFuture<MemoryTypes::MemoryStoreEmbeddingResult>
MemoryOps::GenerateEmbedding(const FMemoryStore &Store, const FString &Text) {
  if (!Store.bInitialized) {
    TPromise<MemoryTypes::MemoryStoreEmbeddingResult> Promise;
    Promise.SetValue(MemoryTypes::make_left(
        FString(TEXT("Memory store not initialized")), TArray<float>{}));
    return Promise.GetFuture();
  }

  return Async(EAsyncExecution::Thread,
               [Store, Text]() -> MemoryTypes::MemoryStoreEmbeddingResult {
                 try {
                   return MemoryInternal::SQLiteVSS::GenerateEmbedding(
                       Store.DatabaseHandle, Text);
                 } catch (const std::exception &e) {
                   return MemoryTypes::make_left(FString(e.what()),
                                                 TArray<float>{});
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
