#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include <functional>

/**
 * Context for validation.
 */
struct FBridgeValidationContext {
  const FAgentState *AgentState;
  const TMap<FString, FString> WorldState;

  FBridgeValidationContext(const FAgentState *InState = nullptr,
                     TMap<FString, FString> InWorld = {})
      : AgentState(InState), WorldState(InWorld) {}
};

/**
 * Validation Rule - Wraps a pure function.
 */
struct FValidationRule {
  FString Id;
  FString Name;
  TArray<FString> ActionTypes;

  // The core logic: (Action, Context) -> Result
  std::function<FValidationResult(const FAgentAction &,
                                  const FBridgeValidationContext &)>
      Validator;

  FValidationRule(FString InId, FString InName, TArray<FString> InTypes,
                  std::function<FValidationResult(const FAgentAction &,
                                                  const FBridgeValidationContext &)>
                      InValidator)
      : Id(InId), Name(InName), ActionTypes(InTypes), Validator(InValidator) {}
};

/**
 * Bridge Operations - Neuro-Symbolic Validation.
 */
namespace BridgeOps {
/**
 * Creates the default set of rules.
 */
FORBOCAI_SDK_API TArray<FValidationRule> CreateDefaultRules();

/**
 * Validates an action against a set of rules.
 * Pure function.
 */
FORBOCAI_SDK_API FValidationResult
Validate(const FAgentAction &Action, const TArray<FValidationRule> &Rules,
         const FBridgeValidationContext &Context);
} // namespace BridgeOps
