#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"

// ==========================================================
// Bridge Slice (UE RTK)
// ==========================================================
// Equivalent to `bridgeSlice.ts`
// Manages the policy validation lane, rulesets, and presets.
// ==========================================================

namespace BridgeSlice {

using namespace rtk;
using namespace func;

enum class EBridgeStatus : uint8 { Idle, Validating, Validated, Error };

// The root state for the Bridge slice
struct FBridgeSliceState {
  TArray<FString> ActivePresetIds;
  TArray<FString> AvailablePresetIds;
  TMap<FString, FString> AvailableRulesets; // Id -> Description/Version
  Maybe<FValidationResult> LastValidation;
  EBridgeStatus Status;
  Maybe<FString> Error;

  FBridgeSliceState() : Status(EBridgeStatus::Idle) {}
};

// --- Actions ---
struct BridgeValidationPendingAction : public Action {
  static const FString Type;
  BridgeValidationPendingAction() : Action(Type) {}
};
inline const FString BridgeValidationPendingAction::Type =
    TEXT("bridge/validationPending");

struct BridgeValidationSuccessAction : public Action {
  static const FString Type;
  FValidationResult Payload;
  BridgeValidationSuccessAction(FValidationResult InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString BridgeValidationSuccessAction::Type =
    TEXT("bridge/validationSuccess");

struct BridgeValidationFailedAction : public Action {
  static const FString Type;
  FString Payload; // Error message
  BridgeValidationFailedAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString BridgeValidationFailedAction::Type =
    TEXT("bridge/validationFailed");

struct SetActivePresetsAction : public Action {
  static const FString Type;
  TArray<FString> Payload; // PresetIds
  SetActivePresetsAction(TArray<FString> InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetActivePresetsAction::Type =
    TEXT("bridge/setActivePresets");

struct ClearBridgeValidationAction : public Action {
  static const FString Type;
  ClearBridgeValidationAction() : Action(Type) {}
};
namespace Actions {
inline BridgeValidationPendingAction BridgeValidationPending() {
  return BridgeValidationPendingAction();
}
inline BridgeValidationSuccessAction
BridgeValidationSuccess(FValidationResult Result) {
  return BridgeValidationSuccessAction(MoveTemp(Result));
}
inline BridgeValidationFailedAction BridgeValidationFailure(FString Error) {
  return BridgeValidationFailedAction(MoveTemp(Error));
}
inline SetActivePresetsAction SetActivePresets(TArray<FString> Presets) {
  return SetActivePresetsAction(MoveTemp(Presets));
}
inline ClearBridgeValidationAction ClearBridgeValidation() {
  return ClearBridgeValidationAction();
}
} // namespace Actions

using namespace Actions; // Export to BridgeSlice namespace

// --- Slice Builder ---
inline Slice<FBridgeSliceState> CreateBridgeSlice() {
  return SliceBuilder<FBridgeSliceState>(TEXT("bridge"), FBridgeSliceState())
      .addCase<BridgeValidationPendingAction>(
          [](FBridgeSliceState State,
             const BridgeValidationPendingAction &Action) {
            State.Status = EBridgeStatus::Validating;
            State.Error = nothing<FString>();
            return State;
          })
      .addCase<BridgeValidationSuccessAction>(
          [](FBridgeSliceState State,
             const BridgeValidationSuccessAction &Action) {
            State.Status = EBridgeStatus::Validated;
            State.LastValidation = just(Action.Payload);
            return State;
          })
      .addCase<BridgeValidationFailedAction>(
          [](FBridgeSliceState State,
             const BridgeValidationFailedAction &Action) {
            State.Status = EBridgeStatus::Error;
            State.Error = just(Action.Payload);
            return State;
          })
      .addCase<SetActivePresetsAction>(
          [](FBridgeSliceState State, const SetActivePresetsAction &Action) {
            State.ActivePresetIds = Action.Payload;
            return State;
          })
      .addCase<ClearBridgeValidationAction>(
          [](FBridgeSliceState State,
             const ClearBridgeValidationAction &Action) {
            State.Status = EBridgeStatus::Idle;
            State.LastValidation = nothing<FValidationResult>();
            State.Error = nothing<FString>();
            return State;
          })
      .build();
}

} // namespace BridgeSlice
