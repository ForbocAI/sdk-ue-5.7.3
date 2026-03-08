#pragma once

#include "CoreMinimal.h"
#include "NPC/AgentTypes.h"
#include "GhostTypes.generated.h"

USTRUCT(BlueprintType)
struct FGhostTestResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Scenario;

  UPROPERTY(BlueprintReadOnly)
  bool bPassed;

  UPROPERTY(BlueprintReadOnly)
  FString ActualResponse;

  UPROPERTY(BlueprintReadOnly)
  FString ErrorMessage;

  UPROPERTY(BlueprintReadOnly)
  int32 Iteration;

  UPROPERTY(BlueprintReadOnly)
  int64 Duration;

  FGhostTestResult() : bPassed(false), Iteration(0), Duration(0) {}
};

USTRUCT(BlueprintType)
struct FGhostHistoryEntry {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString SessionId;

  UPROPERTY(BlueprintReadOnly)
  FString TestSuite;

  UPROPERTY(BlueprintReadOnly)
  int64 StartedAt;

  UPROPERTY(BlueprintReadOnly)
  int64 CompletedAt;

  UPROPERTY(BlueprintReadOnly)
  FString Status;

  UPROPERTY(BlueprintReadOnly)
  float PassRate;

  FGhostHistoryEntry() : StartedAt(0), CompletedAt(0), PassRate(0.0f) {}
};

USTRUCT(BlueprintType)
struct FGhostRunResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString SessionId;

  UPROPERTY(BlueprintReadOnly)
  FString RunStatus;
};

USTRUCT(BlueprintType)
struct FGhostRunRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TestSuite;

  UPROPERTY(BlueprintReadOnly)
  int32 Duration;

  FGhostRunRequest() : Duration(300) {}
};

USTRUCT(BlueprintType)
struct FGhostConfig {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FAgent Agent;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> Scenarios;

  UPROPERTY(BlueprintReadOnly)
  int32 MaxIterations;

  UPROPERTY(BlueprintReadOnly)
  bool bVerbose;

  UPROPERTY(BlueprintReadOnly)
  FString ApiUrl;

  UPROPERTY(BlueprintReadOnly)
  FString ApiKey;

  UPROPERTY(BlueprintReadOnly)
  FString TestSuite;

  UPROPERTY(BlueprintReadOnly)
  int32 Duration;

  FGhostConfig() : MaxIterations(100), bVerbose(false), Duration(300) {}
};

USTRUCT(BlueprintType)
struct FGhostTestReport {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FGhostConfig Config;

  UPROPERTY(BlueprintReadOnly)
  TArray<FGhostTestResult> Results;

  UPROPERTY(BlueprintReadOnly)
  int32 TotalTests;

  UPROPERTY(BlueprintReadOnly)
  int32 PassedTests;

  UPROPERTY(BlueprintReadOnly)
  int32 FailedTests;

  UPROPERTY(BlueprintReadOnly)
  float SuccessRate;

  UPROPERTY(BlueprintReadOnly)
  FString Summary;

  FGhostTestReport()
      : TotalTests(0), PassedTests(0), FailedTests(0), SuccessRate(0.0f) {}
};

struct FGhost {
  FGhostConfig Config;
  bool bInitialized;

  FGhost() : bInitialized(false) {}
};

USTRUCT(BlueprintType)
struct FGhostStatusResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString GhostSessionId;

  UPROPERTY(BlueprintReadOnly)
  FString GhostStatus;

  UPROPERTY(BlueprintReadOnly)
  float GhostProgress;

  UPROPERTY(BlueprintReadOnly)
  int64 GhostStartedAt;

  UPROPERTY(BlueprintReadOnly)
  int32 GhostDuration;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> GhostErrors;

  FGhostStatusResponse()
      : GhostProgress(0.0f), GhostStartedAt(0), GhostDuration(0) {}
};

USTRUCT(BlueprintType)
struct FGhostResultRecord {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TestName;

  UPROPERTY(BlueprintReadOnly)
  bool bTestPassed;

  UPROPERTY(BlueprintReadOnly)
  int64 TestDuration;

  UPROPERTY(BlueprintReadOnly)
  FString TestError;

  UPROPERTY(BlueprintReadOnly)
  FString TestScreenshot;

  FGhostResultRecord() : bTestPassed(false), TestDuration(0) {}
};

USTRUCT(BlueprintType)
struct FGhostResultsResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString ResultsSessionId;

  UPROPERTY(BlueprintReadOnly)
  int32 ResultsTotalTests;

  UPROPERTY(BlueprintReadOnly)
  int32 ResultsPassed;

  UPROPERTY(BlueprintReadOnly)
  int32 ResultsFailed;

  UPROPERTY(BlueprintReadOnly)
  int32 ResultsSkipped;

  UPROPERTY(BlueprintReadOnly)
  int64 ResultsDuration;

  UPROPERTY(BlueprintReadOnly)
  TArray<FGhostResultRecord> ResultsTests;

  UPROPERTY(BlueprintReadOnly)
  float ResultsCoverage;

  UPROPERTY(BlueprintReadOnly)
  TMap<FString, FString> ResultsMetrics;

  FGhostResultsResponse()
      : ResultsTotalTests(0), ResultsPassed(0), ResultsFailed(0),
        ResultsSkipped(0), ResultsDuration(0), ResultsCoverage(0.0f) {}
};

USTRUCT(BlueprintType)
struct FGhostStopResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  bool bStopped;

  UPROPERTY(BlueprintReadOnly)
  FString StopStatus;

  UPROPERTY(BlueprintReadOnly)
  FString StopSessionId;

  FGhostStopResponse() : bStopped(false) {}
};

USTRUCT(BlueprintType)
struct FGhostHistoryResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  TArray<FGhostHistoryEntry> Sessions;
};

namespace TypeFactory {

inline FGhostConfig GhostConfig(const FAgent &Agent,
                                const TArray<FString> &Scenarios =
                                    TArray<FString>(),
                                int32 MaxIterations = 100,
                                bool bVerbose = false,
                                const FString &ApiUrl = TEXT(""),
                                const FString &ApiKey = TEXT(""),
                                const FString &TestSuite = TEXT(""),
                                int32 Duration = 300) {
  FGhostConfig Config;
  Config.Agent = Agent;
  Config.Scenarios = Scenarios;
  Config.MaxIterations = MaxIterations;
  Config.bVerbose = bVerbose;
  Config.ApiUrl = ApiUrl;
  Config.ApiKey = ApiKey;
  Config.TestSuite = TestSuite;
  Config.Duration = Duration;
  return Config;
}

inline FGhostRunRequest GhostRunRequest(const FString &TestSuite,
                                        int32 Duration = 300) {
  FGhostRunRequest Request;
  Request.TestSuite = TestSuite;
  Request.Duration = Duration;
  return Request;
}

} // namespace TypeFactory
