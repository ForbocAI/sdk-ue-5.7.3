#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "NPC/NPCBaseTypes.h"
#include "BridgeTypes.generated.h"
// clang-format on

/**
 * Validation Result — Immutable data.
 */
USTRUCT(BlueprintType)
struct FValidationResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  bool bValid;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString Reason;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FAgentAction CorrectedAction;

  FValidationResult() : bValid(true) {}
};

/**
 * Validation Context
 */
USTRUCT(BlueprintType)
struct FBridgeValidationContext {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString NpcStateJson;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString WorldStateJson;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString ConstraintsJson;

  FBridgeValidationContext() {}
};

/**
 * Bridge Rule
 */
USTRUCT(BlueprintType)
struct FBridgeRule {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString RuleName;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString RuleDescription;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  TArray<FString> RuleActionTypes;

  FBridgeRule() {}
};

/**
 * Directive Rule Set
 */
USTRUCT(BlueprintType)
struct FDirectiveRuleSet {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FString RulesetId;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  TArray<FBridgeRule> RulesetRules;

  FDirectiveRuleSet() {}
};

USTRUCT(BlueprintType)
struct FBridgeValidateRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
  FAgentAction Action;

  UPROPERTY(BlueprintReadOnly, Category = "Bridge")
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
