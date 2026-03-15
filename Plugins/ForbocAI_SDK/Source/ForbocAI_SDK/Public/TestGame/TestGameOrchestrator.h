#pragma once
/**
 * Test-game orchestrator — mirrors TS test-game/src/game.ts
 * Executes scenario steps, records transcripts, enforces CLI coverage
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

#include "CoreMinimal.h"
#include "TestGame/TestGameLib.h"
#include "TestGame/TestGameListeners.h"

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
  FParsedVerdict V;
  /**
   * Simple JSON extraction — look for {"action":{"type":"..."}}
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  int32 ActionIdx = Output.Find(TEXT("\"action\""));
  if (ActionIdx == INDEX_NONE) return V;

  int32 TypeIdx = Output.Find(TEXT("\"type\""), ESearchCase::IgnoreCase,
                               ESearchDir::FromStart, ActionIdx);
  if (TypeIdx == INDEX_NONE) return V;

  /**
   * Extract type value between quotes after "type":
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  int32 ColonIdx = Output.Find(TEXT(":"), ESearchCase::IgnoreCase,
                                ESearchDir::FromStart, TypeIdx + 6);
  if (ColonIdx == INDEX_NONE) return V;

  int32 QuoteStart =
      Output.Find(TEXT("\""), ESearchCase::IgnoreCase,
                   ESearchDir::FromStart, ColonIdx + 1);
  if (QuoteStart == INDEX_NONE) return V;

  int32 QuoteEnd =
      Output.Find(TEXT("\""), ESearchCase::IgnoreCase,
                   ESearchDir::FromStart, QuoteStart + 1);
  if (QuoteEnd == INDEX_NONE) return V;

  V.ActionType = Output.Mid(QuoteStart + 1, QuoteEnd - QuoteStart - 1);
  V.bValid = !V.ActionType.IsEmpty();
  return V;
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
  if (Step.EventType == EEventType::Stealth) {
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
  }

  if (Step.EventType == EEventType::Social) {
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
  }

  if (Step.EventType == EEventType::Escape) {
    Store.dispatch(PlayerActions::SetHidden(false));
  }

  if (Step.EventType == EEventType::Persistence) {
    GameSoulActions::FMarkSoulExportedPayload Export;
    Export.NpcId = TEXT("doomguard");
    Export.TxId = TEXT("tx-demo-001");
    Store.dispatch(GameSoulActions::MarkSoulExported(Export));
    Store.dispatch(
        GameSoulActions::MarkSoulImported(TEXT("tx-demo-001")));
    Store.dispatch(
        GameMemoryActions::ClearMemoryForNpc(TEXT("doomguard")));
  }
}

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
  for (const FScenarioStep &Step : Steps) {
    UE_LOG(LogTemp, Display, TEXT("\n:: %s [%s]"), *Step.Title, *Step.Id);
    UE_LOG(LogTemp, Display, TEXT("%s"), *Step.Description);

    ApplyScenarioInitialState(Step, Store);

    for (const FCommandSpec &Cmd : Step.Commands) {
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
      if (Cmd.Group == ECommandGroup::NpcProcessChat) {
        FParsedVerdict Verdict = TryParseVerdict(CmdResult.Output);
        if (Verdict.bValid) {
          /**
           * Extract NPC id from command string
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
          FString NpcId;
          int32 ProcessIdx = Cmd.Command.Find(TEXT("process "));
          int32 ChatIdx = Cmd.Command.Find(TEXT("chat "));
          int32 StartIdx = ProcessIdx != INDEX_NONE
                               ? ProcessIdx + 8
                               : (ChatIdx != INDEX_NONE ? ChatIdx + 5 : -1);
          if (StartIdx >= 0) {
            int32 EndIdx = Cmd.Command.Find(TEXT(" "),
                                             ESearchCase::IgnoreCase,
                                             ESearchDir::FromStart, StartIdx);
            NpcId = EndIdx != INDEX_NONE
                        ? Cmd.Command.Mid(StartIdx, EndIdx - StartIdx)
                        : Cmd.Command.Mid(StartIdx);
          }
          if (!NpcId.IsEmpty()) {
            NPCsActions::FApplyVerdictPayload VP;
            VP.Id = NpcId;
            VP.ActionType = Verdict.ActionType;
            VP.TargetHex = Verdict.TargetHex;
            VP.SuspicionDelta = Verdict.SuspicionDelta;
            Store.dispatch(NPCsActions::ApplyNpcVerdict(VP));
          }
        }
      }

      if (Cmd.Group == ECommandGroup::BridgeValidate) {
        FParsedVerdict Validation = TryParseVerdict(CmdResult.Output);
        if (Validation.bValid && Validation.ActionType == TEXT("BLOCKED")) {
          Store.dispatch(UIActions::AddMessage(
              TEXT("System Bridge Blocked Action")));
        }
      }

      FString StatusStr = CmdResult.Status == ETranscriptStatus::Ok
                              ? TEXT("[ok]")
                              : TEXT("[error]");
      UE_LOG(LogTemp, Display, TEXT("%s %s"), *StatusStr, *Cmd.Command);

      if (CmdResult.Status == ETranscriptStatus::Error) {
        UE_LOG(LogTemp, Error, TEXT("LOG_ERR_CRITICAL // BIT_ROT_DETECTED"));
        if (!CmdResult.Output.IsEmpty()) {
          UE_LOG(LogTemp, Error, TEXT("  | %s"), *CmdResult.Output);
        }
      }
    }
  }

  /**
   * Build result
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  const auto &State = Store.getState();
  TArray<ECommandGroup> Missing = SelectMissingGroups(State.Harness.Covered);
  bool bComplete = Missing.Num() == 0;

  FString Summary;
  if (bComplete) {
    Summary = TEXT("ALL_BINDINGS_COMPLETE :: Coverage achieved.");
  } else {
    Summary = TEXT("VOID_GAPS_DETECTED :: Missing groups -> ");
    for (int32 i = 0; i < Missing.Num(); ++i) {
      if (i > 0) Summary += TEXT(", ");
      Summary += FString::FromInt(static_cast<int32>(Missing[i]));
    }
  }

  /**
   * Log transcript summary
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  UE_LOG(LogTemp, Display, TEXT("\n=== Transcript Summary ==="));
  for (const FTranscriptEntry &E : State.Transcript.Entries) {
    FString StatusColor = E.Status == ETranscriptStatus::Ok
                              ? TEXT("ok   ")
                              : TEXT("error");
    UE_LOG(LogTemp, Display, TEXT("%s | %s | %s"), *E.Timestamp,
           *StatusColor, *E.Command);
  }
  UE_LOG(LogTemp, Display, TEXT("%s"), *Summary);

  FGameRunResult Result;
  Result.bComplete = bComplete;
  Result.MissingGroups = Missing;
  Result.Transcript = State.Transcript.Entries;
  Result.Summary = Summary;
  return Result;
}

} // namespace TestGame
