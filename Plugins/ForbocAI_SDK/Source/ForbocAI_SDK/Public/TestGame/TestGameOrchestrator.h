#pragma once
/**
 * Test-game orchestrator — mirrors TS test-game/src/game.ts
 * Executes scenario steps, records transcripts, enforces CLI coverage
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

#include "CoreMinimal.h"
#include "TestGame/TestGameLib.h"
#include "TestGame/TestGameListeners.h"
#include "Core/functional_core.hpp"

namespace TestGame {

/**
 * Attempts to parse a JSON verdict from CLI output.
 * User Story: As test-game transcript replay, I need a parsed verdict shape so
 * CLI output can be converted into structured follow-up actions.
 * Returns true if a verdict with an action type was found.
 */
struct FParsedVerdict {
  bool bValid;
  FString ActionType;
  FPosition TargetHex;
  int32 SuspicionDelta;
  FParsedVerdict() : bValid(false), SuspicionDelta(0) {}
};

/**
 * Attempts to parse a verdict payload from CLI output.
 * User Story: As transcript replay, I need verdict parsing so command output
 * can drive follow-up state updates inside the test-game store.
 */
inline FParsedVerdict TryParseVerdict(const FString &Output) {
  /**
   * Simple JSON extraction — look for {"action":{"type":"..."}}
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  int32 ActionIdx = Output.Find(TEXT("\"action\""));
  return ActionIdx == INDEX_NONE
             ? FParsedVerdict()
             : [&]() -> FParsedVerdict {
                 int32 TypeIdx =
                     Output.Find(TEXT("\"type\""), ESearchCase::IgnoreCase,
                                 ESearchDir::FromStart, ActionIdx);
                 return TypeIdx == INDEX_NONE
                            ? FParsedVerdict()
                            : [&]() -> FParsedVerdict {
                                /**
                                 * Extract type value between quotes after "type":
                                 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
                                 */
                                int32 ColonIdx = Output.Find(
                                    TEXT(":"), ESearchCase::IgnoreCase,
                                    ESearchDir::FromStart, TypeIdx + 6);
                                return ColonIdx == INDEX_NONE
                                           ? FParsedVerdict()
                                           : [&]() -> FParsedVerdict {
                                               int32 QuoteStart = Output.Find(
                                                   TEXT("\""),
                                                   ESearchCase::IgnoreCase,
                                                   ESearchDir::FromStart,
                                                   ColonIdx + 1);
                                               return QuoteStart == INDEX_NONE
                                                          ? FParsedVerdict()
                                                          : [&]() -> FParsedVerdict {
                                                              int32 QuoteEnd =
                                                                  Output.Find(
                                                                      TEXT("\""),
                                                                      ESearchCase::IgnoreCase,
                                                                      ESearchDir::FromStart,
                                                                      QuoteStart + 1);
                                                              return QuoteEnd == INDEX_NONE
                                                                         ? FParsedVerdict()
                                                                         : [&]() -> FParsedVerdict {
                                                                             FParsedVerdict V;
                                                                             V.ActionType = Output.Mid(
                                                                                 QuoteStart + 1,
                                                                                 QuoteEnd - QuoteStart - 1);
                                                                             V.bValid = !V.ActionType.IsEmpty();
                                                                             return V;
                                                                           }();
                                                            }();
                                             }();
                              }();
               }();
}

/**
 * Applies initial state mutations for a scenario event type.
 * Mirrors TS applyScenarioInitialState().
 * User Story: As scenario setup, I need event-specific initialization so each
 * scenario starts with the right NPCs, dialogue, and memory state.
 */
