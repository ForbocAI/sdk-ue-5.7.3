#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Ghost/GhostSlice.h"
#include "Misc/AutomationTest.h"

using namespace rtk;
using namespace GhostSlice;

// ---------------------------------------------------------------------------
// Test: Progress update for wrong session is a no-op
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostProgressWrongSessionTest,
                                 "ForbocAI.Slices.Ghost.ProgressWrongSession",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FGhostProgressWrongSessionTest::RunTest(const FString &Parameters) {
  Slice<FGhostSliceState> GSlice = CreateGhostSlice();
  FGhostSliceState State;

  State = GSlice.Reducer(
      State, Actions::GhostSessionStarted(TEXT("sess_active"), TEXT("running")));

  // Progress for a different session
  State = GSlice.Reducer(
      State,
      Actions::GhostSessionProgress(TEXT("sess_wrong"), TEXT("running"), 0.75f));

  TestEqual("Progress unchanged (0.0)", State.Progress, 0.0f);
  TestEqual("Session unchanged", State.ActiveSessionId,
            FString(TEXT("sess_active")));

  return true;
}

// ---------------------------------------------------------------------------
// Test: Failed action for wrong session is a no-op
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostFailedWrongSessionTest,
                                 "ForbocAI.Slices.Ghost.FailedWrongSession",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FGhostFailedWrongSessionTest::RunTest(const FString &Parameters) {
  Slice<FGhostSliceState> GSlice = CreateGhostSlice();
  FGhostSliceState State;

  State = GSlice.Reducer(
      State, Actions::GhostSessionStarted(TEXT("sess_ok"), TEXT("running")));

  State = GSlice.Reducer(
      State,
      Actions::GhostSessionFailed(TEXT("sess_other"), TEXT("Some error")));

  TestEqual("Status still running", State.Status, FString(TEXT("running")));
  TestTrue("No error", State.Error.IsEmpty());

  return true;
}

// ---------------------------------------------------------------------------
// Test: Completed with zero test results
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostCompletedZeroResultsTest,
                                 "ForbocAI.Slices.Ghost.CompletedZeroResults",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FGhostCompletedZeroResultsTest::RunTest(const FString &Parameters) {
  Slice<FGhostSliceState> GSlice = CreateGhostSlice();
  FGhostSliceState State;

  State = GSlice.Reducer(
      State, Actions::GhostSessionStarted(TEXT("empty_sess"), TEXT("running")));

  FGhostTestReport EmptyReport;
  // No results added

  State = GSlice.Reducer(State, Actions::GhostSessionCompleted(EmptyReport));

  TestEqual("Status completed", State.Status, FString(TEXT("completed")));
  TestEqual("Progress 1.0", State.Progress, 1.0f);
  TestTrue("Has results flag set", State.bHasResults);
  TestEqual("Zero results", State.Results.Results.Num(), 0);

  return true;
}

// ---------------------------------------------------------------------------
// Test: Session restart (start after complete) resets state
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostSessionRestartTest,
                                 "ForbocAI.Slices.Ghost.SessionRestart",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FGhostSessionRestartTest::RunTest(const FString &Parameters) {
  Slice<FGhostSliceState> GSlice = CreateGhostSlice();
  FGhostSliceState State;

  // Complete a session
  State = GSlice.Reducer(
      State, Actions::GhostSessionStarted(TEXT("sess_1"), TEXT("running")));
  FGhostTestReport Report;
  FGhostTestResult Result;
  Result.Scenario = TEXT("test");
  Result.bPassed = true;
  Report.Results.Add(Result);
  State = GSlice.Reducer(State, Actions::GhostSessionCompleted(Report));
  TestEqual("Status completed", State.Status, FString(TEXT("completed")));

  // Start a new session
  State = GSlice.Reducer(
      State, Actions::GhostSessionStarted(TEXT("sess_2"), TEXT("running")));

  TestEqual("New session id", State.ActiveSessionId,
            FString(TEXT("sess_2")));
  TestEqual("Status running again", State.Status, FString(TEXT("running")));
  TestEqual("Progress reset", State.Progress, 0.0f);
  TestTrue("Error cleared", State.Error.IsEmpty());

  return true;
}

// ---------------------------------------------------------------------------
// Test: History replacement is atomic
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostHistoryReplacementTest,
                                 "ForbocAI.Slices.Ghost.HistoryReplacement",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FGhostHistoryReplacementTest::RunTest(const FString &Parameters) {
  Slice<FGhostSliceState> GSlice = CreateGhostSlice();
  FGhostSliceState State;

  // First history load
  TArray<FGhostHistoryEntry> History1;
  FGhostHistoryEntry Entry1;
  Entry1.SessionId = TEXT("old");
  History1.Add(Entry1);
  State = GSlice.Reducer(State, Actions::GhostHistoryLoaded(History1));
  TestEqual("First history", State.History.Num(), 1);

  // Second history load replaces
  TArray<FGhostHistoryEntry> History2;
  FGhostHistoryEntry Entry2a;
  Entry2a.SessionId = TEXT("new_a");
  FGhostHistoryEntry Entry2b;
  Entry2b.SessionId = TEXT("new_b");
  History2.Add(Entry2a);
  History2.Add(Entry2b);
  State = GSlice.Reducer(State, Actions::GhostHistoryLoaded(History2));
  TestEqual("History replaced", State.History.Num(), 2);
  TestEqual("First entry", State.History[0].SessionId,
            FString(TEXT("new_a")));

  return true;
}
