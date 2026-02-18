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
using func::AsyncResult;
using func::ConfigBuilder;
using func::Curried;
using func::Either;
using func::Lazy;
using func::Maybe;
using func::Pipeline;
using func::TestResult;
using func::ValidationPipeline;

using func::just;
using func::make_left;
using func::make_right;
using func::nothing;

// Type aliases for Ghost operations
using GhostCreationResult = Either<FString, FGhost>;
using GhostTestRunResult = Either<FString, FGhostTestResult>;
using GhostTestRunAllResult = Either<FString, FGhostTestReport>;
using GhostValidationResult = Either<FString, FGhostConfig>;
} // namespace GhostTypes

// Types (FGhostConfig, FGhostTestResult, FGhostTestReport, FGhost) are defined in ForbocAI_SDK_Types.h

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
 * Async function: (Ghost, Scenario) -> AsyncResult<Result>
 * @param Ghost The Ghost test instance.
 * @param Scenario The scenario to test.
 * @return The async test result.
 */
FORBOCAI_SDK_API GhostTypes::GhostTestRunResult
RunTest(const FGhost &Ghost, const FString &Scenario);

/**
 * Runs all test scenarios.
 * Async function: Ghost -> AsyncResult<Report>
 * @param Ghost The Ghost test instance.
 * @return The complete test report (async).
 */
FORBOCAI_SDK_API GhostTypes::GhostTestRunAllResult
RunAllTests(const FGhost &Ghost);

/**
 * Validates the test configuration.
 * Pure function: Config -> Result
 * @param Config The test configuration to validate.
 * @return The validation result.
 */
FORBOCAI_SDK_API GhostTypes::GhostValidationResult
ValidateConfig(const FGhostConfig &Config);

/**
 * Generates a summary report.
 * Pure function: Report -> Summary
 * @param Report The test report.
 * @return The summary string or error.
 */
FORBOCAI_SDK_API GhostTypes::Either<FString, FString>
GenerateSummary(const FGhostTestReport &Report);

/**
 * Exports test results to JSON.
 * Pure function: Report -> JSON
 * @param Report The test report.
 * @return The JSON string or error.
 */
FORBOCAI_SDK_API GhostTypes::Either<FString, FString>
ExportToJson(const FGhostTestReport &Report);

/**
 * Exports test results to CSV.
 * Pure function: Report -> CSV
 * @param Report The test report.
 * @return The CSV string or error.
 */
FORBOCAI_SDK_API GhostTypes::Either<FString, FString>
ExportToCsv(const FGhostTestReport &Report);

} // namespace GhostOps

// Factory namespace for consistency with other modules
namespace GhostFactory {
inline FGhost Create(const FGhostConfig &Config) {
  return GhostOps::Create(Config);
}
} // namespace GhostFactory

// GhostHelpers moved to end of file to ensure type visibility
// Functional Core Helper Functions for Ghost operations
namespace GhostHelpers {
// Helper to create a lazy ghost
inline GhostTypes::Lazy<FGhost> createLazyGhost(const FGhostConfig &config) {
  return func::lazy([config]() -> FGhost { return GhostOps::Create(config); });
}

// Helper to create a validation pipeline for ghost configuration
inline GhostTypes::ValidationPipeline<FGhostConfig, FString>
ghostConfigValidationPipeline() {
  return func::validationPipeline<FGhostConfig, FString>()
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

// Helper to create a pipeline for ghost test execution
inline GhostTypes::Pipeline<FGhost> ghostTestPipeline(const FGhost &ghost) {
  return func::pipe(ghost);
}

// Helper to create a curried ghost creation function
inline GhostTypes::Curried<
    1, std::function<GhostTypes::GhostCreationResult(FGhostConfig)>>
curriedGhostCreation() {
  return func::curry<1>(
      [](FGhostConfig config) -> GhostTypes::GhostCreationResult {
        try {
          FGhost ghost = GhostOps::Create(config);
          return GhostTypes::make_right(FString(), ghost);
        } catch (const std::exception &e) {
          return GhostTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace GhostHelpers
