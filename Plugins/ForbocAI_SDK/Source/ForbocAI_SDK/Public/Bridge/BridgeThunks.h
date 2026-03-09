#pragma once

#include "Core/ThunkDetail.h"
#include "Bridge/BridgeSlice.h"

namespace rtk {

// ---------------------------------------------------------------------------
// Bridge thunks (mirrors TS bridgeSlice.ts)
// ---------------------------------------------------------------------------

inline ThunkAction<FValidationResult, FSDKState>
validateBridgeThunk(const FAgentAction &Action,
                    const FBridgeValidationContext &Context,
                    const FString &NpcId = TEXT("")) {
  return [Action, Context, NpcId](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<FValidationResult> {
    Dispatch(BridgeSlice::Actions::BridgeValidationPending());
    return func::AsyncChain::then<FValidationResult, FValidationResult>(
        APISlice::Endpoints::postBridgeValidate(
            NpcId, TypeFactory::BridgeValidateRequest(Action, Context))(
            Dispatch, GetState),
        [Dispatch](const FValidationResult &Result) {
          Dispatch(BridgeSlice::Actions::BridgeValidationSuccess(Result));
          return detail::ResolveAsync(Result);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(BridgeSlice::Actions::BridgeValidationFailure(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FDirectiveRuleSet, FSDKState>
loadBridgePresetThunk(const FString &PresetName) {
  return [PresetName](std::function<AnyAction(const AnyAction &)> Dispatch,
                      std::function<FSDKState()> GetState)
             -> func::AsyncResult<FDirectiveRuleSet> {
    return func::AsyncChain::then<FDirectiveRuleSet, FDirectiveRuleSet>(
        APISlice::Endpoints::postBridgePreset(PresetName)(Dispatch, GetState),
        [Dispatch, PresetName](const FDirectiveRuleSet &Ruleset) {
          Dispatch(BridgeSlice::Actions::AddActivePresetId(
              Ruleset.Id.IsEmpty() ? PresetName : Ruleset.Id));
          return detail::ResolveAsync(Ruleset);
        });
  };
}

inline ThunkAction<TArray<FBridgeRule>, FSDKState> getBridgeRulesThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FBridgeRule>> {
    return APISlice::Endpoints::getBridgeRules()(Dispatch, GetState);
  };
}

inline ThunkAction<TArray<FDirectiveRuleSet>, FSDKState> listRulesetsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FDirectiveRuleSet>> {
    return func::AsyncChain::then<TArray<FDirectiveRuleSet>,
                                  TArray<FDirectiveRuleSet>>(
        APISlice::Endpoints::getRulesets()(Dispatch, GetState),
        [Dispatch](const TArray<FDirectiveRuleSet> &Rulesets) {
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Rulesets);
        });
  };
}

inline ThunkAction<TArray<FString>, FSDKState> listRulePresetsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FString>> {
    return func::AsyncChain::then<TArray<FString>, TArray<FString>>(
        APISlice::Endpoints::getRulePresets()(Dispatch, GetState),
        [Dispatch](const TArray<FString> &PresetIds) {
          Dispatch(BridgeSlice::Actions::SetAvailablePresetIds(PresetIds));
          return detail::ResolveAsync(PresetIds);
        });
  };
}

inline ThunkAction<FDirectiveRuleSet, FSDKState>
registerRulesetThunk(const FDirectiveRuleSet &Ruleset) {
  return [Ruleset](std::function<AnyAction(const AnyAction &)> Dispatch,
                   std::function<FSDKState()> GetState)
             -> func::AsyncResult<FDirectiveRuleSet> {
    return func::AsyncChain::then<FDirectiveRuleSet, FDirectiveRuleSet>(
        APISlice::Endpoints::postRuleRegister(Ruleset)(Dispatch, GetState),
        [Dispatch, GetState](const FDirectiveRuleSet &Registered) {
          TArray<FDirectiveRuleSet> Rulesets = GetState().Bridge.AvailableRulesets;
          bool bReplaced = false;
          for (int32 Index = 0; Index < Rulesets.Num(); ++Index) {
            if (Rulesets[Index].Id == Registered.Id) {
              Rulesets[Index] = Registered;
              bReplaced = true;
              break;
            }
          }
          if (!bReplaced) {
            Rulesets.Add(Registered);
          }
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Registered);
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
deleteRulesetThunk(const FString &RulesetId) {
  return [RulesetId](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncChain::then<rtk::FEmptyPayload, rtk::FEmptyPayload>(
        APISlice::Endpoints::deleteRule(RulesetId)(Dispatch, GetState),
        [Dispatch, GetState, RulesetId](const rtk::FEmptyPayload &Payload) {
          TArray<FDirectiveRuleSet> Rulesets = GetState().Bridge.AvailableRulesets;
          Rulesets.RemoveAll([RulesetId](const FDirectiveRuleSet &Ruleset) {
            return Ruleset.Id == RulesetId || Ruleset.RulesetId == RulesetId;
          });
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Payload);
        });
  };
}

} // namespace rtk
