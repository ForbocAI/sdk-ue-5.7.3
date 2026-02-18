#include "BridgeModule.h"
#include "Misc/AutomationTest.h"

DEFINE_SPEC(FForbocAIBridgeSpec, "ForbocAI.Bridge.Validation",
            EAutomationTestFlags::ProductFilter |
                EAutomationTestFlags::ApplicationContextMask)

void FForbocAIBridgeSpec::Define() {
  Describe("Action Validation", [this]() {
    It("Should validate a valid MOVE action", [this]() {
      FAgentAction Action;
      Action.Type = "MOVE";
      Action.Payload.Add("x", 10);
      Action.Payload.Add("y", 20);

      FValidationResult Result = FBridgeModule::ValidateAction(Action);

      // Default bridge has no rules, so it should be valid
      TestTrue("Result.bValid", Result.bValid);
    });

    It("Should create valid error messages", [this]() {
      FValidationError Error = FValidationError::Create("Test Error");
      TestEqual("Error Message", Error.Message, TEXT("Test Error"));
    });
  });
}
