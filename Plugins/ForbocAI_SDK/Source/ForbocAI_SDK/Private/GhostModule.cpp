#include "GhostModule.h"
#include "AgentModule.h"
#include "Core/functional_core.hpp"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

// ==========================================================
// Ghost Module Implementation — Strict FP
// ==========================================================
// All operations are pure free functions. NO classes,
// NO member functions. Data is immutable.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

// Single test implementation (Async)
GhostTypes::GhostRunTestResult GhostOps::RunTest(const FGhost &Ghost,
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

        // Create agent from configuration
        FAgent Agent = AgentFactory::Create(Ghost.Config.Agent);

        // Run the scenario test via Internal helper
        Internal::RunScenarioTest(Agent, Scenario)
            .then([resolve](FGhostTestResult Result) { resolve(Result); })
            .catch_([reject](std::string Error) { reject(Error); });
      });
}

// All tests implementation (Async)
GhostTypes::GhostRunAllTestsResult GhostOps::RunAllTests(const FGhost &Ghost) {
  return GhostTypes::AsyncResult<FGhostTestReport>::create(
      [Ghost](std::function<void(FGhostTestReport)> resolve,
              std::function<void(std::string)> reject) {
        // Create agent from configuration
        FAgent Agent = AgentFactory::Create(Ghost.Config.Agent);

        // Initialize report
        auto ReportPtr = std::make_shared<FGhostTestReport>();
        ReportPtr->Config = Ghost.Config;
        ReportPtr->TotalTests = Ghost.Config.Scenarios.Num();

        if (ReportPtr->TotalTests == 0) {
          resolve(*ReportPtr);
          return;
        }

        // We need to run tests sequentially or in parallel.
        // For simplicity and stability, we'll run them sequentially using a
        // recursive lambda.

        auto SharedScenarios =
            std::make_shared<TArray<FString>>(Ghost.Config.Scenarios);
        auto SharedIdx = std::make_shared<int32>(0);

        // std::function must be used to allow recursion
        std::function<void()> RunNext;

        RunNext = [Ghost, SharedScenarios, SharedIdx, ReportPtr, resolve,
                   reject,
                   &RunNext]() { // Capture RunNext by reference is tricky with
                                 // std::function in local scope, usually needs
                                 // a shared_ptr wrapper or strict lifetime.
          // Actually, let's use a Y-combinator style or just a shared_ptr
          // wrapper for the lambda to capture itself easily.
        };
      });
  // Re-implementing RunAllTests with proper async recursion structure below
}

