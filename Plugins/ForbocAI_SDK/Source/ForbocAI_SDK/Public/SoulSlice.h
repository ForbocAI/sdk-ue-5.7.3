#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

// ==========================================================
// Soul Slice (UE RTK)
// ==========================================================
// Equivalent to `soulSlice.ts`
// Manages the persistence, export, and import of Soul (Portable Identity).
// ==========================================================

namespace SoulSlice {

using namespace rtk;
using namespace func;

enum class ESoulOpStatus : uint8 { Idle, Processing, Completed, Error };

// The root state for the Soul slice
struct FSoulSliceState {
  ESoulOpStatus ExportStatus;
  ESoulOpStatus ImportStatus;
  Maybe<FSoul> LastExport;
  Maybe<FSoul> LastImport;
  TArray<FString> AvailableSoulIds;
  Maybe<FString> Error;

  FSoulSliceState()
      : ExportStatus(ESoulOpStatus::Idle), ImportStatus(ESoulOpStatus::Idle) {}
};

// --- Actions ---
struct SoulExportPendingAction : public Action {
  static const FString Type;
  SoulExportPendingAction() : Action(Type) {}
};
inline const FString SoulExportPendingAction::Type = TEXT("soul/exportPending");

struct SoulExportSuccessAction : public Action {
  static const FString Type;
  FSoul Payload;
  SoulExportSuccessAction(FSoul InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SoulExportSuccessAction::Type = TEXT("soul/exportSuccess");

struct SoulExportFailedAction : public Action {
  static const FString Type;
  FString Payload;
  SoulExportFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SoulExportFailedAction::Type = TEXT("soul/exportFailed");

struct SoulImportPendingAction : public Action {
  static const FString Type;
  SoulImportPendingAction() : Action(Type) {}
};
inline const FString SoulImportPendingAction::Type = TEXT("soul/importPending");

struct SoulImportSuccessAction : public Action {
  static const FString Type;
  FSoul Payload;
  SoulImportSuccessAction(FSoul InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SoulImportSuccessAction::Type = TEXT("soul/importSuccess");

struct SoulImportFailedAction : public Action {
  static const FString Type;
  FString Payload;
  SoulImportFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SoulImportFailedAction::Type = TEXT("soul/importFailed");

struct ClearSoulStateAction : public Action {
  static const FString Type;
  ClearSoulStateAction() : Action(Type) {}
};
inline const FString ClearSoulStateAction::Type = TEXT("soul/clearSoulState");

// --- Slice Builder ---
inline Slice<FSoulSliceState> CreateSoulSlice() {
  return SliceBuilder<FSoulSliceState>(TEXT("soul"), FSoulSliceState())
      .addCase<SoulExportPendingAction>(
          [](FSoulSliceState State, const SoulExportPendingAction &Action) {
            State.ExportStatus = ESoulOpStatus::Processing;
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<SoulExportSuccessAction>(
          [](FSoulSliceState State, const SoulExportSuccessAction &Action) {
            State.ExportStatus = ESoulOpStatus::Completed;
            State.LastExport = just(Action.Payload);
            return State;
          })
      .addCase<SoulExportFailedAction>(
          [](FSoulSliceState State, const SoulExportFailedAction &Action) {
            State.ExportStatus = ESoulOpStatus::Error;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<SoulImportPendingAction>(
          [](FSoulSliceState State, const SoulImportPendingAction &Action) {
            State.ImportStatus = ESoulOpStatus::Processing;
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<SoulImportSuccessAction>(
          [](FSoulSliceState State, const SoulImportSuccessAction &Action) {
            State.ImportStatus = ESoulOpStatus::Completed;
            State.LastImport = just(Action.Payload);
            return State;
          })
      .addCase<SoulImportFailedAction>(
          [](FSoulSliceState State, const SoulImportFailedAction &Action) {
            State.ImportStatus = ESoulOpStatus::Error;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<ClearSoulStateAction>(
          [](FSoulSliceState State, const ClearSoulStateAction &Action) {
            State.ExportStatus = ESoulOpStatus::Idle;
            State.ImportStatus = ESoulOpStatus::Idle;
            State.LastExport = nothing<FSoul>();
            State.LastImport = nothing<FSoul>();
            State.Error = nothing<FString>();
            return State;
          })
      .build();
}

} // namespace SoulSlice
