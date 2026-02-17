#include "BridgeModule.h"
#include "AgentModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Serialization/JsonSerializer.h"
#include "Core/functional_core.hpp"

// ==========================================
// PURE VALIDATION FUNCTIONS
// ==========================================

namespace BridgeRules {

BridgeTypes::ValidationResult ValidateMovement(const FAgentAction &Action,
                                   const FBridgeValidationContext &Context) {
  if (Action.PayloadJson.Contains(TEXT("x")) &&
      Action.PayloadJson.Contains(TEXT("y"))) {
    return BridgeTypes::make_right(TypeFactory::Valid(TEXT("Valid coordinates")));
  }

  return BridgeTypes::make_left(TEXT("Missing x,y in payload"));
}

BridgeTypes::ValidationResult ValidateAttack(const FAgentAction &Action,
                                 const FBridgeValidationContext &Context) {
  if (Action.Target.IsEmpty()) {
    return BridgeTypes::make_left(TEXT("Missing target"));
  }
  return BridgeTypes::make_right(TypeFactory::Valid(TEXT("Target specified")));
}

} // namespace BridgeRules

// ==========================================
// BRIDGE OPERATIONS — Free functions
// ==========================================

TArray<FValidationRule> BridgeOps::CreateDefaultRules() {
  // Game-Agnostic: No default rules are enforced by the protocol.
  // Games must explicitly register rules or use presets.
  return TArray<FValidationRule>();
}

BridgeTypes::ValidationResult BridgeOps::Validate(const FAgentAction &Action,
                                      const TArray<FValidationRule> &Rules,
                                      const FBridgeValidationContext &Context) {
  // Use functional validation pipeline
  auto validationPipeline = BridgeHelpers::bridgeValidationPipeline();
  auto result = validationPipeline.run(Action);
  
  if (result.isLeft) {
      return BridgeTypes::make_left(result.left);
  }
  
  FAgentAction validatedAction = result.right;
  
  for (const FValidationRule &Rule : Rules) {
    if (Rule.ActionTypes.Contains(validatedAction.Type)) {
      const FValidationResult Result = Rule.Validator(validatedAction, Context);

      if (!Result.bValid) {
        return BridgeTypes::make_left(Result.Reason);
      }
    }
  }

  return BridgeTypes::make_right(TypeFactory::Valid(TEXT("All rules passed")));
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

void BridgeOps::RegisterRule(const FValidationRule &Rule,
                             const FString &ApiUrl) {
  if (ApiUrl.IsEmpty())
    return;

  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
      FHttpModule::Get().CreateRequest();
  Request->SetVerb(TEXT("POST"));
  Request->SetURL(ApiUrl + TEXT("/rules/register"));
  Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

  FString Content = FString::Printf(TEXT("{\"id\":\"%s\",\"name\":\"%s\"}"),
                                    *Rule.Id, *Rule.Name);
  Request->SetContentAsString(Content);
  Request->ProcessRequest();
}

// Functional helper implementations
namespace BridgeHelpers {
    // Implementation of lazy HTTP request creation
    BridgeTypes::Lazy<IHttpRequest*> createLazyHttpRequest(const FString& url) {
        return func::lazy([url]() -> IHttpRequest* {
            FHttpModule* HttpModule = &FHttpModule::Get();
            IHttpRequest* Request = HttpModule->CreateRequest();
            Request->SetURL(url);
            return Request;
        });
    }
    
    // Implementation of bridge validation pipeline
    BridgeTypes::ValidationPipeline<FAgentAction> bridgeValidationPipeline() {
        return func::validationPipeline<FAgentAction>()
            .add([](const FAgentAction& action) -> BridgeTypes::Either<FString, FAgentAction> {
                if (action.Type.IsEmpty()) {
                    return BridgeTypes::make_left(FString(TEXT("Action type cannot be empty")));
                }
                return BridgeTypes::make_right(action);
            })
            .add([](const FAgentAction& action) -> BridgeTypes::Either<FString, FAgentAction> {
                if (action.Target.IsEmpty()) {
                    return BridgeTypes::make_left(FString(TEXT("Action target cannot be empty")));
                }
                return BridgeTypes::make_right(action);
            });
    }
    
    // Implementation of bridge processing pipeline
    BridgeTypes::Pipeline<FAgentAction> bridgeProcessingPipeline(const FAgentAction& action) {
        return func::pipe(action);
    }
    
    // Implementation of curried bridge validation
    BridgeTypes::Curried<2, std::function<BridgeTypes::ValidationResult(FAgentAction, FBridgeValidationContext)>> curriedBridgeValidation() {
        return func::curry<2>([](FAgentAction action, FBridgeValidationContext context) -> BridgeTypes::ValidationResult {
            try {
                FValidationResult result = BridgeOps::Validate(action, TArray<FValidationRule>(), context);
                return BridgeTypes::make_right(FString(), result);
            } catch (const std::exception& e) {
                return BridgeTypes::make_left(FString(e.what()));
            }
        });
    }
    
    // Implementation of rule registration pipeline
    BridgeTypes::Pipeline<FValidationRule> ruleRegistrationPipeline(const FValidationRule& rule) {
        return func::pipe(rule);
    }
}