#include "Bridge/BridgeSlice.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

using namespace rtk;
using namespace BridgeSlice;

// ---------------------------------------------------------------------------
// Test: BridgeValidationPending / Success / Failure lifecycle
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBridgeSliceValidationLifecycleTest,
    "ForbocAI.Slices.Bridge.ValidationLifecycle",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FBridgeSliceValidationLifecycleTest::RunTest(const FString &Parameters) {
  Slice<FBridgeSliceState> BSlice = CreateBridgeSlice();
  FBridgeSliceState State;

  // Initial
  TestEqual("Initial status idle", State.Status, FString(TEXT("idle")));
  TestFalse("No validation yet", State.bHasLastValidation);

  // Pending
  State = BSlice.Reducer(State, BridgeSlice::Actions::BridgeValidationPending());
  TestEqual("Status validating", State.Status, FString(TEXT("validating")));
  TestTrue("Error cleared", State.Error.IsEmpty());

  // Success
  FValidationResult Result;
  Result.bValid = true;
  State = BSlice.Reducer(State, BridgeSlice::Actions::BridgeValidationSuccess(Result));
  TestEqual("Status idle after success", State.Status, FString(TEXT("idle")));
  TestTrue("Has last validation", State.bHasLastValidation);
  TestTrue("Validation is valid", State.LastValidation.bValid);

  return true;
}

// ---------------------------------------------------------------------------
// Test: BridgeValidationFailure sets error state
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeSliceValidationFailTest,
                                 "ForbocAI.Slices.Bridge.ValidationFailure",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FBridgeSliceValidationFailTest::RunTest(const FString &Parameters) {
  Slice<FBridgeSliceState> BSlice = CreateBridgeSlice();
  FBridgeSliceState State;

  State = BSlice.Reducer(State, BridgeSlice::Actions::BridgeValidationPending());
  State = BSlice.Reducer(
      State, BridgeSlice::Actions::BridgeValidationFailure(TEXT("Rule violation")));

  TestEqual("Status error", State.Status, FString(TEXT("error")));
  TestEqual("Error message", State.Error, FString(TEXT("Rule violation")));
  TestTrue("Has last validation", State.bHasLastValidation);

  return true;
}

// ---------------------------------------------------------------------------
// Test: AddActivePreset and SetActivePresets
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeSlicePresetsTest,
                                 "ForbocAI.Slices.Bridge.Presets",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FBridgeSlicePresetsTest::RunTest(const FString &Parameters) {
  Slice<FBridgeSliceState> BSlice = CreateBridgeSlice();
  FBridgeSliceState State;

  // Add preset
  FDirectiveRuleSet FirstPreset;
  FirstPreset.Id = TEXT("rpg_default");
  State = BSlice.Reducer(State, BridgeSlice::Actions::AddActivePreset(FirstPreset));
  TestEqual("One active preset", State.ActivePresets.Num(), 1);
  TestEqual("Preset is rpg_default", State.ActivePresets[0].Id,
            FString(TEXT("rpg_default")));

  // Duplicate add should not duplicate
  State = BSlice.Reducer(State, BridgeSlice::Actions::AddActivePreset(FirstPreset));
  TestEqual("Still one preset (no dupe)", State.ActivePresets.Num(), 1);

  // Add another
  FDirectiveRuleSet SecondPreset;
  SecondPreset.Id = TEXT("combat");
  State = BSlice.Reducer(State, BridgeSlice::Actions::AddActivePreset(SecondPreset));
  TestEqual("Two presets", State.ActivePresets.Num(), 2);

  // SetActivePresets replaces all
  TArray<FDirectiveRuleSet> NewPresets;
  FDirectiveRuleSet StealthPreset;
  StealthPreset.Id = TEXT("stealth");
  NewPresets.Add(StealthPreset);
  State = BSlice.Reducer(State, BridgeSlice::Actions::SetActivePresets(NewPresets));
  TestEqual("Replaced to one", State.ActivePresets.Num(), 1);
  TestEqual("Preset is stealth", State.ActivePresets[0].Id,
            FString(TEXT("stealth")));

  return true;
}

// ---------------------------------------------------------------------------
// Test: SetAvailableRulesets
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeSliceRulesetsTest,
                                 "ForbocAI.Slices.Bridge.Rulesets",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FBridgeSliceRulesetsTest::RunTest(const FString &Parameters) {
  Slice<FBridgeSliceState> BSlice = CreateBridgeSlice();
  FBridgeSliceState State;

  TArray<FDirectiveRuleSet> Rulesets;
  FDirectiveRuleSet Rs;
  Rs.Id = TEXT("rs_1");
  Rs.RulesetId = TEXT("Default Rules");
  Rulesets.Add(Rs);

  State = BSlice.Reducer(State, BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
  TestEqual("One ruleset available", State.AvailableRulesets.Num(), 1);
  TestEqual("Ruleset id", State.AvailableRulesets[0].Id,
            FString(TEXT("rs_1")));

  // Clear resets everything
  State = BSlice.Reducer(State, BridgeSlice::Actions::ClearBridgeValidation());
  TestEqual("Status reset", State.Status, FString(TEXT("idle")));
  TestFalse("No last validation", State.bHasLastValidation);
  TestEqual("Presets cleared", State.ActivePresets.Num(), 0);

  return true;
}
