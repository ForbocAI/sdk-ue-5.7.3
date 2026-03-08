#include "Ghost/GhostModule.h"
#include "NPC/NPCModule.h"
#include "Core/functional_core.hpp"
#include "Ghost/GhostModuleInternal.h"
#include "Ghost/GhostSlice.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"

// ==========================================================
// Ghost Module Implementation — Strict FP (Reduced)
// ==========================================================

FGhost GhostOps::Create(const FGhostConfig &Config) {
  FGhost Ghost;
  Ghost.Config = Config;
  Ghost.bInitialized = true;
  return Ghost;
}

// Single test implementation (Async)
GhostTypes::GhostTestRunResult GhostOps::RunTest(const FGhost &Ghost,
                                                 const FString &Scenario) {
  return GhostTypes::AsyncResult<FGhostTestResult>::create(
      [Ghost, Scenario](std::function<void(FGhostTestResult)> resolve,
                        std::function<void(std::string)> reject) {
        if (!Ghost.bInitialized) {
          reject("Ghost not initialized");
          return;
        }

        if (Scenario.IsEmpty()) {
          reject("Scenario cannot be empty");
          return;
        }

        auto SDKStore = ConfigureSDKStore();
        SDKStore.dispatch(
            GhostSlice::Actions::GhostSessionStarted(Scenario, TEXT("running")));

        // Create agent from configuration
        FAgent Agent = Ghost.Config.Agent;

        // Run the scenario test via Internal helper
        GhostInternal::RunScenarioTest(Agent, Scenario)
            .then([resolve, SDKStore, Scenario](FGhostTestResult Result) {
              FGhostTestReport Report;
              Report.Results.Add(Result);
              Report.TotalTests = 1;
              Report.PassedTests = Result.bPassed ? 1 : 0;
              Report.FailedTests = Result.bPassed ? 0 : 1;
              Report.SuccessRate = Result.bPassed ? 1.0f : 0.0f;
              Report.Summary = Result.bPassed ? TEXT("Passed") : TEXT("Failed");
              SDKStore.dispatch(
                  GhostSlice::Actions::GhostSessionCompleted(Report));
              resolve(Result);
            })
            .catch_([reject, SDKStore, Scenario](std::string Error) {
              FGhostTestResult Failure;
              Failure.Scenario = Scenario;
              Failure.bPassed = false;
              Failure.ErrorMessage = FString(Error.c_str());
              SDKStore.dispatch(GhostSlice::Actions::GhostSessionFailed(
                  Scenario, Failure.ErrorMessage));
              reject(Error);
            });
      });
}

// All tests implementation (Async)
GhostTypes::GhostTestRunAllResult GhostOps::RunAllTests(const FGhost &Ghost) {
  return GhostTypes::AsyncResult<FGhostTestReport>::create(
      [Ghost](std::function<void(FGhostTestReport)> resolve,
              std::function<void(std::string)> reject) {
        auto Report = MakeShared<FGhostTestReport>();
        Report->Config = Ghost.Config;

        GhostInternal::RunTestsSequentially(
            Ghost, Report, 0,
            [resolve](FGhostTestReport FinalReport) {
              auto SDKStore = ConfigureSDKStore();
              SDKStore.dispatch(
                  GhostSlice::Actions::GhostSessionCompleted(FinalReport));
              resolve(FinalReport);
            },
            [reject](std::string Error) {
              auto SDKStore = ConfigureSDKStore();
              SDKStore.dispatch(GhostSlice::Actions::GhostSessionFailed(
                  TEXT("ghost-run"), FString(Error.c_str())));
              reject(Error);
            });
      });
}

// Configuration validation implementation
GhostTypes::GhostValidationResult
GhostOps::ValidateConfig(const FGhostConfig &Config) {
  return GhostInternal::ValidateTestConfig(Config);
}

// Summary generation implementation
GhostTypes::Either<FString, FString>
GhostOps::GenerateSummary(const FGhostTestReport &Report) {
  return GhostInternal::GenerateTestSummary(Report);
}

// JSON export implementation
GhostTypes::Either<FString, FString>
GhostOps::ExportToJson(const FGhostTestReport &Report) {
  return GhostInternal::ExportResultsToJson(Report);
}

// CSV export implementation
GhostTypes::Either<FString, FString>
GhostOps::ExportToCsv(const FGhostTestReport &Report) {
  return GhostInternal::ExportResultsToCsv(Report);
}
