#include "Bridge/BridgeModule.h"
#include "NPC/NPCModule.h"
#include "Bridge/BridgeSlice.h"
#include "Core/functional_core.hpp"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"
#include "Bridge/BridgeThunks.h"

// ==========================================
// PURE VALIDATION FUNCTIONS
// ==========================================

namespace BridgeRules {

FValidationResult ValidateMovement(const FAgentAction &Action,
                                   const FBridgeRuleContext &Context) {
  if (Action.PayloadJson.Contains(TEXT("x")) &&
      Action.PayloadJson.Contains(TEXT("y"))) {
    return TypeFactory::Valid(TEXT("Valid coordinates"));
  }

  return TypeFactory::Invalid(TEXT("Missing x,y in payload"));
}

FValidationResult ValidateAttack(const FAgentAction &Action,
                                 const FBridgeRuleContext &Context) {
  if (Action.Target.IsEmpty()) {
    return TypeFactory::Invalid(TEXT("Missing target"));
  }
  return TypeFactory::Valid(TEXT("Target specified"));
}

} // namespace BridgeRules

namespace BridgeHelpers {
BridgeTypes::ValidationPipeline<FAgentAction, FString>
bridgeValidationPipeline();
}

// ==========================================
// BRIDGE OPERATIONS — Free functions
// ==========================================

TArray<FValidationRule> BridgeOps::CreateDefaultRules() {
  // Game-Agnostic: No default rules are enforced by the protocol.
  // Games must explicitly register rules or use presets.
  return TArray<FValidationRule>();
}

FValidationResult BridgeOps::Validate(const FAgentAction &Action,
                                      const TArray<FValidationRule> &Rules,
                                      const FBridgeRuleContext &Context) {
  // Use functional validation pipeline
  auto validationPipeline = BridgeHelpers::bridgeValidationPipeline();
  auto result = validationPipeline.run(Action);

  auto SDKStore = ConfigureSDKStore();
  SDKStore.dispatch(BridgeSlice::Actions::BridgeValidationPending());

  if (result.isLeft) {
    SDKStore.dispatch(
        BridgeSlice::Actions::BridgeValidationFailure(result.left));
    return TypeFactory::Invalid(result.left);
  }

  FAgentAction validatedAction = result.right;

  for (const FValidationRule &Rule : Rules) {
    if (Rule.ActionTypes.Contains(validatedAction.Type)) {
      const FValidationResult RuleResult =
          Rule.Validator(validatedAction, Context);

      if (!RuleResult.bValid) {
        SDKStore.dispatch(BridgeSlice::Actions::BridgeValidationFailure(
            RuleResult.Reason));
        return RuleResult;
      }
    }
  }

  FValidationResult FinalSuccess = TypeFactory::Valid(TEXT("All rules passed"));
  SDKStore.dispatch(BridgeSlice::Actions::BridgeValidationSuccess(
      FinalSuccess));
  return FinalSuccess;
}

TArray<FValidationRule> BridgeOps::CreateRPGRules() {
  TArray<FValidationRule> Rules;

  Rules.Add(
      BridgeFactory::CreateRule(TEXT("move-val"), TEXT("Movement Validation"),
                                {TEXT("MOVE")}, BridgeRules::ValidateMovement));

  Rules.Add(
      BridgeFactory::CreateRule(TEXT("attack-val"), TEXT("Attack Validation"),
                                {TEXT("ATTACK")}, BridgeRules::ValidateAttack));

  return Rules;
}

BridgeTypes::AsyncResult<FDirectiveRuleSet>
BridgeOps::RegisterRule(const FValidationRule &Rule, const FString & /*ApiUrl*/) {
  // Dispatch through the store — no direct HTTP calls.
  auto Store = ConfigureSDKStore();
  FDirectiveRuleSet Ruleset;
  Ruleset.Id = Rule.Id;
  Ruleset.Name = Rule.Name;
  return Store.dispatch(rtk::registerRulesetThunk(Ruleset));
}

// Functional helper implementations
namespace BridgeHelpers {
// Implementation of lazy HTTP request creation
BridgeTypes::Lazy<TSharedRef<IHttpRequest, ESPMode::ThreadSafe>>
createLazyHttpRequest(const FString &url) {
  return func::lazy([url]() -> TSharedRef<IHttpRequest, ESPMode::ThreadSafe> {
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();
    Request->SetURL(url);
    return Request;
  });
}

// Implementation of bridge validation pipeline
BridgeTypes::ValidationPipeline<FAgentAction, FString>
bridgeValidationPipeline() {
  return func::validationPipeline<FAgentAction, FString>()
      .add([](const FAgentAction &action)
               -> BridgeTypes::Either<FString, FAgentAction> {
        if (action.Type.IsEmpty()) {
          return BridgeTypes::make_left(
              FString(TEXT("Action type cannot be empty")),
              FAgentAction{});
        }
        return BridgeTypes::make_right(FString(), action);
      })
      .add([](const FAgentAction &action)
               -> BridgeTypes::Either<FString, FAgentAction> {
        if (action.Target.IsEmpty()) {
          return BridgeTypes::make_left(
              FString(TEXT("Action target cannot be empty")),
              FAgentAction{});
        }
        return BridgeTypes::make_right(FString(), action);
      });
}

// Implementation of bridge processing pipeline
BridgeTypes::Pipeline<FAgentAction>
bridgeProcessingPipeline(const FAgentAction &action) {
  return func::pipe(action);
}

// Implementation of curried bridge validation
inline auto curriedBridgeValidation()
    -> decltype(func::curry<2>(std::function<BridgeTypes::ValidationResult(
        FAgentAction, FBridgeRuleContext)>())) {
  std::function<BridgeTypes::ValidationResult(FAgentAction, FBridgeRuleContext)>
      Creator =
          [](FAgentAction action,
             FBridgeRuleContext context) -> BridgeTypes::ValidationResult {
        try {
          FValidationResult result =
              BridgeOps::Validate(action, TArray<FValidationRule>(), context);
          return BridgeTypes::make_right(FString(), result);
        } catch (const std::exception &e) {
          return BridgeTypes::make_left(FString(e.what()), FValidationResult{});
        }
      };
  return func::curry<2>(Creator);
}

// Implementation of rule registration pipeline
BridgeTypes::Pipeline<FValidationRule>
ruleRegistrationPipeline(const FValidationRule &rule) {
  return func::pipe(rule);
}
} // namespace BridgeHelpers
