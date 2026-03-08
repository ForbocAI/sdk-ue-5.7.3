#pragma once

#include "CoreMinimal.h"
#include "Types.h"

namespace GhostInternal {

/** Validates the test configuration. */
GhostTypes::Either<FString, FGhostConfig>
ValidateTestConfig(const FGhostConfig &Config);

/** Generates a summary of the test report. */
GhostTypes::Either<FString, FString>
GenerateTestSummary(const FGhostTestReport &Report);

/** Exports results to JSON string. */
GhostTypes::Either<FString, FString>
ExportResultsToJson(const FGhostTestReport &Report);

/** Exports results to CSV string. */
GhostTypes::Either<FString, FString>
ExportResultsToCsv(const FGhostTestReport &Report);

/** Internal async test runner. */
GhostTypes::AsyncResult<FGhostTestResult>
RunScenarioTest(const FAgent &Agent, const FString &Scenario);

/** Helper to run all tests sequentially. */
void RunTestsSequentially(const FGhost &Ghost,
                          TSharedPtr<FGhostTestReport> Report, int32 Index,
                          std::function<void(FGhostTestReport)> OnComplete,
                          std::function<void(std::string)> OnError);

} // namespace GhostInternal
