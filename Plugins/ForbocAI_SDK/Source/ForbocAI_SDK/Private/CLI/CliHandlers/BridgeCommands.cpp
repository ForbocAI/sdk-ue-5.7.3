#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleBridge(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("bridge_validate")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: bridge_validate <actionJson>"));
    }
    FAgentAction Action;
    Action.PayloadJson = Args[0];
    FBridgeValidationContext Context;
    FValidationResult VResult = Ops::ValidateBridge(Store, Action, Context);
    UE_LOG(LogTemp, Display, TEXT("Validation: %s"),
           VResult.bValid ? TEXT("PASS") : TEXT("FAIL"));
    return just(Result::Success("Bridge validation done"));
  }

  if (CommandKey == TEXT("bridge_rules")) {
    TArray<FBridgeRule> Rules = Ops::BridgeRules(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d bridge rules"), Rules.Num());
    return just(Result::Success("Bridge rules listed"));
  }

  if (CommandKey == TEXT("bridge_preset")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: bridge_preset <presetName>"));
    }
    FDirectiveRuleSet Preset = Ops::BridgePreset(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Loaded preset: %s"), *Preset.Id);
    return just(Result::Success("Bridge preset loaded"));
  }

  if (CommandKey == TEXT("rules_list")) {
    TArray<FDirectiveRuleSet> Rulesets = Ops::RulesList(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d rulesets"), Rulesets.Num());
    return just(Result::Success("Rulesets listed"));
  }

  if (CommandKey == TEXT("rules_presets")) {
    TArray<FString> Presets = Ops::RulesPresets(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d presets"), Presets.Num());
    return just(Result::Success("Rule presets listed"));
  }

  if (CommandKey == TEXT("rules_register")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: rules_register <rulesetJson>"));
    }
    FDirectiveRuleSet Ruleset;
    Ruleset.Id = Args[0];
    FDirectiveRuleSet Registered = Ops::RulesRegister(Store, Ruleset);
    UE_LOG(LogTemp, Display, TEXT("Registered ruleset: %s"), *Registered.Id);
    return just(Result::Success("Ruleset registered"));
  }

  if (CommandKey == TEXT("rules_delete")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: rules_delete <rulesetId>"));
    }
    Ops::RulesDelete(Store, Args[0]);
    return just(Result::Success("Ruleset deleted"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
