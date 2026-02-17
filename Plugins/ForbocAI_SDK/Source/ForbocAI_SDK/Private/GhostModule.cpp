#include "GhostModule.h"
#include "AgentModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Serialization/JsonSerializer.h"
#include "Core/functional_core.hpp"

// ==========================================================
// Ghost Module Implementation — Strict FP
// ==========================================================
// All operations are pure free functions. NO classes,
// NO member functions. Data is immutable.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

namespace {

namespace Internal {

/**
 * Runs a test scenario against an agent.
 * Pure function: (Agent, Scenario) -> Result
 * @param Agent The agent to test.
 * @param Scenario The scenario to run.
 * @return The test result.
 */
GhostTypes::GhostTestRunResult RunScenarioTest(const FAgent &Agent, const FString &Scenario) {
  try {
    FGhostTestResult Result;
    Result.Scenario = Scenario;
    Result.Iteration = 0;
    
    // Generate a test input based on the scenario
    FString TestInput = FString::Printf(TEXT("Test scenario: %s"), *Scenario);
    
    // Process the input through the agent
    AgentOps::Process(Agent, TestInput, TMap<FString, FString>(), 
                     [&Result](const FAgentResponse &Response) {
      Result.ActualResponse = Response.Dialogue;
      Result.Iteration = 1; // Single iteration for now
      Result.bPassed = !Response.Dialogue.IsEmpty();
    });
    
    return GhostTypes::make_right(FString(), Result);
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

/**
 * Validates test configuration.
 * Pure function: Config -> Result
 * @param Config The configuration to validate.
 * @return The validation result.
 */
GhostTypes::GhostValidationResult ValidateTestConfig(const FGhostConfig &Config) {
  try {
    if (Config.Scenarios.Num() == 0) {
      return GhostTypes::make_left(FString(TEXT("No test scenarios provided")));
    }
    
    if (!Config.Agent.Id.IsEmpty() && !Config.Agent.Persona.IsEmpty()) {
      return GhostTypes::make_right(Config);
    }
    
    return GhostTypes::make_left(FString(TEXT("Invalid agent configuration")));
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

/**
 * Generates a test summary.
 * Pure function: Report -> Summary
 * @param Report The test report.
 * @return The summary string.
 */
GhostTypes::GhostTestRunResult GenerateTestSummary(const FGhostTestReport &Report) {
  try {
    FString Summary = FString::Printf(TEXT("Ghost Test Summary\n"));
    Summary += FString::Printf(TEXT("Total Tests: %d\n"), Report.TotalTests);
    Summary += FString::Printf(TEXT("Passed: %d\n"), Report.PassedTests);
    Summary += FString::Printf(TEXT("Failed: %d\n"), Report.FailedTests);
    Summary += FString::Printf(TEXT("Success Rate: %.1f%%\n"), Report.SuccessRate * 100.0f);
    
    if (Report.FailedTests > 0) {
      Summary += TEXT("\nFailed Scenarios:\n");
      for (const FGhostTestResult &Result : Report.Results) {
        if (!Result.bPassed) {
          Summary += FString::Printf(TEXT("  - %s: %s\n"), *Result.Scenario, *Result.ErrorMessage);
        }
      }
    }
    
    return GhostTypes::make_right(FString(), Summary);
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

/**
 * Exports results to JSON.
 * Pure function: Report -> JSON
 * @param Report The test report.
 * @return The JSON string representation.
 */
GhostTypes::GhostTestRunResult ExportResultsToJson(const FGhostTestReport &Report) {
  try {
    FString JsonString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    
    JsonWriter->WriteObjectStart();
    
    // Write configuration
    JsonWriter->WriteObjectStart(TEXT("config"));
    JsonWriter->WriteValue(TEXT("agentId"), Report.Config.Agent.Id);
    JsonWriter->WriteValue(TEXT("persona"), Report.Config.Agent.Persona);
    JsonWriter->WriteValue(TEXT("maxIterations"), Report.Config.MaxIterations);
    JsonWriter->WriteValue(TEXT("verbose"), Report.Config.bVerbose);
    JsonWriter->WriteArrayStart(TEXT("scenarios"));
    for (const FString &Scenario : Report.Config.Scenarios) {
      JsonWriter->WriteValue(*Scenario);
    }
    JsonWriter->WriteArrayEnd();
    JsonWriter->WriteObjectEnd();
    
    // Write results
    JsonWriter->WriteArrayStart(TEXT("results"));
    for (const FGhostTestResult &Result : Report.Results) {
      JsonWriter->WriteObjectStart();
      JsonWriter->WriteValue(TEXT("scenario"), Result.Scenario);
      JsonWriter->WriteValue(TEXT("passed"), Result.bPassed);
      JsonWriter->WriteValue(TEXT("actualResponse"), Result.ActualResponse);
      JsonWriter->WriteValue(TEXT("expectedResponse"), Result.ExpectedResponse);
      JsonWriter->WriteValue(TEXT("errorMessage"), Result.ErrorMessage);
      JsonWriter->WriteValue(TEXT("iteration"), Result.Iteration);
      JsonWriter->WriteObjectEnd();
    }
    JsonWriter->WriteArrayEnd();
    
    // Write summary
    JsonWriter->WriteObjectStart(TEXT("summary"));
    JsonWriter->WriteValue(TEXT("totalTests"), Report.TotalTests);
    JsonWriter->WriteValue(TEXT("passedTests"), Report.PassedTests);
    JsonWriter->WriteValue(TEXT("failedTests"), Report.FailedTests);
    JsonWriter->WriteValue(TEXT("successRate"), Report.SuccessRate);
    JsonWriter->WriteValue(TEXT("summary"), Report.Summary);
    JsonWriter->WriteObjectEnd();
    
    JsonWriter->WriteObjectEnd();
    JsonWriter->Close();
    
    return GhostTypes::make_right(FString(), JsonString);
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

/**
 * Exports results to CSV.
 * Pure function: Report -> CSV
 * @param Report The test report.
 * @return The CSV string representation.
 */
GhostTypes::GhostTestRunResult ExportResultsToCsv(const FGhostTestReport &Report) {
  try {
    FString CsvString;
    
    // Write header
    CsvString += TEXT("Scenario,Passed,ActualResponse,ExpectedResponse,ErrorMessage,Iteration\n");
    
    // Write results
    for (const FGhostTestResult &Result : Report.Results) {
    CsvString += FString::Printf(TEXT("\"%s\",%s,\"%s\",\"%s\",\"%s\",%d\n"),
      *Result.Scenario,
      Result.bPassed ? TEXT("true") : TEXT("false"),
      *Result.ActualResponse.Replace(TEXT("\""), TEXT("\"\"")),
      *Result.ExpectedResponse.Replace(TEXT("\""), TEXT("\"\"")),
      *Result.ErrorMessage.Replace(TEXT("\""), TEXT("\"\"")),
      Result.Iteration
    );
    }
    
    return GhostTypes::make_right(FString(), CsvString);
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

} // namespace Internal

} // namespace

// Factory function implementation
FGhost GhostFactory::Create(const FGhostConfig &Config) {
  // Use functional validation
  auto validationResult = GhostHelpers::ghostConfigValidationPipeline().run(Config);
  if (validationResult.isLeft) {
      throw std::runtime_error(validationResult.left.c_str());
  }
  
  FGhost ghost;
  ghost.Config = Config;
  ghost.bInitialized = true;
  return ghost;
}

// Single test implementation
GhostTypes::GhostTestRunResult GhostOps::RunTest(const FGhost &Ghost, const FString &Scenario) {
  try {
    if (!Ghost.bInitialized) {
      return GhostTypes::make_left(FString(TEXT("Ghost not initialized")));
    }
    
    if (Scenario.IsEmpty()) {
      return GhostTypes::make_left(FString(TEXT("Scenario cannot be empty")));
    }
    
    // Create agent from configuration
    FAgent Agent = AgentFactory::Create(Ghost.Config.Agent);
    
    // Run the scenario test
    return Internal::RunScenarioTest(Agent, Scenario);
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

// All tests implementation
GhostTypes::GhostTestRunAllResult GhostOps::RunAllTests(const FGhost &Ghost) {
  try {
    // Create agent from configuration
    FAgent Agent = AgentFactory::Create(Ghost.Config.Agent);
    
    // Initialize report
    FGhostTestReport Report;
    Report.Config = Ghost.Config;
    Report.TotalTests = Ghost.Config.Scenarios.Num();
    Report.PassedTests = 0;
    Report.FailedTests = 0;
    Report.SuccessRate = 0.0f;
    
    // Run each scenario
    for (const FString &Scenario : Ghost.Config.Scenarios) {
      auto result = GhostOps::RunTest(Ghost, Scenario);
      if (result.isLeft) {
          FGhostTestResult testResult;
          testResult.Scenario = Scenario;
          testResult.bPassed = false;
          testResult.ErrorMessage = result.left;
          Report.Results.Add(testResult);
          Report.FailedTests++;
      } else {
          Report.Results.Add(result.right);
          if (result.right.bPassed) {
            Report.PassedTests++;
          } else {
            Report.FailedTests++;
          }
      }
    }
    
    // Calculate totals and success rate
    Report.TotalTests = Report.Results.Num();
    Report.SuccessRate = Report.TotalTests > 0 ? (float)Report.PassedTests / Report.TotalTests : 0.0f;
    
    // Generate summary
    auto summaryResult = Internal::GenerateTestSummary(Report);
    if (summaryResult.isLeft) {
        Report.Summary = summaryResult.left;
    } else {
        Report.Summary = summaryResult.right;
    }
    
    return GhostTypes::make_right(FString(), Report);
  } catch (const std::exception& e) {
    return GhostTypes::make_left(FString(e.what()));
  }
}

// Configuration validation implementation
GhostTypes::GhostValidationResult GhostOps::ValidateConfig(const FGhostConfig &Config) {
  return Internal::ValidateTestConfig(Config);
}

// Summary generation implementation
FString GhostOps::GenerateSummary(const FGhostTestReport &Report) {
  auto result = Internal::GenerateTestSummary(Report);
  return result.isLeft ? result.left : result.right;
}

// JSON export implementation
FString GhostOps::ExportToJson(const FGhostTestReport &Report) {
  auto result = Internal::ExportResultsToJson(Report);
  return result.isLeft ? result.left : result.right;
}

// CSV export implementation
FString GhostOps::ExportToCsv(const FGhostTestReport &Report) {
  auto result = Internal::ExportResultsToCsv(Report);
  return result.isLeft ? result.left : result.right;
}

// Functional helper implementations
namespace GhostHelpers {
    // Implementation of lazy ghost creation
    GhostTypes::Lazy<FGhost> createLazyGhost(const FGhostConfig& config) {
        return func::lazy([config]() -> FGhost {
            return GhostFactory::Create(config);
        });
    }
    
    // Implementation of ghost config validation pipeline
    GhostTypes::ValidationPipeline<FGhostConfig> ghostConfigValidationPipeline() {
        return func::validationPipeline<FGhostConfig>()
            .add([](const FGhostConfig& config) -> GhostTypes::Either<FString, FGhostConfig> {
                if (config.Agent.Id.IsEmpty() || config.Agent.Persona.IsEmpty()) {
                    return GhostTypes::make_left(FString(TEXT("Agent must have valid Id and Persona")));
                }
                return GhostTypes::make_right(config);
            })
            .add([](const FGhostConfig& config) -> GhostTypes::Either<FString, FGhostConfig> {
                if (config.Scenarios.Num() == 0) {
                    return GhostTypes::make_left(FString(TEXT("At least one test scenario must be provided")));
                }
                return GhostTypes::make_right(config);
            })
            .add([](const FGhostConfig& config) -> GhostTypes::Either<FString, FGhostConfig> {
                if (config.MaxIterations < 1) {
                    return GhostTypes::make_left(FString(TEXT("Max iterations must be at least 1")));
                }
                return GhostTypes::make_right(config);
            });
    }
    
    // Implementation of ghost test pipeline
    GhostTypes::Pipeline<FGhost> ghostTestPipeline(const FGhost& ghost) {
        return func::pipe(ghost);
    }
    
    // Implementation of curried ghost creation
    GhostTypes::Curried<1, std::function<GhostTypes::GhostCreationResult(FGhostConfig)>> curriedGhostCreation() {
        return func::curry<1>([](FGhostConfig config) -> GhostTypes::GhostCreationResult {
            try {
                FGhost ghost = GhostFactory::Create(config);
                return GhostTypes::make_right(FString(), ghost);
            } catch (const std::exception& e) {
                return GhostTypes::make_left(FString(e.what()));
            }
        });
    }
}