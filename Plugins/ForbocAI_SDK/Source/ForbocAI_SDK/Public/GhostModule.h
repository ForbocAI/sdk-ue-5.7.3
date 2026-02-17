#pragma once

#include "AgentModule.h"
#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "GhostModule.generated.h"

// ==========================================================
// Ghost Module — Automated QA Testing (UE SDK)
// ==========================================================
// Strict functional programming implementation for automated
// testing of agent behavior and responses.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

// Functional Core Type Aliases for Ghost operations
namespace GhostTypes {
    using func::Maybe;
    using func::Either;
    using func::Pipeline;
    using func::Curried;
    using func::Lazy;
    using func::ValidationPipeline;
    using func::ConfigBuilder;
    using func::TestResult;
    using func::AsyncResult;
    
    // Type aliases for Ghost operations
    using GhostCreationResult = Either<FString, FGhost>;
    using GhostTestRunResult = Either<FString, FGhostTestResult>;
    using GhostTestRunAllResult = Either<FString, FGhostTestReport>;
    using GhostValidationResult = Either<FString, FGhostConfig>;
}

// Functional Core Helper Functions for Ghost operations
namespace GhostHelpers {
    // Helper to create a lazy ghost
    inline GhostTypes::Lazy<FGhost> createLazyGhost(const FGhostConfig& config) {
        return func::lazy([config]() -> FGhost {
            return GhostFactory::Create(config);
        });
    }
    
    // Helper to create a validation pipeline for ghost configuration
    inline GhostTypes::ValidationPipeline<FGhostConfig> ghostConfigValidationPipeline() {
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
    
    // Helper to create a pipeline for ghost test execution
    inline GhostTypes::Pipeline<FGhost> ghostTestPipeline(const FGhost& ghost) {
        return func::pipe(ghost);
    }
    
    // Helper to create a curried ghost creation function
    inline GhostTypes::Curried<1, std::function<GhostTypes::GhostCreationResult(FGhostConfig)>> curriedGhostCreation() {
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

// ==========================================================
// Ghost Module — Automated QA Testing (UE SDK)
// ==========================================================
// Strict functional programming implementation for automated
// testing of agent behavior and responses.
// All operations are pure free functions.
// Enhanced with functional core patterns for better
// error handling and composition.
// ==========================================================

/**
 * Ghost Test Configuration — Immutable data.
 */
struct FGhostConfig {
  /** The agent to test. */
  FAgent Agent;
  /** Test scenarios to run. */
  TArray<FString> Scenarios;
  /** Maximum number of test iterations. */
  int32 MaxIterations;
  /** Expected response patterns. */
  TMap<FString, FString> ExpectedResponses;
  /** Whether to run in verbose mode. */
  bool bVerbose;
  /** API URL for the agent. */
  FString ApiUrl;

  FGhostConfig() : MaxIterations(100), bVerbose(false) {}
};

/**
 * Ghost Test Result — Immutable data.
 */
USTRUCT()
struct FGhostTestResult {
  GENERATED_BODY()

  /** The test scenario that was run. */
  UPROPERTY()
  FString Scenario;

  /** Whether the test passed. */
  UPROPERTY()
  bool bPassed;

  /** The actual response from the agent. */
  UPROPERTY()
  FString ActualResponse;

  /** The expected response. */
  UPROPERTY()
  FString ExpectedResponse;

  /** Any error messages encountered. */
  UPROPERTY()
  FString ErrorMessage;

  /** The iteration number when the test completed. */
  UPROPERTY()
  int32 Iteration;

  FGhostTestResult() : bPassed(false), Iteration(0) {}
};

/**
 * Ghost Test Report — Immutable data.
 */
USTRUCT()
struct FGhostTestReport {
  GENERATED_BODY()

  /** The overall test configuration. */
  FGhostConfig Config;

  /** All test results. */
  UPROPERTY()
  TArray<FGhostTestResult> Results;

  /** The total number of tests run. */
  UPROPERTY()
  int32 TotalTests;

  /** The number of tests that passed. */
  UPROPERTY()
  int32 PassedTests;

  /** The number of tests that failed. */
  UPROPERTY()
  int32 FailedTests;

  /** The overall success rate. */
  UPROPERTY()
  float SuccessRate;

  /** Any summary messages. */
  UPROPERTY()
  FString Summary;

  FGhostTestReport()
      : TotalTests(0), PassedTests(0), FailedTests(0), SuccessRate(0.0f) {}
};

/**
 * Ghost Test Engine — Immutable data.
 */
struct FGhost {
  /** The configuration for this test engine. */
  FGhostConfig Config;
  /** Whether this ghost has been initialized. */
  bool bInitialized;

  FGhost() : bInitialized(false) {}
};

/**
 * Ghost Operations — Stateless free functions.
 */
namespace GhostOps {

/**
 * Factory: Creates a Ghost test instance.
 * Pure function: Config -> Ghost
 * @param Config The test configuration.
 * @return A new Ghost test instance.
 */
FORBOCAI_SDK_API FGhost Create(const FGhostConfig &Config);

/**
 * Runs a single test scenario.
 * Pure function: (Ghost, Scenario) -> Result
 * @param Ghost The Ghost test instance.
 * @param Scenario The scenario to test.
 * @return The test result.
 */
FORBOCAI_SDK_API GhostTypes::GhostTestRunResult RunTest(const FGhost &Ghost,
                                          const FString &Scenario);

/**
 * Runs all test scenarios.
 * Pure function: Ghost -> Report
 * @param Ghost The Ghost test instance.
 * @return The complete test report.
 */
FORBOCAI_SDK_API GhostTypes::GhostTestRunAllResult RunAllTests(const FGhost &Ghost);

/**
 * Validates the test configuration.
 * Pure function: Config -> Result
 * @param Config The test configuration to validate.
 * @return The validation result.
 */
FORBOCAI_SDK_API GhostTypes::GhostValidationResult ValidateConfig(const FGhostConfig &Config);

/**
 * Generates a summary report.
 * Pure function: Report -> Summary
 * @param Report The test report.
 * @return The summary string.
 */
FORBOCAI_SDK_API FString GenerateSummary(const FGhostTestReport &Report);

/**
 * Exports test results to JSON.
 * Pure function: Report -> JSON
 * @param Report The test report.
 * @return The JSON string representation.
 */
FORBOCAI_SDK_API FString ExportToJson(const FGhostTestReport &Report);

/**
 * Exports test results to CSV.
 * Pure function: Report -> CSV
 * @param Report The test report.
 * @return The CSV string representation.
 */
FORBOCAI_SDK_API FString ExportToCsv(const FGhostTestReport &Report);

} // namespace GhostOps
