#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include <functional>

// ==========================================================
// Bridge Module — Strict FP (Neuro-Symbolic Validation)
// ==========================================================
// All types are pure data structs. NO constructors,
// NO member functions. Construction via BridgeFactory
// namespace. Operations via BridgeOps namespace.
// ==========================================================

/**
 * Validation Context — Pure data.
 */
struct FBridgeValidationContext {
  const FAgentState *AgentState;
  const TMap<FString, FString> WorldState;
};

/**
 * Validation Rule — Pure data wrapping a function.
 * The Validator field holds a pure function: (Action, Context) -> Result
 */
struct FValidationRule {
  FString Id;
  FString Name;
  TArray<FString> ActionTypes;
  std::function<FValidationResult(const FAgentAction &,
                                  const FBridgeValidationContext &)>
      Validator;
};

/**
 * Factory functions for Bridge types.
 */
namespace BridgeFactory {

/**
 * Factory: Creates a validation context.
 */
inline FBridgeValidationContext
CreateContext(const FAgentState *State = nullptr,
             TMap<FString, FString> World = {}) {
  return FBridgeValidationContext{State, MoveTemp(World)};
}

/**
 * Factory: Creates a validation rule.
 */
inline FValidationRule
CreateRule(FString Id, FString Name, TArray<FString> Types,
           std::function<FValidationResult(
               const FAgentAction &, const FBridgeValidationContext &)>
               InValidator) {
  FValidationRule R;
  R.Id = MoveTemp(Id);
  R.Name = MoveTemp(Name);
  R.ActionTypes = MoveTemp(Types);
  R.Validator = MoveTemp(InValidator);
  return R;
}

} // namespace BridgeFactory

/**
 * Bridge Operations — stateless free functions.
 */
namespace BridgeOps {

/**
 * Creates the default set of validation rules.
 */
FORBOCAI_SDK_API TArray<FValidationRule> CreateDefaultRules();

/**
 * Validates an action against a set of rules.
 * Pure function: (Action, Rules, Context) -> Result
 */
FORBOCAI_SDK_API FValidationResult
Validate(const FAgentAction &Action, const TArray<FValidationRule> &Rules,
         const FBridgeValidationContext &Context);

} // namespace BridgeOps
