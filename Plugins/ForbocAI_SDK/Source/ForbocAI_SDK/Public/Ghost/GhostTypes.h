#pragma once

#include "CoreMinimal.h"
#include "GhostTypes.generated.h"
#include "NPC/AgentTypes.h"

/**
 * Ghost Status
 */
UENUM(BlueprintType)
enum class EGhostStatus : uint8 { Pending, Running, Completed, Failed };

/**
 * Ghost Status Info
 */
USTRUCT(BlueprintType)
struct FGhostStatusInfo {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString SessionId;

  UPROPERTY(BlueprintReadOnly)
  EGhostStatus Status;

  UPROPERTY(BlueprintReadOnly)
  float Progress;

  UPROPERTY(BlueprintReadOnly)
  int64 StartedAt;

  UPROPERTY(BlueprintReadOnly)
  int64 Duration;

  UPROPERTY(BlueprintReadOnly)
  TArray<FString> Errors;

  FGhostStatusInfo()
      : Status(EGhostStatus::Pending), Progress(0.0f), StartedAt(0),
        Duration(0) {}
};

/**
 * Ghost Test Result
 */
USTRUCT(BlueprintType)
struct FGhostTestResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Name;

  UPROPERTY(BlueprintReadOnly)
  bool bPassed;

  UPROPERTY(BlueprintReadOnly)
  int64 Duration;

  UPROPERTY(BlueprintReadOnly)
  FString Error;

  UPROPERTY(BlueprintReadOnly)
  FString Screenshot;

  FGhostTestResult() : bPassed(false), Duration(0) {}
};

/**
 * Ghost Results
 */
USTRUCT(BlueprintType)
struct FGhostResults {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString SessionId;

  UPROPERTY(BlueprintReadOnly)
  int32 TotalTests;

  UPROPERTY(BlueprintReadOnly)
  int32 Passed;

  UPROPERTY(BlueprintReadOnly)
  int32 Failed;

  UPROPERTY(BlueprintReadOnly)
  int32 Skipped;

  UPROPERTY(BlueprintReadOnly)
  int64 Duration;

  UPROPERTY(BlueprintReadOnly)
  TArray<FGhostTestResult> Tests;

  FGhostResults()
      : TotalTests(0), Passed(0), Failed(0), Skipped(0), Duration(0) {}
};

/**
 * Ghost Run Response
 */
USTRUCT(BlueprintType)
struct FGhostRunResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString SessionId;

  UPROPERTY(BlueprintReadOnly)
  FString RunStatus;

  FGhostRunResponse() {}
};

/**
 * Ghost History Entry
 */
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

/**
 * Ghost Test Configuration — Immutable data.
 */
struct FGhostConfig {
  FAgent Agent;
  TArray<FString> Scenarios;
  int32 MaxIterations;
  TMap<FString, FString> ExpectedResponses;
  bool bVerbose;
  FString ApiUrl;

  FGhostConfig() : MaxIterations(100), bVerbose(false) {}
};

/**
 * Ghost Test Engine — Immutable data.
 */
struct FGhost {
  FGhostConfig Config;
  bool bInitialized;

  FGhost() : bInitialized(false) {}
};

namespace TypeFactory {

inline FGhostStatusInfo GhostStatusInfo(FString SessionId, EGhostStatus Status,
                                        float Progress) {
  FGhostStatusInfo I;
  I.SessionId = MoveTemp(SessionId);
  I.Status = Status;
  I.Progress = Progress;
  return I;
}

inline FGhostConfig GhostConfig(FAgent Agent, TArray<FString> Scenarios = {},
                                int32 MaxIterations = 100,
                                TMap<FString, FString> ExpectedResponses = {},
                                bool bVerbose = false,
                                FString ApiUrl = TEXT("")) {
  FGhostConfig C;
  C.Agent = MoveTemp(Agent);
  C.Scenarios = MoveTemp(Scenarios);
  C.MaxIterations = MaxIterations;
  C.ExpectedResponses = MoveTemp(ExpectedResponses);
  C.bVerbose = bVerbose;
  C.ApiUrl = MoveTemp(ApiUrl);
  return C;
}

} // namespace TypeFactory
