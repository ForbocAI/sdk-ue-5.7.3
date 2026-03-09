#pragma once

#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Types.h"

namespace BridgeSlice {

using namespace rtk;
using namespace func;

struct FBridgeSliceState {
  TArray<FDirectiveRuleSet> ActivePresets;
  TArray<FDirectiveRuleSet> AvailableRulesets;
  TArray<FString> AvailablePresetIds;
  FValidationResult LastValidation;
  bool bHasLastValidation;
  FString Status;
  FString Error;

  FBridgeSliceState()
      : bHasLastValidation(false), Status(TEXT("idle")) {}
};

namespace Actions {

inline const EmptyActionCreator &BridgeValidationPendingActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("bridge/validationPending"));
  return ActionCreator;
}

inline const ActionCreator<FValidationResult> &
BridgeValidationSuccessActionCreator() {
  static const ActionCreator<FValidationResult> ActionCreator =
      createAction<FValidationResult>(TEXT("bridge/validationSuccess"));
  return ActionCreator;
}

inline const ActionCreator<FString> &BridgeValidationFailedActionCreator() {
  static const ActionCreator<FString> ActionCreator =
      createAction<FString>(TEXT("bridge/validationFailed"));
  return ActionCreator;
}

inline const ActionCreator<TArray<FDirectiveRuleSet>> &
SetActivePresetsActionCreator() {
  static const ActionCreator<TArray<FDirectiveRuleSet>> ActionCreator =
      createAction<TArray<FDirectiveRuleSet>>(TEXT("bridge/setActivePresets"));
  return ActionCreator;
}

inline const ActionCreator<FDirectiveRuleSet> &AddActivePresetActionCreator() {
  static const ActionCreator<FDirectiveRuleSet> ActionCreator =
      createAction<FDirectiveRuleSet>(TEXT("bridge/addActivePreset"));
  return ActionCreator;
}

inline const ActionCreator<TArray<FDirectiveRuleSet>> &
SetAvailableRulesetsActionCreator() {
  static const ActionCreator<TArray<FDirectiveRuleSet>> ActionCreator =
      createAction<TArray<FDirectiveRuleSet>>(TEXT("bridge/setAvailableRulesets"));
  return ActionCreator;
}

inline const ActionCreator<TArray<FString>> &
SetAvailablePresetIdsActionCreator() {
  static const ActionCreator<TArray<FString>> ActionCreator =
      createAction<TArray<FString>>(TEXT("bridge/setAvailablePresetIds"));
  return ActionCreator;
}

inline const EmptyActionCreator &ClearBridgeValidationActionCreator() {
  static const EmptyActionCreator ActionCreator =
      createAction(TEXT("bridge/clearBridgeValidation"));
  return ActionCreator;
}

inline AnyAction BridgeValidationPending() {
  return BridgeValidationPendingActionCreator()();
}

inline AnyAction BridgeValidationSuccess(const FValidationResult &Result) {
  return BridgeValidationSuccessActionCreator()(Result);
}

inline AnyAction BridgeValidationFailure(const FString &Error) {
  return BridgeValidationFailedActionCreator()(Error);
}

inline AnyAction SetActivePresets(const TArray<FDirectiveRuleSet> &Presets) {
  return SetActivePresetsActionCreator()(Presets);
}

inline AnyAction AddActivePreset(const FDirectiveRuleSet &Preset) {
  return AddActivePresetActionCreator()(Preset);
}

inline AnyAction
SetAvailableRulesets(const TArray<FDirectiveRuleSet> &Rulesets) {
  return SetAvailableRulesetsActionCreator()(Rulesets);
}

inline AnyAction SetAvailablePresetIds(const TArray<FString> &PresetIds) {
  return SetAvailablePresetIdsActionCreator()(PresetIds);
}

inline AnyAction ClearBridgeValidation() {
  return ClearBridgeValidationActionCreator()();
}

} // namespace Actions

inline Slice<FBridgeSliceState> CreateBridgeSlice() {
  return SliceBuilder<FBridgeSliceState>(TEXT("bridge"), FBridgeSliceState())
      .addExtraCase(
          Actions::BridgeValidationPendingActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<rtk::FEmptyPayload> &Action) -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            Next.Status = TEXT("validating");
            Next.Error.Empty();
            return Next;
          })
      .addExtraCase(
          Actions::BridgeValidationSuccessActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<FValidationResult> &Action) -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            Next.Status = TEXT("idle");
            Next.LastValidation = Action.PayloadValue;
            Next.bHasLastValidation = true;
            return Next;
          })
      .addExtraCase(
          Actions::BridgeValidationFailedActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<FString> &Action) -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            Next.Status = TEXT("error");
            Next.Error = Action.PayloadValue;
            Next.LastValidation = TypeFactory::Invalid(Action.PayloadValue);
            Next.bHasLastValidation = true;
            return Next;
          })
      .addExtraCase(
          Actions::SetActivePresetsActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<TArray<FDirectiveRuleSet>> &Action)
              -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            Next.ActivePresets = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::AddActivePresetActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<FDirectiveRuleSet> &Action) -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            const FString TargetId =
                Action.PayloadValue.Id.IsEmpty() ? Action.PayloadValue.RulesetId
                                                 : Action.PayloadValue.Id;
            bool bExists = false;
            for (const FDirectiveRuleSet &Preset : Next.ActivePresets) {
              const FString ExistingId =
                  Preset.Id.IsEmpty() ? Preset.RulesetId : Preset.Id;
              if (ExistingId == TargetId) {
                bExists = true;
                break;
              }
            }
            if (!bExists) {
              Next.ActivePresets.Add(Action.PayloadValue);
            }
            return Next;
          })
      .addExtraCase(
          Actions::SetAvailableRulesetsActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<TArray<FDirectiveRuleSet>> &Action)
              -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            Next.AvailableRulesets = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::SetAvailablePresetIdsActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<TArray<FString>> &Action) -> FBridgeSliceState {
            FBridgeSliceState Next = State;
            Next.AvailablePresetIds = Action.PayloadValue;
            return Next;
          })
      .addExtraCase(
          Actions::ClearBridgeValidationActionCreator(),
          [](const FBridgeSliceState &State,
             const Action<rtk::FEmptyPayload> &Action)
              -> FBridgeSliceState {
            return FBridgeSliceState();
          })
      .build();
}

} // namespace BridgeSlice