inline void ApplyScenarioInitialState(
    const FScenarioStep &Step,
    rtk::EnhancedStore<FTestGameState> &Store) {
  (Step.EventType == EEventType::Stealth)
      ? [&]() {
          Store.dispatch(StealthActions::SetDoorOpen(true));
          Store.dispatch(StealthActions::BumpAlert(25));

          FGameNPC Doomguard;
          Doomguard.Id = TEXT("doomguard");
          Doomguard.Name = TEXT("Doomguard Patrol");
          Doomguard.Faction = TEXT("Doomguards");
          Doomguard.Hp = 100;
          Doomguard.Suspicion = 40;
          Doomguard.Position = FPosition(5, 10);
          Store.dispatch(NPCsActions::UpsertNPC(Doomguard));

          FMemoryRecord Mem;
          Mem.Id = TEXT("mem-door-001");
          Mem.NpcId = TEXT("doomguard");
          Mem.Text = TEXT("Armory door found open at x:5, y:12");
          Mem.Importance = 0.9f;
          Store.dispatch(GameMemoryActions::StoreMemory(Mem));
        }()
      : (void)0;

  (Step.EventType == EEventType::Social)
      ? [&]() {
          FGameNPC Miller;
          Miller.Id = TEXT("miller");
          Miller.Name = TEXT("Miller");
          Miller.Faction = TEXT("Neutral");
          Miller.Hp = 100;
          Miller.Suspicion = 50;
          Miller.Inventory.Add(TEXT("medkit"));
          Miller.KnownSecrets.Add(TEXT("player_stole_rations"));
          Miller.Position = FPosition(5, 12);
          Store.dispatch(NPCsActions::UpsertNPC(Miller));

          Store.dispatch(SocialActions::SetDialogue(
              TEXT("I know you took those rations...")));

          FTradeOffer Trade;
          Trade.NpcId = TEXT("miller");
          Trade.Item = TEXT("medkit");
          Trade.Price = 100;
          Store.dispatch(SocialActions::SetTradeOffer(Trade));

          NPCsActions::FPatchNPCPayload Patch;
          Patch.Id = TEXT("miller");
          Patch.Suspicion = 75;
          Patch.bHasSuspicion = true;
          Store.dispatch(NPCsActions::PatchNPC(Patch));
        }()
      : (void)0;

  (Step.EventType == EEventType::Escape)
      ? (Store.dispatch(PlayerActions::SetHidden(false)), void())
      : (void)0;

  (Step.EventType == EEventType::Persistence)
      ? [&]() {
          GameSoulActions::FMarkSoulExportedPayload Export;
          Export.NpcId = TEXT("doomguard");
          Export.TxId = TEXT("tx-demo-001");
          Store.dispatch(GameSoulActions::MarkSoulExported(Export));
          Store.dispatch(
              GameSoulActions::MarkSoulImported(TEXT("tx-demo-001")));
          Store.dispatch(
              GameMemoryActions::ClearMemoryForNpc(TEXT("doomguard")));
        }()
      : (void)0;
}

/**
 * Extracts an NPC id from a command string containing "process" or "chat".
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline FString ExtractNpcIdFromCommand(const FString &Command) {
  int32 ProcessIdx = Command.Find(TEXT("process "));
  int32 ChatIdx = Command.Find(TEXT("chat "));
  int32 StartIdx = ProcessIdx != INDEX_NONE
                       ? ProcessIdx + 8
                       : (ChatIdx != INDEX_NONE ? ChatIdx + 5 : -1);
  return StartIdx < 0
             ? FString()
             : [&]() -> FString {
                 int32 EndIdx = Command.Find(TEXT(" "), ESearchCase::IgnoreCase,
                                             ESearchDir::FromStart, StartIdx);
                 return EndIdx != INDEX_NONE
                            ? Command.Mid(StartIdx, EndIdx - StartIdx)
                            : Command.Mid(StartIdx);
               }();
}

/**
 * Applies a parsed verdict to the store for a given command.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline void ApplyVerdictIfValid(
    const FCommandSpec &Cmd, const FCommandResult &CmdResult,
    rtk::EnhancedStore<FTestGameState> &Store) {
  (Cmd.Group == ECommandGroup::NpcProcessChat)
      ? [&]() {
          FParsedVerdict Verdict = TryParseVerdict(CmdResult.Output);
          Verdict.bValid
              ? [&]() {
                  /**
                   * Extract NPC id from command string
                   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
                   */
                  FString NpcId = ExtractNpcIdFromCommand(Cmd.Command);
                  !NpcId.IsEmpty()
                      ? [&]() {
                          NPCsActions::FApplyVerdictPayload VP;
                          VP.Id = NpcId;
                          VP.ActionType = Verdict.ActionType;
                          VP.TargetHex = Verdict.TargetHex;
                          VP.SuspicionDelta = Verdict.SuspicionDelta;
                          Store.dispatch(NPCsActions::ApplyNpcVerdict(VP));
                        }()
                      : (void)0;
                }()
              : (void)0;
        }()
      : (void)0;

  (Cmd.Group == ECommandGroup::BridgeValidate)
      ? [&]() {
          FParsedVerdict Validation = TryParseVerdict(CmdResult.Output);
          (Validation.bValid && Validation.ActionType == TEXT("BLOCKED"))
              ? (Store.dispatch(
                     UIActions::AddMessage(TEXT("System Bridge Blocked Action"))),
                 void())
              : (void)0;
        }()
      : (void)0;
}

