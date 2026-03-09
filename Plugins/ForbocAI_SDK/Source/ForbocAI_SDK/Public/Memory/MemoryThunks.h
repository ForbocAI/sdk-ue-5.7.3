#pragma once

#include "Core/ThunkDetail.h"
#include "Memory/MemorySlice.h"

namespace rtk {

// Forward declarations
inline ThunkAction<FMemoryItem, FSDKState>
nodeMemoryStoreThunk(const FMemoryItem &Item);

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
nodeMemoryRecallThunk(const FMemoryRecallRequest &Request);

// ---------------------------------------------------------------------------
// Node memory thunks (mirrors TS nodeMemorySlice.ts)
// ---------------------------------------------------------------------------

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
initNodeMemoryThunk(const FString &DatabasePath = TEXT("")) {
  return [DatabasePath](std::function<AnyAction(const AnyAction &)> Dispatch,
                        std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncResult<rtk::FEmptyPayload>::create(
        [DatabasePath](std::function<void(rtk::FEmptyPayload)> Resolve,
                       std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [DatabasePath, Resolve, Reject]() {
            Native::Sqlite::DB &Handle = detail::NodeMemoryHandle();
            if (Handle) {
              Native::Sqlite::Close(Handle);
              Handle = nullptr;
            }

            const FString Path = DatabasePath.IsEmpty()
                                     ? detail::DefaultNodeMemoryPath()
                                     : DatabasePath;
            Handle = Native::Sqlite::Open(Path);

            AsyncTask(ENamedThreads::GameThread, [Handle, Resolve, Reject]() {
              if (Handle) {
                Resolve(rtk::FEmptyPayload{});
              } else {
                Reject("Failed to initialize node memory database");
              }
            });
          });
        });
  };
}

inline ThunkAction<FMemoryItem, FSDKState>
nodeMemoryStoreThunk(const FMemoryItem &Item) {
  return [Item](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<FMemoryItem> {
    Dispatch(MemorySlice::Actions::MemoryStoreStart());

    return func::AsyncResult<FMemoryItem>::create(
        [Item, Dispatch](std::function<void(FMemoryItem)> Resolve,
                         std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Item, Dispatch, Resolve, Reject]() {
            Native::Sqlite::DB Db = detail::EnsureNodeMemoryDatabase();
            if (!Db) {
              const FString Error = TEXT("Failed to open node memory database");
              AsyncTask(ENamedThreads::GameThread, [Dispatch, Reject, Error]() {
                Dispatch(MemorySlice::Actions::MemoryStoreFailed(Error));
                Reject(TCHAR_TO_UTF8(*Error));
              });
              return;
            }

            FMemoryItem Stored = Item;
            Stored.Embedding =
                Native::Llama::Embed(detail::NodeCortexHandle(), Stored.Text);
            const bool bStored =
                Native::Sqlite::Upsert(Db, Stored, Stored.Embedding);

            AsyncTask(ENamedThreads::GameThread,
                      [Dispatch, Resolve, Reject, Stored, bStored]() {
                        if (bStored) {
                          Dispatch(MemorySlice::Actions::MemoryStoreSuccess(
                              Stored));
                          Resolve(Stored);
                        } else {
                          const FString Error =
                              TEXT("Failed to store local memory");
                          Dispatch(MemorySlice::Actions::MemoryStoreFailed(
                              Error));
                          Reject(TCHAR_TO_UTF8(*Error));
                        }
                      });
          });
        });
  };
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
nodeMemoryRecallThunk(const FMemoryRecallRequest &Request) {
  return [Request](std::function<AnyAction(const AnyAction &)> Dispatch,
                   std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FMemoryItem>> {
    Dispatch(MemorySlice::Actions::MemoryRecallStart());

    return func::AsyncResult<TArray<FMemoryItem>>::create(
        [Request, Dispatch](std::function<void(TArray<FMemoryItem>)> Resolve,
                            std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Request, Dispatch, Resolve, Reject]() {
            Native::Sqlite::DB Db = detail::EnsureNodeMemoryDatabase();
            if (!Db) {
              const FString Error = TEXT("Failed to open node memory database");
              AsyncTask(ENamedThreads::GameThread, [Dispatch, Reject, Error]() {
                Dispatch(MemorySlice::Actions::MemoryRecallFailed(Error));
                Reject(TCHAR_TO_UTF8(*Error));
              });
              return;
            }

            const TArray<float> QueryEmbedding =
                Native::Llama::Embed(detail::NodeCortexHandle(), Request.Query);
            TArray<FMemoryItem> Results =
                Native::Sqlite::Search(Db, QueryEmbedding, Request.Limit);

            if (Request.Threshold > 0.0f) {
              Results.RemoveAll([Request](const FMemoryItem &Item) {
                return Item.Similarity < Request.Threshold;
              });
            }

            AsyncTask(ENamedThreads::GameThread,
                      [Dispatch, Resolve, Results]() {
                        Dispatch(MemorySlice::Actions::MemoryRecallSuccess(
                            Results));
                        Resolve(Results);
                      });
          });
        });
  };
}

