#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

// ==========================================================
// Memory Slice (UE RTK)
// ==========================================================
// Equivalent to `memorySlice.ts`
// Manages the state of Memory Items using an EntityAdapter,
// and tracks async operations (storage, recall).
// ==========================================================

namespace MemorySlice {

using namespace rtk;
using namespace func;

namespace AsyncStatus {
const FString Idle = TEXT("idle");
const FString Loading = TEXT("loading");
const FString Succeeded = TEXT("succeeded");
const FString Failed = TEXT("failed");
} // namespace AsyncStatus

inline FString MemoryItemIdSelector(const FMemoryItem &Item) { return Item.Id; }

// Global EntityAdapter instance for Memories
inline EntityAdapter<FMemoryItem, decltype(&MemoryItemIdSelector)>
GetMemoryAdapter() {
  return EntityAdapter<FMemoryItem, decltype(&MemoryItemIdSelector)>(
      &MemoryItemIdSelector);
}

// The root state for the Memory slice
struct FMemorySliceState {
  EntityState<FMemoryItem> Entities;
  FString StorageStatus;
  FString RecallStatus;
  Maybe<FString> Error;
  TArray<FString> LastRecalledIds;

  FMemorySliceState()
      : Entities(GetMemoryAdapter().getInitialState()),
        StorageStatus(AsyncStatus::Idle), RecallStatus(AsyncStatus::Idle) {}
};

// --- Actions ---
struct MemoryStorePendingAction : public Action {
  static const FString Type;
  MemoryStorePendingAction() : Action(Type) {}
};
inline const FString MemoryStorePendingAction::Type =
    TEXT("memory/storePending");

struct MemoryStoreSuccessAction : public Action {
  static const FString Type;
  FMemoryItem Payload;
  MemoryStoreSuccessAction(FMemoryItem InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString MemoryStoreSuccessAction::Type =
    TEXT("memory/storeSuccess");

struct MemoryStoreFailedAction : public Action {
  static const FString Type;
  FString Payload;
  MemoryStoreFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString MemoryStoreFailedAction::Type = TEXT("memory/storeFailed");

struct MemoryRecallPendingAction : public Action {
  static const FString Type;
  MemoryRecallPendingAction() : Action(Type) {}
};
inline const FString MemoryRecallPendingAction::Type =
    TEXT("memory/recallPending");

struct MemoryRecallSuccessAction : public Action {
  static const FString Type;
  TArray<FMemoryItem> Payload;
  MemoryRecallSuccessAction(TArray<FMemoryItem> InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString MemoryRecallSuccessAction::Type =
    TEXT("memory/recallSuccess");

struct MemoryRecallFailedAction : public Action {
  static const FString Type;
  FString Payload;
  MemoryRecallFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString MemoryRecallFailedAction::Type =
    TEXT("memory/recallFailed");

struct MemoryClearAction : public Action {
  static const FString Type;
  MemoryClearAction() : Action(Type) {}
};
namespace Actions {
inline MemoryStorePendingAction MemoryStorePending(FString Text) {
  return MemoryStorePendingAction();
}
inline MemoryStoreSuccessAction MemoryStoreSuccess(FMemoryItem Item) {
  return MemoryStoreSuccessAction(MoveTemp(Item));
}
inline MemoryStoreFailedAction MemoryStoreFailure(FString Error) {
  return MemoryStoreFailedAction(MoveTemp(Error));
}
inline MemoryRecallPendingAction MemoryRecallPending(FString Query) {
  return MemoryRecallPendingAction();
}
inline MemoryRecallSuccessAction
MemoryRecallSuccess(TArray<FMemoryItem> Results) {
  return MemoryRecallSuccessAction(MoveTemp(Results));
}
inline MemoryRecallFailedAction MemoryRecallFailure(FString Error) {
  return MemoryRecallFailedAction(MoveTemp(Error));
}
inline MemoryClearAction MemoryClear() { return MemoryClearAction(); }
} // namespace Actions

// --- Slice Builder ---
inline Slice<FMemorySliceState> CreateMemorySlice() {
  return SliceBuilder<FMemorySliceState>(TEXT("memory"), FMemorySliceState())
      .addCase<MemoryStorePendingAction>(
          [](FMemorySliceState State, const MemoryStorePendingAction &Action) {
            State.StorageStatus = AsyncStatus::Loading;
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<MemoryStoreSuccessAction>(
          [](FMemorySliceState State, const MemoryStoreSuccessAction &Action) {
            State.StorageStatus = AsyncStatus::Succeeded;
            State.Entities =
                GetMemoryAdapter().upsertOne(State.Entities, Action.Payload);
            return State;
          })
      .addCase<MemoryStoreFailedAction>(
          [](FMemorySliceState State, const MemoryStoreFailedAction &Action) {
            State.StorageStatus = AsyncStatus::Failed;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<MemoryRecallPendingAction>(
          [](FMemorySliceState State, const MemoryRecallPendingAction &Action) {
            State.RecallStatus = AsyncStatus::Loading;
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<MemoryRecallSuccessAction>(
          [](FMemorySliceState State, const MemoryRecallSuccessAction &Action) {
            State.RecallStatus = AsyncStatus::Succeeded;
            State.Entities =
                GetMemoryAdapter().upsertMany(State.Entities, Action.Payload);

            State.LastRecalledIds.Empty(Action.Payload.Num());
            for (const FMemoryItem &Item : Action.Payload) {
              State.LastRecalledIds.Add(Item.Id);
            }
            return State;
          })
      .addCase<MemoryRecallFailedAction>(
          [](FMemorySliceState State, const MemoryRecallFailedAction &Action) {
            State.RecallStatus = AsyncStatus::Failed;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<MemoryClearAction>(
          [](FMemorySliceState State, const MemoryClearAction &Action) {
            State.Entities = GetMemoryAdapter().removeAll(State.Entities);
            State.StorageStatus = AsyncStatus::Idle;
            State.RecallStatus = AsyncStatus::Idle;
            State.Error = nothing<FString>();
            State.LastRecalledIds.Empty();
            return State;
          })
      .build();
}

} // namespace MemorySlice
