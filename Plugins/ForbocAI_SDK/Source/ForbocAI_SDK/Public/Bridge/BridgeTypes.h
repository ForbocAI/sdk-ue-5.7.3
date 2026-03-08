#pragma once

#include "CoreMinimal.h"
#include "NPC/AgentTypes.h"
#include "BridgeTypes.generated.h"

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
struct FBridgeValidationContext {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString NpcStateJson;

  UPROPERTY(BlueprintReadOnly)
  FString WorldStateJson;

  UPROPERTY(BlueprintReadOnly)
  FString ConstraintsJson;

  FBridgeValidationContext() {}
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

USTRUCT(BlueprintType)
struct FBridgeValidateRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly)
  FBridgeValidationContext Context;
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

inline FBridgeValidateRequest
BridgeValidateRequest(const FAgentAction &Action,
                      const FBridgeValidationContext &Context) {
  FBridgeValidateRequest Request;
  Request.Action = Action;
  Request.Context = Context;
  return Request;
}

} // namespace TypeFactory
