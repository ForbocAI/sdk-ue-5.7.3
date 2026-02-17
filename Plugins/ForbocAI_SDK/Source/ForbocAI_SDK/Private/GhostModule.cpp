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
FGhostTestResult RunScenarioTest(const FAgent &Agent, const FString &Scenario) {
  FGhostTestResult Result;
  Result.Scenario = Scenario;
  Result.Iteration = 0;
  
  // Generate a test input based on the scenario
  FString TestInput = FString::Printf(TEXT("Test scenario: %s"), *Scenario);
  
  // Process the input through the agent
  const FAgentResponse Response = AgentOps::Process(Agent, TestInput, TMap<FString, FString>());
  
  Result.ActualResponse = Response.Dialogue;
  Result.Iteration = 1; // Single iteration for now
  
  // TODO: Implement actual validation logic
  // For now, mark as passed if response is not empty
  Result.bPassed = !Response.Dialogue.IsEmpty();
  
  return Result;
}

/**
 * Validates test configuration.
 * Pure function: Config -> Result
 * @param Config The configuration to validate.
 * @return The validation result.
 */
FValidationResult ValidateTestConfig(const FGhostConfig &Config) {
  if (Config.Scenarios.Num() == 0) {
    return TypeFactory::Invalid(TEXT("No test scenarios provided"));
  }
  
  if (!Config.Agent.Id.IsEmpty() && !Config.Agent.Persona.IsEmpty()) {
    return TypeFactory::Valid(TEXT("Test configuration valid"));
  }
  
  return TypeFactory::Invalid(TEXT("Invalid agent configuration"));
}

/**
 * Generates a test summary.
 * Pure function: Report -> Summary
 * @param Report The test report.
 * @return The summary string.
 */
FString GenerateTestSummary(const FGhostTestReport &Report) {
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
  
  return Summary;
}

/**
 * Exports results to JSON.
 * Pure function: Report -> JSON
 * @param Report The test report.
 * @return The JSON string representation.
 */
FString ExportResultsToJson(const FGhostTestReport &Report) {
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
  
  return JsonString;
}

/**
 * Exports results to CSV.
 * Pure function: Report -> CSV
 * @param Report The test report.
 * @return The CSV string representation.
 */
FString ExportResultsToCsv(const FGhostTestReport &Report) {
  FString CsvString;
  
  // Write header
  CsvString += TEXT("Scenario,Passed,ActualResponse,ExpectedResponse,ErrorMessage,Iteration\n");
  
  // Write results
  for (const FGhostTestResult &Result : Report.Results) {
    CsvString += FString::Printf(TEXT("\"%s\",%s,\"%s\",\"%s\",\"%s\",%d\n"),
      *Result.Scenario,
      Result.bPassed ? TEXT("true") : TEXT("false"),
      *Result.ActualResponse.Replace(TEXT("\"'), TEXT("\"\""),
      *Result.ExpectedResponse.Replace(TEXT("\"'), TEXT("\"\"")),
      *Result.ErrorMessage.Replace(TEXT("\"'), TEXT("\"\"")),
      Result.Iteration
    );
  }
  
  return CsvString;
}

} // namespace Internal

} // namespace

// Factory function implementation
FGhost GhostFactory::Create(const FGhostConfig &Config) {
  FGhost Ghost;
  Ghost.Config = Config;
  Ghost.bInitialized = false;
  
  // Validate configuration
  const FValidationResult Validation = GhostOps::ValidateConfig(Config);
  if (Validation.bValid) {
    Ghost.bInitialized = true;
  }
  
  return Ghost;
}

// Single test implementation
FGhostTestResult GhostOps::RunTest(const FGhost &Ghost, const FString &Scenario) {
  FGhostTestResult Result;
  
  if (!Ghost.bInitialized) {
    Result.bPassed = false;
    Result.ErrorMessage = TEXT("Ghost not initialized");
    return Result;
  }
  
  if (Scenario.IsEmpty()) {
    Result.bPassed = false;
    Result.ErrorMessage = TEXT("Scenario cannot be empty");
    return Result;
  }
  
  // Run the scenario test
  Result = Internal::RunScenarioTest(Ghost.Config.Agent, Scenario);
  
  return Result;
}

// All tests implementation
FGhostTestReport GhostOps::RunAllTests(const FGhost &Ghost) {
  FGhostTestReport Report;
  Report.Config = Ghost.Config;
  
  if (!Ghost.bInitialized) {
    Report.Summary = TEXT("Ghost not initialized");
    return Report;
  }
  
  // Run each scenario
  for (const FString &Scenario : Ghost.Config.Scenarios) {
    FGhostTestResult Result = GhostOps::RunTest(Ghost, Scenario);
    Report.Results.Add(Result);
    
    if (Result.bPassed) {
      Report.PassedTests++;
    } else {
      Report.FailedTests++;
    }
  }
  
  // Calculate totals and success rate
  Report.TotalTests = Report.Results.Num();
  Report.SuccessRate = Report.TotalTests > 0 ? (float)Report.PassedTests / Report.TotalTests : 0.0f;
  Report.Summary = Internal::GenerateTestSummary(Report);
  
  return Report;
}

// Configuration validation implementation
FValidationResult GhostOps::ValidateConfig(const FGhostConfig &Config) {
  return Internal::ValidateTestConfig(Config);
}

// Summary generation implementation
FString GhostOps::GenerateSummary(const FGhostTestReport &Report) {
  return Internal::GenerateTestSummary(Report);
}

// JSON export implementation
FString GhostOps::ExportToJson(const FGhostTestReport &Report) {
  return Internal::ExportResultsToJson(Report);
}

// CSV export implementation
FString GhostOps::ExportToCsv(const FGhostTestReport &Report) {
  return Internal::ExportResultsToCsv(Report);
}