namespace Internal {

// Internal async test runner
GhostTypes::AsyncResult<FGhostTestResult>
RunScenarioTest(const FAgent &Agent, const FString &Scenario) {
  return GhostTypes::AsyncResult<FGhostTestResult>::create(
      [Agent, Scenario](std::function<void(FGhostTestResult)> resolve,
                        std::function<void(std::string)> reject) {
        FString TestInput =
            FString::Printf(TEXT("Test scenario: %s"), *Scenario);

        // AgentOps::Process is now async
        AgentOps::Process(Agent, TestInput, TMap<FString, FString>())
            .then([Scenario, resolve](FAgentResponse Response) {
              FGhostTestResult Result;
              Result.Scenario = Scenario;
              Result.ActualResponse = Response.Dialogue;
              Result.Iteration = 1;
              Result.bPassed = !Response.Dialogue.IsEmpty();
              resolve(Result);
            })
            .catch_([reject](std::string Error) {
              reject(Error); // Propagate error from AgentOps
            });
      });
}

// Helper to run all tests sequentially
void RunTestsSequentially(const FGhost &Ghost,
                          TSharedPtr<FGhostTestReport> Report, int32 Index,
                          std::function<void(FGhostTestReport)> OnComplete,
                          std::function<void(std::string)> OnError) {
  if (Index >= Ghost.Config.Scenarios.Num()) {
    // All done, finalize report
    Report->TotalTests = Report->Results.Num();
    Report->SuccessRate = Report->TotalTests > 0
                              ? (float)Report->PassedTests / Report->TotalTests
                              : 0.0f;

    auto summaryResult = Internal::GenerateTestSummary(*Report);
    if (summaryResult.isLeft)
      Report->Summary = summaryResult.left;
    else
      Report->Summary = summaryResult.right;

    OnComplete(*Report);
    return;
  }

  FString Scenario = Ghost.Config.Scenarios[Index];

  GhostOps::RunTest(Ghost, Scenario)
      .then(
          [Ghost, Report, Index, OnComplete, OnError](FGhostTestResult Result) {
            Report->Results.Add(Result);
            if (Result.bPassed)
              Report->PassedTests++;
            else
              Report->FailedTests++;

            // Recursively run next
            RunTestsSequentially(Ghost, Report, Index + 1, OnComplete, OnError);
          })
      .catch_([Ghost, Report, Scenario, Index, OnComplete,
               OnError](std::string Error) {
        // If a single test fails with an infrastructure error, we record it as
        // a failure but continue
        FGhostTestResult Result;
        Result.Scenario = Scenario;
        Result.bPassed = false;
        Result.ErrorMessage = FString(Error.c_str());

        Report->Results.Add(Result);
        Report->FailedTests++;

        // Recursively run next
        RunTestsSequentially(Ghost, Report, Index + 1, OnComplete, OnError);
      });
}

/** Validates the test configuration. */
GhostTypes::Either<FString, FGhostConfig>
ValidateTestConfig(const FGhostConfig &Config) {
  if (Config.Scenarios.Num() == 0) {
    return GhostTypes::make_left(FString(TEXT("No test scenarios provided")));
  }
  if (Config.MaxIterations < 1) {
    return GhostTypes::make_left(FString(TEXT("Max iterations must be >= 1")));
  }
  return GhostTypes::make_right(Config);
}

/** Generates a summary of the test report. */
GhostTypes::Either<FString, FString>
GenerateTestSummary(const FGhostTestReport &Report) {
  FString Summary = FString::Printf(TEXT("Ghost Test Summary for Agent: %s\n"),
                                    *Report.Config.Agent.Id);
  Summary += FString::Printf(TEXT("Total Tests: %d\n"), Report.TotalTests);
  Summary += FString::Printf(TEXT("Passed: %d\n"), Report.PassedTests);
  Summary += FString::Printf(TEXT("Failed: %d\n"), Report.FailedTests);
  Summary += FString::Printf(TEXT("Success Rate: %.1f%%\n"),
                             Report.SuccessRate * 100.0f);

  return GhostTypes::make_right(Summary);
}

/** Exports results to JSON string. */
GhostTypes::Either<FString, FString>
ExportResultsToJson(const FGhostTestReport &Report) {
  // Simple JSON simulation for now
  FString Json = TEXT("{\n  \"agent\": \"") + Report.Config.Agent.Id +
                 TEXT("\",\n  \"results\": [\n");
  for (int32 i = 0; i < Report.Results.Num(); ++i) {
    const auto &Res = Report.Results[i];
    Json += FString::Printf(
        TEXT("    { \"scenario\": \"%s\", \"passed\": %s }"), *Res.Scenario,
        Res.bPassed ? TEXT("true") : TEXT("false"));
    if (i < Report.Results.Num() - 1)
      Json += TEXT(",");
    Json += TEXT("\n");
  }
  Json += TEXT("  ]\n}");
  return GhostTypes::make_right(Json);
}

/** Exports results to CSV string. */
GhostTypes::Either<FString, FString>
ExportResultsToCsv(const FGhostTestReport &Report) {
  FString Csv = TEXT("Scenario,Passed,ActualResponse,ErrorMessage\n");
  for (const auto &Res : Report.Results) {
    Csv +=
        FString::Printf(TEXT("\"%s\",%s,\"%s\",\"%s\"\n"),
                        *Res.Scenario.Replace(TEXT("\""), TEXT("\"\"")),
                        Res.bPassed ? TEXT("True") : TEXT("False"),
                        *Res.ActualResponse.Replace(TEXT("\""), TEXT("\"\"")),
                        *Res.ErrorMessage.Replace(TEXT("\""), TEXT("\"\"")));
  }
  return GhostTypes::make_right(Csv);
}

} // namespace Internal