/**
 * Logs command result status and error details.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline void LogCommandResult(const FCommandSpec &Cmd,
                              const FCommandResult &CmdResult) {
  FString StatusStr = CmdResult.Status == ETranscriptStatus::Ok
                          ? FString(TEXT("[ok]"))
                          : FString(TEXT("[error]"));
  UE_LOG(LogTemp, Display, TEXT("%s %s"), *StatusStr, *Cmd.Command);

  (CmdResult.Status == ETranscriptStatus::Error)
      ? [&]() {
          UE_LOG(LogTemp, Error, TEXT("LOG_ERR_CRITICAL // BIT_ROT_DETECTED"));
          !CmdResult.Output.IsEmpty()
              ? (UE_LOG(LogTemp, Error, TEXT("  | %s"), *CmdResult.Output),
                 void())
              : (void)0;
        }()
      : (void)0;
}

/**
 * Processes a single command within a scenario step.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline void ProcessCommand(const FScenarioStep &Step, const FCommandSpec &Cmd,
                            rtk::EnhancedStore<FTestGameState> &Store) {
  FCommandResult CmdResult = RunCommand(Cmd);

  /**
   * Update coverage
   * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
   */
  Store.dispatch(HarnessActions::MarkCovered(Cmd.Group));

  /**
   * Record transcript
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  TranscriptActions::FRecordTranscriptPayload TxPayload;
  TxPayload.ScenarioId = Step.Id;
  TxPayload.CommandGroup = Cmd.Group;
  TxPayload.Command = Cmd.Command;
  TxPayload.ExpectedRoutes = Cmd.ExpectedRoutes;
  TxPayload.Status = CmdResult.Status;
  TxPayload.Output = CmdResult.Output;
  Store.dispatch(TranscriptActions::RecordTranscript(TxPayload));

  /**
   * Drive state from CLI output
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  ApplyVerdictIfValid(Cmd, CmdResult, Store);

  LogCommandResult(Cmd, CmdResult);
}

/**
 * Recursive helper — iterates commands in a step.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
namespace detail {
inline void ProcessCommands(const FScenarioStep &Step,
                             const TArray<FCommandSpec> &Commands, int32 Index,
                             rtk::EnhancedStore<FTestGameState> &Store) {
  return Index >= Commands.Num()
             ? (void)0
             : (ProcessCommand(Step, Commands[Index], Store),
                ProcessCommands(Step, Commands, Index + 1, Store));
}

/**
 * Recursive helper — iterates scenario steps.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline void ProcessSteps(const TArray<FScenarioStep> &Steps, int32 Index,
                          rtk::EnhancedStore<FTestGameState> &Store) {
  return Index >= Steps.Num()
             ? (void)0
             : (UE_LOG(LogTemp, Display, TEXT("\n:: %s [%s]"),
                       *Steps[Index].Title, *Steps[Index].Id),
                UE_LOG(LogTemp, Display, TEXT("%s"),
                       *Steps[Index].Description),
                ApplyScenarioInitialState(Steps[Index], Store),
                ProcessCommands(Steps[Index], Steps[Index].Commands, 0, Store),
                ProcessSteps(Steps, Index + 1, Store));
}

/**
 * Recursive helper — joins missing group ids into a comma-separated string.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline FString JoinMissingGroups(const TArray<ECommandGroup> &Missing,
                                  int32 Index, const FString &Acc) {
  return Index >= Missing.Num()
             ? Acc
             : JoinMissingGroups(
                   Missing, Index + 1,
                   Acc + (Index > 0 ? FString(TEXT(", ")) : FString()) +
                       FString::FromInt(static_cast<int32>(Missing[Index])));
}

/**
 * Recursive helper — logs transcript entries.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
inline void LogTranscriptEntries(const TArray<FTranscriptEntry> &Entries,
                                  int32 Index) {
  return Index >= Entries.Num()
             ? (void)0
             : (UE_LOG(LogTemp, Display, TEXT("%s | %s | %s"),
                       *Entries[Index].Timestamp,
                       *(Entries[Index].Status == ETranscriptStatus::Ok
                             ? FString(TEXT("ok   "))
                             : FString(TEXT("error"))),
                       *Entries[Index].Command),
                LogTranscriptEntries(Entries, Index + 1));
}
} // namespace detail

/**
 * Runs a full game session in the given mode.
 * Returns a GameRunResult with coverage status and transcript.
 * Mirrors TS runGame().
 * User Story: As end-to-end test execution, I need one orchestrator entrypoint
 * so a full scenario suite can run and report transcript plus coverage state.
 */