// Convenience wrappers
inline ThunkAction<FMemoryItem, FSDKState>
storeNodeMemoryThunk(const FString &Text,
                     const FString &Type = TEXT("observation"),
                     float Importance = 0.5f) {
  FMemoryStoreInstruction Instruction;
  Instruction.Text = Text;
  Instruction.Type = Type;
  Instruction.Importance = Importance;
  return nodeMemoryStoreThunk(detail::MakeMemoryItem(Instruction));
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
recallNodeMemoryThunk(const FString &Query, int32 Limit = 10,
                      float Threshold = 0.7f) {
  FMemoryRecallRequest Request;
  Request.Query = Query;
  Request.Limit = Limit;
  Request.Threshold = Threshold;
  return nodeMemoryRecallThunk(Request);
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState> clearNodeMemoryThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncResult<rtk::FEmptyPayload>::create(
        [Dispatch](std::function<void(rtk::FEmptyPayload)> Resolve,
                   std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Dispatch, Resolve]() {
            Native::Sqlite::DB &Handle = detail::NodeMemoryHandle();
            if (Handle) {
              Native::Sqlite::Close(Handle);
              Handle = nullptr;
            }

            IFileManager::Get().Delete(*detail::DefaultNodeMemoryPath(), false,
                                       true, true);

            AsyncTask(ENamedThreads::GameThread, [Dispatch, Resolve]() {
              Dispatch(MemorySlice::Actions::MemoryClear());
              Resolve(rtk::FEmptyPayload{});
            });
          });
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState> initNodeVectorThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return detail::ResolveAsync(rtk::FEmptyPayload{});
  };
}

// ---------------------------------------------------------------------------
// Remote memory thunks (mirrors TS core thunks.ts)
// ---------------------------------------------------------------------------

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
storeMemoryRemoteThunk(const FString &NpcId, const FString &Observation,
                       float Importance = 0.8f) {
  return [NpcId, Observation, Importance](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return APISlice::Endpoints::postMemoryStore(
        NpcId, TypeFactory::RemoteMemoryStoreRequest(Observation, Importance))(
        Dispatch, GetState);
  };
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
listMemoryRemoteThunk(const FString &NpcId) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FMemoryItem>> {
    return func::AsyncChain::then<TArray<FMemoryItem>, TArray<FMemoryItem>>(
        APISlice::Endpoints::getMemoryList(NpcId)(Dispatch, GetState),
        [Dispatch](const TArray<FMemoryItem> &Items) {
          Dispatch(MemorySlice::Actions::MemoryRecallSuccess(Items));
          return detail::ResolveAsync(Items);
        });
  };
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
recallMemoryRemoteThunk(const FString &NpcId, const FString &Query,
                        float Similarity = 0.0f) {
  return [NpcId, Query, Similarity](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FMemoryItem>> {
    Dispatch(MemorySlice::Actions::MemoryRecallStart());
    return func::AsyncChain::then<TArray<FMemoryItem>, TArray<FMemoryItem>>(
        APISlice::Endpoints::postMemoryRecall(
            NpcId, TypeFactory::RemoteMemoryRecallRequest(Query, Similarity))(
            Dispatch, GetState),
        [Dispatch](const TArray<FMemoryItem> &Items) {
          Dispatch(MemorySlice::Actions::MemoryRecallSuccess(Items));
          return detail::ResolveAsync(Items);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(MemorySlice::Actions::MemoryRecallFailed(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
clearMemoryRemoteThunk(const FString &NpcId) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncChain::then<rtk::FEmptyPayload, rtk::FEmptyPayload>(
        APISlice::Endpoints::deleteMemoryClear(NpcId)(Dispatch, GetState),
        [Dispatch](const rtk::FEmptyPayload &Payload) {
          Dispatch(MemorySlice::Actions::MemoryClear());
          return detail::ResolveAsync(Payload);
        });
  };
}

} // namespace rtk
