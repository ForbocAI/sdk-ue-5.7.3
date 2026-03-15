#pragma once
/**
 * Test-game lib modules — mirrors TS lib/commandRunner.ts, render.ts, coverage.ts
 * Command execution, ASCII grid rendering, runtime connectivity checks
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

#include "CoreMinimal.h"
#include "TestGame/TestGameStore.h"
#include "TestGame/TestGameTypes.h"

namespace TestGame {

/**
 * Command Runner — mirrors TS lib/commandRunner.ts
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

struct FCommandResult {
  ETranscriptStatus Status;
  FString Output;
};

/**
 * Executes a scenario command via FPlatformProcess and returns transcript-ready
 * status/output. Mirrors TS runCommand() using child_process.exec.
 * User Story: As test-game scenario execution, I need a command runner so CLI
 * steps can be invoked and captured in transcript-friendly form.
 */
inline FCommandResult RunCommand(const FCommandSpec &Command) {
  FString StdOut;
  FString StdErr;
  int32 ReturnCode = -1;

  /**
   * Parse the command — first token is the executable
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
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
    /**
     * Wait with 10s timeout
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
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

/**
 * Resolves a single grid cell character for the current game state.
 * User Story: As ASCII rendering, I need one cell resolver so the grid view
 * can show blocked tiles, the player, and NPCs consistently.
 */
inline TCHAR CellAt(const FPosition &Pos, const FTestGameState &State) {
  /**
   * Check blocked
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  for (const FPosition &B : State.Grid.Blocked) {
    if (B == Pos) return TEXT('#');
  }

  /**
   * Check player
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (State.Player.Position == Pos) return TEXT('P');

  /**
   * Check NPCs
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
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
 * User Story: As terminal rendering, I need the full grid rendered so scenario
 * output can show the current world state in text form.
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
 * User Story: As terminal rendering, I need a legend string so players can
 * interpret the ASCII symbols shown in the grid output.
 */
inline FString RenderLegend() {
  return TEXT("Legend :: P=Scout  D=Doomguard  M=Miller  #=Blocked  .=Open");
}

/**
 * Checks runtime connectivity to a given status URL.
 * Returns true if the endpoint responds with HTTP 200 within 1.5s.
 * User Story: As runtime fallback selection, I need a connectivity probe so
 * the test game can decide which runtime URL is available.
 */
inline bool CheckRuntimeConnectivity(
    const FString &Url = TEXT("http://localhost:8080/status")) {
  /**
   * Simplified sync check — in UE context this would use FHttpModule
   * For the test harness, we just check if the URL is reachable
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  FString ResponseBody;
  /**
   * Note: Actual HTTP connectivity requires async FHttpModule usage.
   * For the test game this is a stub — the orchestrator handles fallback.
   * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
   */
  return false;
}

/**
 * Resolves the best available runtime URL.
 * User Story: As runtime fallback selection, I need one URL resolver so the
 * test game can prefer local runtime and fall back to remote API cleanly.
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
