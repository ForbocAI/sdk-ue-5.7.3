#pragma once

#include "CoreMinimal.h"
#include "BridgeTypes.generated.h"
#include "NPC/AgentTypes.h"

/**
 * Validation Result — Immutable data.
 */
USTRUCT(BlueprintType)
struct FValidationResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  bool bValid;

  UPROPERTY(BlueprintReadOnly)
  FString Reason;

  UPROPERTY(BlueprintReadOnly)
  FAgentAction CorrectedAction;

  FValidationResult() : bValid(true) {}
};

/**
 * Validation Context
 */
USTRUCT(BlueprintType)
struct FValidationContext {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString NpcStateJson;

  UPROPERTY(BlueprintReadOnly)
  FString WorldStateJson;

  UPROPERTY(BlueprintReadOnly)
  FString ConstraintsJson;

  FValidationContext() {}
};

/**
 * Bridge Rule
 */
USTRUCT(BlueprintType)
struct FBridgeRule {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString RuleName;

  UPROPERTY(BlueprintReadOnly)
  FString RuleDescription;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> RuleActionTypes;

  FBridgeRule() {}
};

/**
 * Directive Rule Set
 */
USTRUCT(BlueprintType)
struct FDirectiveRuleSet {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString RulesetId;

  UPROPERTY(BlueprintReadOnly)
  TArray<FBridgeRule> RulesetRules;

  FDirectiveRuleSet() {}
};

namespace TypeFactory {

inline FValidationResult Valid(FString Reason) {
  FValidationResult R;
  R.bValid = true;
  R.Reason = MoveTemp(Reason);
  return R;
}

inline FValidationResult Invalid(FString Reason) {
  FValidationResult R;
  R.bValid = false;
  R.Reason = MoveTemp(Reason);
  return R;
}

} // namespace TypeFactory
