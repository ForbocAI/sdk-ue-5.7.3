#pragma once
// personas and souls; the essence of the machine

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace SoulSlice {

using namespace rtk;

struct FSoulSliceState {
  FString ExportStatus;
  FString ImportStatus;
  FSoulExportResult LastExport;
  bool bHasLastExport;
  FSoul LastImport;
  bool bHasLastImport;
  TArray<FSoulListItem> AvailableSouls;
  FString Error;

  FSoulSliceState()
      : ExportStatus(TEXT("idle")), ImportStatus(TEXT("idle")),
        bHasLastExport(false), bHasLastImport(false) {}
};

namespace Actions {

inline const EmptyActionCreator &RemoteExportSoulPendingActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("soul/remoteExportPending"));
  return ActionCreator;
}

inline const ActionCreator<FSoulExportResult> &
RemoteExportSoulSuccessActionCreator() {
  static const ActionCreator<FSoulExportResult> ActionCreator =
      createAction<FSoulExportResult>(TEXT("soul/remoteExportSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &RemoteExportSoulFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("soul/remoteExportFailed"));
  return ActionCreator;
}

inline const EmptyActionCreator &ImportSoulPendingActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("soul/importPending"));
  return ActionCreator;
}

inline const ActionCreator<FSoul> &ImportSoulSuccessActionCreator() {
  static const ActionCreator<FSoul> ActionCreator =
      createAction<FSoul>(TEXT("soul/importSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &ImportSoulFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("soul/importFailed"));
  return ActionCreator;
}

inline const ActionCreator<TArray<FSoulListItem>> &SetSoulListActionCreator() {
  static const ActionCreator<TArray<FSoulListItem>> ActionCreator =
      createAction<TArray<FSoulListItem>>(TEXT("soul/setSoulList"));
  return ActionCreator;
}

inline const EmptyActionCreator &ClearSoulStateActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("soul/clearSoulState"));
  return ActionCreator;
}

inline AnyAction RemoteExportSoulPending() {
  return RemoteExportSoulPendingActionCreator()();
}

inline AnyAction RemoteExportSoulSuccess(const FSoulExportResult &Result) {
  return RemoteExportSoulSuccessActionCreator()(Result);
}

inline AnyAction RemoteExportSoulFailed(const FString &Error) {
  return RemoteExportSoulFailedActionCreator()(Error);
}

inline AnyAction ImportSoulPending() {
  return ImportSoulPendingActionCreator()();
}

inline AnyAction ImportSoulSuccess(const FSoul &Soul) {
  return ImportSoulSuccessActionCreator()(Soul);
}

inline AnyAction ImportSoulFailed(const FString &Error) {
  return ImportSoulFailedActionCreator()(Error);
}

inline AnyAction SetSoulList(const TArray<FSoulListItem> &SoulList) {
  return SetSoulListActionCreator()(SoulList);
}

inline AnyAction ClearSoulState() { return ClearSoulStateActionCreator()(); }

} // namespace Actions

inline Slice<FSoulSliceState> CreateSoulSlice() {
  return SliceBuilder<FSoulSliceState>(TEXT("soul"), FSoulSliceState())
      .addExtraCase(
          Actions::RemoteExportSoulPendingActionCreator(),
          [](const FSoulSliceState &State,
             const Action<rtk::FEmptyPayload> &Action) -> FSoulSliceState {
            FSoulSliceState Next = State;
            Next.ExportStatus = TEXT("exporting");
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(
          Actions::RemoteExportSoulSuccessActionCreator(),
          [](const FSoulSliceState &State,
             const Action<FSoulExportResult> &Action) -> FSoulSliceState {
            FSoulSliceState Next = State;
            Next.ExportStatus = TEXT("success");
            Next.LastExport = Action.PayloadValue;
            Next.bHasLastExport = true;
            return Next;
          })
      .addExtraCase(Actions::RemoteExportSoulFailedActionCreator(),
                    [](const FSoulSliceState &State,
                       const Action<FString> &Action) -> FSoulSliceState {
                      FSoulSliceState Next = State;
                      Next.ExportStatus = TEXT("failed");
                      Next.Error = Action.PayloadValue;
                      return Next;
                    })
      .addExtraCase(
          Actions::ImportSoulPendingActionCreator(),
          [](const FSoulSliceState &State,
             const Action<rtk::FEmptyPayload> &Action) -> FSoulSliceState {
            FSoulSliceState Next = State;
            Next.ImportStatus = TEXT("importing");
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(Actions::ImportSoulSuccessActionCreator(),
                    [](const FSoulSliceState &State,
                       const Action<FSoul> &Action) -> FSoulSliceState {
                      FSoulSliceState Next = State;
                      Next.ImportStatus = TEXT("success");
                      Next.LastImport = Action.PayloadValue;
                      Next.bHasLastImport = true;
                      return Next;
                    })
      .addExtraCase(Actions::ImportSoulFailedActionCreator(),
                    [](const FSoulSliceState &State,
                       const Action<FString> &Action) -> FSoulSliceState {
                      FSoulSliceState Next = State;
                      Next.ImportStatus = TEXT("failed");
                      Next.Error = Action.PayloadValue;
                      return Next;
                    })
      .addExtraCase(
          Actions::SetSoulListActionCreator(),
          [](const FSoulSliceState &State,
             const Action<TArray<FSoulListItem>> &Action) -> FSoulSliceState {
            FSoulSliceState Next = State;
            Next.AvailableSouls = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::ClearSoulStateActionCreator(),
          [](const FSoulSliceState &State,
             const Action<rtk::FEmptyPayload> &Action) -> FSoulSliceState {
            return FSoulSliceState();
          })
      .build();
}

} // namespace SoulSlice
