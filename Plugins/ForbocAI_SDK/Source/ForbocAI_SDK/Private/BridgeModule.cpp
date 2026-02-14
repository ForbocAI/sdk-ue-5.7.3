#include "BridgeModule.h"

namespace BridgeRules {
// ==========================================
// PURE VALIDATION FUNCTIONS
// ==========================================

FValidationResult ValidateMovement(const FAgentAction &Action,
                                   const FBridgeValidationContext &Context) {
  // Simple example: Check if coordinates exist in JSON payload (simulated)
  if (Action.PayloadJson.Contains(TEXT("x")) &&
      Action.PayloadJson.Contains(TEXT("y"))) {
    return FValidationResult(true, TEXT("Valid coordinates"));
  }

  // Fail
  FAgentAction Corrected = Action;
  Corrected.Type = TEXT("IDLE");
  Corrected.Reason = TEXT("Invalid coordinates");

  return FValidationResult(false, TEXT("Missing x,y in payload"));
}

FValidationResult ValidateAttack(const FAgentAction &Action,
                                 const FBridgeValidationContext &Context) {
  if (Action.Target.IsEmpty()) {
    return FValidationResult(false, TEXT("Missing target"));
  }
  return FValidationResult(true, TEXT("Target specified"));
}
} // namespace BridgeRules

TArray<FValidationRule> BridgeOps::CreateDefaultRules() {
  TArray<FValidationRule> Rules;

  // Movement Rule
  Rules.Add(FValidationRule(TEXT("core.movement"), TEXT("Movement Validation"),
                            {TEXT("MOVE"), TEXT("move")},
                            BridgeRules::ValidateMovement));

  // Attack Rule
  Rules.Add(FValidationRule(TEXT("core.attack"), TEXT("Attack Validation"),
                            {TEXT("ATTACK"), TEXT("attack")},
                            BridgeRules::ValidateAttack));

  return Rules;
}

FValidationResult BridgeOps::Validate(const FAgentAction &Action,
                                      const TArray<FValidationRule> &Rules,
                                      const FBridgeValidationContext &Context) {
  for (const FValidationRule &Rule : Rules) {
    // Check if rule applies to this action type
    if (Rule.ActionTypes.Contains(Action.Type)) {
      // Execute the wrapped std::function
      FValidationResult Result = Rule.Validator(Action, Context);

      if (!Result.bValid) {
        return Result; // Stop at first failure
      }
    }
  }

  return FValidationResult(true, TEXT("All rules passed"));
}
