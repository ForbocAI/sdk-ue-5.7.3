#pragma once
// Test-game lib modules — mirrors TS lib/commandRunner.ts, render.ts, coverage.ts
// Command execution, ASCII grid rendering, runtime connectivity checks

#include "CoreMinimal.h"
#include "TestGame/TestGameStore.h"
#include "TestGame/TestGameTypes.h"

namespace TestGame {

// =========================================================================
// Command Runner — mirrors TS lib/commandRunner.ts
// =========================================================================

struct FCommandResult {
  ETranscriptStatus Status;
  FString Output;
};

/**
 * Executes a scenario command via FPlatformProcess and returns transcript-ready
 * status/output. Mirrors TS runCommand() using child_process.exec.
 */
inline FCommandResult RunCommand(const FCommandSpec &Command) {
  FString StdOut;
  FString StdErr;
  int32 ReturnCode = -1;

  // Parse the command — first token is the executable
  FString Executable;
  FString Args;
  if (!Command.Command.Split(TEXT(" "), &Executable, &Args)) {
    Executable = Command.Command;
  }

  void *ReadPipe = nullptr;
  void *WritePipe = nullptr;
  FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

  FProcHandle Proc = FPlatformProcess::CreateProc(
      *Executable, *Args, false, true, true, nullptr, 0, nullptr, WritePipe,
      ReadPipe);

  if (Proc.IsValid()) {
    // Wait with 10s timeout
    const double StartTime = FPlatformTime::Seconds();
    while (FPlatformProcess::IsProcRunning(Proc)) {
      if (FPlatformTime::Seconds() - StartTime > 10.0) {
        FPlatformProcess::TerminateProc(Proc, true);
        break;
      }
      StdOut += FPlatformProcess::ReadPipe(ReadPipe);
      FPlatformProcess::Sleep(0.01f);
    }
    StdOut += FPlatformProcess::ReadPipe(ReadPipe);
    FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode);
    FPlatformProcess::CloseProc(Proc);
  }

  FPlatformProcess::ClosePipe(ReadPipe);
  FPlatformProcess::ClosePipe(WritePipe);

  if (ReturnCode == 0) {
    return {ETranscriptStatus::Ok,
            StdOut.IsEmpty() ? TEXT("Command completed.") : StdOut.TrimEnd()};
  }
  return {ETranscriptStatus::Error,
          StdOut.IsEmpty() ? TEXT("Command failed.") : StdOut.TrimEnd()};
}

// =========================================================================
// Render — mirrors TS lib/render.ts
// =========================================================================

/**
 * Resolves a single grid cell character for the current game state.
 */
inline TCHAR CellAt(const FPosition &Pos, const FTestGameState &State) {
  // Check blocked
  for (const FPosition &B : State.Grid.Blocked) {
    if (B == Pos) return TEXT('#');
  }

  // Check player
  if (State.Player.Position == Pos) return TEXT('P');

  // Check NPCs
  auto AllNpcs = GetNPCAdapter().getSelectors().selectAll(State.NPCs.Entities);
  for (const FGameNPC &Npc : AllNpcs) {
    if (Npc.Position == Pos) {
      if (Npc.Id == TEXT("miller")) return TEXT('M');
      if (Npc.Id == TEXT("doomguard")) return TEXT('D');
      return TEXT('N');
    }
  }

  return TEXT('.');
}

/**
 * Renders the whole tile grid as ASCII rows.
 */
inline FString RenderGrid(const FTestGameState &State) {
  FString Result;
  for (int32 Y = 0; Y < State.Grid.Height; ++Y) {
    if (Y > 0) Result += TEXT("\n");
    for (int32 X = 0; X < State.Grid.Width; ++X) {
      if (X > 0) Result += TEXT(" ");
      Result += CellAt(FPosition(X, Y), State);
    }
  }
  return Result;
}

/**
 * Renders a compact legend string.
 */
inline FString RenderLegend() {
  return TEXT("Legend :: P=Scout  D=Doomguard  M=Miller  #=Blocked  .=Open");
}

// =========================================================================
// Coverage — mirrors TS lib/coverage.ts
// =========================================================================

/**
 * Checks runtime connectivity to a given status URL.
 * Returns true if the endpoint responds with HTTP 200 within 1.5s.
 */
inline bool CheckRuntimeConnectivity(
    const FString &Url = TEXT("http://localhost:8080/status")) {
  // Simplified sync check — in UE context this would use FHttpModule
  // For the test harness, we just check if the URL is reachable
  FString ResponseBody;
  // Note: Actual HTTP connectivity requires async FHttpModule usage.
  // For the test game this is a stub — the orchestrator handles fallback.
  return false;
}

/**
 * Resolves the best available runtime URL.
 */
inline FString ResolveRuntimeUrl() {
  if (CheckRuntimeConnectivity(TEXT("http://localhost:8080/status"))) {
    return TEXT("http://localhost:8080");
  }
  if (CheckRuntimeConnectivity(TEXT("https://api.forboc.ai/status"))) {
    return TEXT("https://api.forboc.ai");
  }
  return FString();
}

} // namespace TestGame
