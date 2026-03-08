#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace MemorySlice {

using namespace rtk;
using namespace func;

inline FString MemoryItemIdSelector(const FMemoryItem &Item) { return Item.Id; }

inline EntityAdapterOps<FMemoryItem> GetMemoryAdapter() {
  return createEntityAdapter<FMemoryItem>(&MemoryItemIdSelector);
}

struct FMemorySliceState {
  EntityState<FMemoryItem> Entities;
  FString StorageStatus;
  FString RecallStatus;
  FString Error;
  TArray<FString> LastRecalledIds;

  FMemorySliceState()
      : Entities(GetMemoryAdapter().getInitialState()),
        StorageStatus(TEXT("idle")), RecallStatus(TEXT("idle")) {}
};

namespace Actions {

inline const EmptyActionCreator &MemoryStoreStartActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("memory/storeStart"));
  return ActionCreator;
}

inline const ActionCreator<FMemoryItem> &MemoryStoreSuccessActionCreator() {
  static const ActionCreator<FMemoryItem> ActionCreator =
      createAction<FMemoryItem>(TEXT("memory/storeSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &MemoryStoreFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("memory/storeFailed"));
  return ActionCreator;
}

inline const EmptyActionCreator &MemoryRecallStartActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("memory/recallStart"));
  return ActionCreator;
}

inline const ActionCreator<TArray<FMemoryItem>> &
MemoryRecallSuccessActionCreator() {
  static const ActionCreator<TArray<FMemoryItem>> ActionCreator =
      createAction<TArray<FMemoryItem>>(TEXT("memory/recallSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &MemoryRecallFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("memory/recallFailed"));
  return ActionCreator;
}

inline const EmptyActionCreator &MemoryClearActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("memory/clear"));
  return ActionCreator;
}

inline AnyAction MemoryStoreStart() {
  return MemoryStoreStartActionCreator()();
}

inline AnyAction MemoryStoreSuccess(const FMemoryItem &Item) {
  return MemoryStoreSuccessActionCreator()(Item);
}

inline AnyAction MemoryStoreFailed(const FString &Error) {
  return MemoryStoreFailedActionCreator()(Error);
}

inline AnyAction MemoryRecallStart() {
  return MemoryRecallStartActionCreator()();
}

inline AnyAction MemoryRecallSuccess(const TArray<FMemoryItem> &Items) {
  return MemoryRecallSuccessActionCreator()(Items);
}

inline AnyAction MemoryRecallFailed(const FString &Error) {
  return MemoryRecallFailedActionCreator()(Error);
}

inline AnyAction MemoryClear() { return MemoryClearActionCreator()(); }

} // namespace Actions

inline Slice<FMemorySliceState> CreateMemorySlice() {
  return SliceBuilder<FMemorySliceState>(TEXT("memory"), FMemorySliceState())
      .addExtraCase(
          Actions::MemoryStoreStartActionCreator(),
          [](const FMemorySliceState &State,
             const Action<rtk::FEmptyPayload> &Action)
              -> FMemorySliceState {
            FMemorySliceState Next = State;
            Next.StorageStatus = TEXT("storing");
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(
          Actions::MemoryStoreSuccessActionCreator(),
          [](const FMemorySliceState &State,
             const Action<FMemoryItem> &Action) -> FMemorySliceState {
            FMemorySliceState Next = State;
            Next.StorageStatus = TEXT("idle");
            Next.Entities =
                GetMemoryAdapter().upsertOne(Next.Entities, Action.PayloadValue);
            return Next;
          })
      .addExtraCase(
          Actions::MemoryStoreFailedActionCreator(),
          [](const FMemorySliceState &State,
             const Action<FString> &Action) -> FMemorySliceState {
            FMemorySliceState Next = State;
            Next.StorageStatus = TEXT("error");
            Next.Error = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::MemoryRecallStartActionCreator(),
          [](const FMemorySliceState &State,
             const Action<rtk::FEmptyPayload> &Action)
              -> FMemorySliceState {
            FMemorySliceState Next = State;
            Next.RecallStatus = TEXT("recalling");
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(
          Actions::MemoryRecallSuccessActionCreator(),
          [](const FMemorySliceState &State,
             const Action<TArray<FMemoryItem>> &Action) -> FMemorySliceState {
            FMemorySliceState Next = State;
            Next.RecallStatus = TEXT("idle");
            Next.Entities =
                GetMemoryAdapter().upsertMany(Next.Entities, Action.PayloadValue);
            Next.LastRecalledIds.Empty(Action.PayloadValue.Num());
            for (int32 Index = 0; Index < Action.PayloadValue.Num(); ++Index) {
              Next.LastRecalledIds.Add(Action.PayloadValue[Index].Id);
            }
            return Next;
          })
      .addExtraCase(
          Actions::MemoryRecallFailedActionCreator(),
          [](const FMemorySliceState &State,
             const Action<FString> &Action) -> FMemorySliceState {
            FMemorySliceState Next = State;
            Next.RecallStatus = TEXT("error");
            Next.Error = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::MemoryClearActionCreator(),
          [](const FMemorySliceState &State,
             const Action<rtk::FEmptyPayload> &Action)
              -> FMemorySliceState {
            return FMemorySliceState();
          })
      .build();
}

inline Maybe<FMemoryItem> SelectMemoryById(const FMemorySliceState &State,
                                           const FString &Id) {
  return GetMemoryAdapter().getSelectors().selectById(State.Entities, Id);
}

inline TArray<FMemoryItem> SelectAllMemories(const FMemorySliceState &State) {
  return GetMemoryAdapter().getSelectors().selectAll(State.Entities);
}

inline TArray<FMemoryItem>
SelectLastRecalledMemories(const FMemorySliceState &State) {
  TArray<FMemoryItem> Results;
  for (int32 Index = 0; Index < State.LastRecalledIds.Num(); ++Index) {
    const Maybe<FMemoryItem> Item =
        SelectMemoryById(State, State.LastRecalledIds[Index]);
    if (Item.hasValue) {
      Results.Add(Item.value);
    }
  }
  return Results;
}

} // namespace MemorySlice
