#include "Ghost/GhostModuleInternal.h"

namespace GhostInternal {

GhostTypes::Either<FString, FGhostConfig>
ValidateTestConfig(const FGhostConfig &Config) {
  if (Config.Scenarios.Num() == 0) {
    return GhostTypes::make_left(FString(TEXT("No test scenarios provided")),
                                 FGhostConfig{});
  }
  if (Config.MaxIterations < 1) {
    return GhostTypes::make_left(FString(TEXT("Max iterations must be >= 1")),
                                 FGhostConfig{});
  }
  return GhostTypes::make_right(FString(), Config);
}

GhostTypes::Either<FString, FString>
GenerateTestSummary(const FGhostTestReport &Report) {
  FString Summary = FString::Printf(TEXT("Ghost Test Summary for Agent: %s\n"),
                                    *Report.Config.Agent.Id);
  Summary += FString::Printf(TEXT("Total Tests: %d\n"), Report.TotalTests);
  Summary += FString::Printf(TEXT("Passed: %d\n"), Report.PassedTests);
  Summary += FString::Printf(TEXT("Failed: %d\n"), Report.FailedTests);
  Summary += FString::Printf(TEXT("Success Rate: %.1f%%\n"),
                             Report.SuccessRate * 100.0f);
  return GhostTypes::make_right(FString(), Summary);
}

GhostTypes::Either<FString, FString>
ExportResultsToJson(const FGhostTestReport &Report) {
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
  return GhostTypes::make_right(FString(), Json);
}

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
  return GhostTypes::make_right(FString(), Csv);
}

} // namespace GhostInternal