// Re-implementation of RunAllTests using the helper
GhostTypes::GhostRunAllTestsResult GhostOps::RunAllTests(const FGhost &Ghost) {
  return GhostTypes::AsyncResult<FGhostTestReport>::create(
      [Ghost](std::function<void(FGhostTestReport)> resolve,
              std::function<void(std::string)> reject) {
        auto Report = MakeShared<FGhostTestReport>();
        Report->Config = Ghost.Config;

        Internal::RunTestsSequentially(Ghost, Report, 0, resolve, reject);
      });
}

// Configuration validation implementation
GhostTypes::GhostValidationResult
GhostOps::ValidateConfig(const FGhostConfig &Config) {
  return Internal::ValidateTestConfig(Config);
}

// Summary generation implementation
GhostTypes::Either<FString, FString>
GhostOps::GenerateSummary(const FGhostTestReport &Report) {
  return Internal::GenerateTestSummary(Report);
}

// JSON export implementation
GhostTypes::Either<FString, FString>
GhostOps::ExportToJson(const FGhostTestReport &Report) {
  return Internal::ExportResultsToJson(Report);
}

// CSV export implementation
GhostTypes::Either<FString, FString>
GhostOps::ExportToCsv(const FGhostTestReport &Report) {
  return Internal::ExportResultsToCsv(Report);
}

// Functional helper implementations
namespace GhostHelpers {
// Implementation of lazy ghost creation
GhostTypes::Lazy<FGhost> createLazyGhost(const FGhostConfig &config) {
  return func::lazy(
      [config]() -> FGhost { return GhostFactory::Create(config); });
}

// Implementation of ghost config validation pipeline
GhostTypes::ValidationPipeline<FGhostConfig> ghostConfigValidationPipeline() {
  return func::validationPipeline<FGhostConfig>()
      .add([](const FGhostConfig &config)
               -> GhostTypes::Either<FString, FGhostConfig> {
        if (config.Agent.Id.IsEmpty() || config.Agent.Persona.IsEmpty()) {
          return GhostTypes::make_left(
              FString(TEXT("Agent must have valid Id and Persona")));
        }
        return GhostTypes::make_right(config);
      })
      .add([](const FGhostConfig &config)
               -> GhostTypes::Either<FString, FGhostConfig> {
        if (config.Scenarios.Num() == 0) {
          return GhostTypes::make_left(
              FString(TEXT("At least one test scenario must be provided")));
        }
        return GhostTypes::make_right(config);
      })
      .add([](const FGhostConfig &config)
               -> GhostTypes::Either<FString, FGhostConfig> {
        if (config.MaxIterations < 1) {
          return GhostTypes::make_left(
              FString(TEXT("Max iterations must be at least 1")));
        }
        return GhostTypes::make_right(config);
      });
}

// Implementation of ghost test pipeline
GhostTypes::Pipeline<FGhost> ghostTestPipeline(const FGhost &ghost) {
  return func::pipe(ghost);
}

// Implementation of curried ghost creation
GhostTypes::Curried<
    1, std::function<GhostTypes::GhostCreationResult(FGhostConfig)>>
curriedGhostCreation() {
  return func::curry<1>(
      [](FGhostConfig config) -> GhostTypes::GhostCreationResult {
        try {
          FGhost ghost = GhostFactory::Create(config);
          return GhostTypes::make_right(FString(), ghost);
        } catch (const std::exception &e) {
          return GhostTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace GhostHelpers