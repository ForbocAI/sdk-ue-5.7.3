#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
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
  /** Pointer to the Agent's state (optional). */
  const FAgentState *AgentState;
  /** Key-value map of the current world state. */
  const TMap<FString, FString> WorldState;
};

/**
 * Validation Rule — Pure data wrapping a function.
 * The Validator field holds a pure function: (Action, Context) -> Result
 */
struct FValidationRule {
  /** Unique ID for the rule. */
  FString Id;
  /** Human-readable name of the rule. */
  FString Name;
  /** List of action types this rule applies to. */
  TArray<FString> ActionTypes;
  /** The validation function to execute. */
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
 * @param State Optional pointer to agent state.
 * @param World Optional world state map.
 * @return A new FBridgeValidationContext.
 */
inline FBridgeValidationContext
CreateContext(const FAgentState *State = nullptr,
              TMap<FString, FString> World = {}) {
  return FBridgeValidationContext{State, MoveTemp(World)};
}

/**
 * Factory: Creates a validation rule.
 * @param Id Unique ID.
 * @param Name Human-readable name.
 * @param Types Action types this rule applies to.
 * @param InValidator The validation function.
 * @return A new FValidationRule.
 */
inline FValidationRule
CreateRule(FString Id, FString Name, TArray<FString> Types,
           std::function<FValidationResult(const FAgentAction &,
                                           const FBridgeValidationContext &)>
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
 * @return An array of default validation rules.
 */
FORBOCAI_SDK_API TArray<FValidationRule> CreateDefaultRules();

/**
 * Creates a preset of RPG-specific validation rules.
 * @return An array of RPG validation rules.
 */
FORBOCAI_SDK_API TArray<FValidationRule> CreateRPGRules();

/**
 * Validates an action against a set of rules.
 * Pure function: (Action, Rules, Context) -> Result
 * @param Action The action to validate.
 * @param Rules The rules to validate against.
 * @param Context The validation context.
 * @return The result of the validation.
 */
FORBOCAI_SDK_API FValidationResult
Validate(const FAgentAction &Action, const TArray<FValidationRule> &Rules,
         const FBridgeValidationContext &Context);

/**
 * Registers a rule with the API.
 * Fire-and-forget async operation.
 * @param Rule The rule to register.
 * @param ApiUrl The API endpoint.
 */
FORBOCAI_SDK_API void RegisterRule(const FValidationRule &Rule,
                                   const FString &ApiUrl);

} // namespace BridgeOps
