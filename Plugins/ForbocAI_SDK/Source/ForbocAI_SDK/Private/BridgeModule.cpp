#include "BridgeModule.h"

// ==========================================
// PURE VALIDATION FUNCTIONS
// ==========================================

namespace BridgeRules {

FValidationResult ValidateMovement(const FAgentAction &Action,
                                   const FBridgeValidationContext &Context) {
  if (Action.PayloadJson.Contains(TEXT("x")) &&
      Action.PayloadJson.Contains(TEXT("y"))) {
    return TypeFactory::Valid(TEXT("Valid coordinates"));
  }

  return TypeFactory::Invalid(TEXT("Missing x,y in payload"));
}

FValidationResult ValidateAttack(const FAgentAction &Action,
                                 const FBridgeValidationContext &Context) {
  if (Action.Target.IsEmpty()) {
    return TypeFactory::Invalid(TEXT("Missing target"));
  }
  return TypeFactory::Valid(TEXT("Target specified"));
}

} // namespace BridgeRules

// ==========================================
// BRIDGE OPERATIONS — Free functions
// ==========================================

TArray<FValidationRule> BridgeOps::CreateDefaultRules() {
  TArray<FValidationRule> Rules;

  Rules.Add(BridgeFactory::CreateRule(
      TEXT("core.movement"), TEXT("Movement Validation"),
      {TEXT("MOVE"), TEXT("move")}, BridgeRules::ValidateMovement));

  Rules.Add(BridgeFactory::CreateRule(
      TEXT("core.attack"), TEXT("Attack Validation"),
      {TEXT("ATTACK"), TEXT("attack")}, BridgeRules::ValidateAttack));

  return Rules;
}

FValidationResult BridgeOps::Validate(const FAgentAction &Action,
                                      const TArray<FValidationRule> &Rules,
                                      const FBridgeValidationContext &Context) {
  for (const FValidationRule &Rule : Rules) {
    if (Rule.ActionTypes.Contains(Action.Type)) {
      const FValidationResult Result = Rule.Validator(Action, Context);

      if (!Result.bValid) {
        return Result;
      }
    }
  }

  return TypeFactory::Valid(TEXT("All rules passed"));
}