inline FGameRunResult RunGame(EPlayMode Mode) {
  auto Store = createTestGameStoreWithListeners();
  Store.dispatch(UIActions::SetMode(Mode));

  /**
   * Seed initial player NPC
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  FGameNPC Scout;
  Scout.Id = TEXT("scout");
  Scout.Name = TEXT("Scout");
  Scout.Faction = TEXT("Player");
  Scout.Hp = 100;
  Scout.Suspicion = 0;
  Scout.Inventory.Add(TEXT("coin-pouch"));
  Scout.Position = FPosition(1, 1);
  Store.dispatch(NPCsActions::UpsertNPC(Scout));

  UE_LOG(LogTemp, Display, TEXT("SYSTEM_OVERRIDE // NEURAL_LINK_ESTABLISHED"));
  UE_LOG(LogTemp, Display, TEXT("[VOID::WATCHER] Echoes session booting at "
                                "mode=%s"),
         Mode == EPlayMode::Autoplay ? TEXT("autoplay") : TEXT("manual"));
  UE_LOG(LogTemp, Display, TEXT("%s"), *RenderLegend());

  const auto &Steps = Store.getState().Scenario.Steps;
  detail::ProcessSteps(Steps, 0, Store);

  /**
   * Build result
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  const auto &State = Store.getState();
  TArray<ECommandGroup> Missing = SelectMissingGroups(State.Harness.Covered);
  bool bComplete = Missing.Num() == 0;

  FString Summary = bComplete
                        ? FString(TEXT("ALL_BINDINGS_COMPLETE :: Coverage achieved."))
                        : FString(TEXT("VOID_GAPS_DETECTED :: Missing groups -> ")) +
                              detail::JoinMissingGroups(Missing, 0, FString());

  /**
   * Log transcript summary
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  UE_LOG(LogTemp, Display, TEXT("\n=== Transcript Summary ==="));
  detail::LogTranscriptEntries(State.Transcript.Entries, 0);
  UE_LOG(LogTemp, Display, TEXT("%s"), *Summary);

  FGameRunResult Result;
  Result.bComplete = bComplete;
  Result.MissingGroups = Missing;
  Result.Transcript = State.Transcript.Entries;
  Result.Summary = Summary;
  return Result;
}

} // namespace TestGame
