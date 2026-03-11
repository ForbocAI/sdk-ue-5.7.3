#pragma once

#include "Bridge/BridgeModule.h"
#include "Bridge/BridgeSlice.h"
#include "Core/ThunkDetail.h"
#include "Errors.h"
#include "RuntimeConfig.h"

namespace BridgeHelpers {
/** Pure local validation. Implemented in BridgeModule.cpp. */
FValidationResult RunLocalBridgeValidation(const FAgentAction &Action,
                                          const TArray<FValidationRule> &Rules,
                                          const FBridgeRuleContext &Context);
} // namespace BridgeHelpers

namespace rtk {

// ---------------------------------------------------------------------------
// Bridge thunks (mirrors TS bridgeSlice.ts)
// ---------------------------------------------------------------------------

/** Local rule-based validation (no API). Dispatches slice actions; store-visible. */
inline ThunkAction<FValidationResult, FStoreState>
localValidateBridgeThunk(const FAgentAction &Action,
                         const TArray<FValidationRule> &Rules,
                         const FBridgeRuleContext &Context) {
  return [Action, Rules, Context](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FStoreState()> GetState)
             -> func::AsyncResult<FValidationResult> {
    Dispatch(BridgeSlice::Actions::BridgeValidationPending());

    FValidationResult Result =
        BridgeHelpers::RunLocalBridgeValidation(Action, Rules, Context);

    if (Result.bValid) {
      Dispatch(BridgeSlice::Actions::BridgeValidationSuccess(Result));
    } else {
      Dispatch(BridgeSlice::Actions::BridgeValidationFailure(Result.Reason));
    }
    return detail::ResolveAsync(Result);
  };
}

inline ThunkAction<FValidationResult, FStoreState>
validateBridgeThunk(const FAgentAction &Action,
                    const FBridgeValidationContext &Context,
                    const FString &NpcId = TEXT("")) {
  return [Action, Context, NpcId](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FStoreState()> GetState)
             -> func::AsyncResult<FValidationResult> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FValidationResult>(ApiKeyError.value);
    }

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

inline ThunkAction<FDirectiveRuleSet, FStoreState>
loadBridgePresetThunk(const FString &PresetName) {
  return [PresetName](std::function<AnyAction(const AnyAction &)> Dispatch,
                      std::function<FStoreState()> GetState)
             -> func::AsyncResult<FDirectiveRuleSet> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FDirectiveRuleSet>(ApiKeyError.value);
    }

    return func::AsyncChain::then<FDirectiveRuleSet, FDirectiveRuleSet>(
        APISlice::Endpoints::postBridgePreset(PresetName)(Dispatch, GetState),
        [Dispatch, PresetName](const FDirectiveRuleSet &Ruleset) {
          FDirectiveRuleSet ActiveRuleset = Ruleset;
          if (ActiveRuleset.Id.IsEmpty()) {
            ActiveRuleset.Id = PresetName;
          }
          Dispatch(BridgeSlice::Actions::AddActivePreset(ActiveRuleset));
          return detail::ResolveAsync(Ruleset);
        });
  };
}

inline ThunkAction<TArray<FBridgeRule>, FStoreState> getBridgeRulesThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FStoreState()> GetState)
             -> func::AsyncResult<TArray<FBridgeRule>> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<TArray<FBridgeRule>>(ApiKeyError.value);
    }
    return APISlice::Endpoints::getBridgeRules()(Dispatch, GetState);
  };
}

inline ThunkAction<TArray<FDirectiveRuleSet>, FStoreState> listRulesetsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FStoreState()> GetState)
             -> func::AsyncResult<TArray<FDirectiveRuleSet>> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<TArray<FDirectiveRuleSet>>(ApiKeyError.value);
    }
    return func::AsyncChain::then<TArray<FDirectiveRuleSet>,
                                  TArray<FDirectiveRuleSet>>(
        APISlice::Endpoints::getRulesets()(Dispatch, GetState),
        [Dispatch](const TArray<FDirectiveRuleSet> &Rulesets) {
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Rulesets);
        });
  };
}

inline ThunkAction<TArray<FString>, FStoreState> listRulePresetsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FStoreState()> GetState)
             -> func::AsyncResult<TArray<FString>> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<TArray<FString>>(ApiKeyError.value);
    }
    return func::AsyncChain::then<TArray<FString>, TArray<FString>>(
        APISlice::Endpoints::getRulePresets()(Dispatch, GetState),
        [Dispatch](const TArray<FString> &PresetIds) {
          Dispatch(BridgeSlice::Actions::SetAvailablePresetIds(PresetIds));
          return detail::ResolveAsync(PresetIds);
        });
  };
}

inline ThunkAction<FDirectiveRuleSet, FStoreState>
registerRulesetThunk(const FDirectiveRuleSet &Ruleset) {
  return [Ruleset](std::function<AnyAction(const AnyAction &)> Dispatch,
                   std::function<FStoreState()> GetState)
             -> func::AsyncResult<FDirectiveRuleSet> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FDirectiveRuleSet>(ApiKeyError.value);
    }
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

inline ThunkAction<rtk::FEmptyPayload, FStoreState>
deleteRulesetThunk(const FString &RulesetId) {
  return [RulesetId](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FStoreState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<rtk::FEmptyPayload>(ApiKeyError.value);
    }
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
