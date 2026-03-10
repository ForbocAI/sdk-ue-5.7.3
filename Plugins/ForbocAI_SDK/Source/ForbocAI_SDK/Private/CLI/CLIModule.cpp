#include "CLI/CLIModule.h"
#include "CLI/CliOperations.h"
#include "RuntimeConfig.h"
#include "RuntimeStore.h"
#include <exception>

namespace CLIOps {

namespace {

FString DescribeCortexStatus(const FCortexStatus &Status) {
  if (Status.bReady) {
    return TEXT("ready");
  }
  if (!Status.Error.IsEmpty()) {
    return Status.Error;
  }
  return TEXT("initializing");
}

} // namespace

/**
 * Single shared store instance for all CLI commands.
 * Mirrors TS cli.ts which imports { store } from '../index'.
 */
static rtk::EnhancedStore<FStoreState> &GetStore() {
  static rtk::EnhancedStore<FStoreState> Store = ConfigureStore();
  return Store;
}

func::TestResult<void> DispatchCommand(const FString &CommandKey,
                                       const TArray<FString> &Args) {
  using Result = func::TestResult<void>;
  rtk::EnhancedStore<FStoreState> &Store = GetStore();
  try {

  // ---- System ----
  if (CommandKey == TEXT("doctor") || CommandKey == TEXT("system_status")) {
    FApiStatusResponse Status = Ops::CheckApiStatus(Store);
    UE_LOG(LogTemp, Display, TEXT("API Status: %s (v%s)"), *Status.Status,
           *Status.Version);
    return Result::Success("Doctor check completed");
  }

  // ---- NPC ----
  if (CommandKey == TEXT("npc_create")) {
    FString Persona = Args.Num() > 0 ? Args[0] : TEXT("default");
    FNPCInternalState Npc = Ops::CreateNpc(Store, Persona);
    UE_LOG(LogTemp, Display, TEXT("Created NPC: %s"), *Npc.Id);
    return Result::Success("NPC created");
  }

  if (CommandKey == TEXT("npc_list")) {
    TArray<FNPCInternalState> Npcs = Ops::ListNpcs(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d NPCs"), Npcs.Num());
    for (const FNPCInternalState &Npc : Npcs) {
      UE_LOG(LogTemp, Display, TEXT("- %s (%s)"), *Npc.Id, *Npc.Persona);
    }
    return Result::Success("NPCs listed");
  }

  if (CommandKey == TEXT("npc_process")) {
    if (Args.Num() < 2) {
      return Result::Failure("Usage: npc_process <npcId> <text>");
    }
    FAgentResponse Resp = Ops::ProcessNpc(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("Verdict: %s"), *Resp.Dialogue);
    return Result::Success("Protocol complete");
  }

  if (CommandKey == TEXT("npc_active")) {
    func::Maybe<FNPCInternalState> Active = Ops::GetActiveNpc(Store);
    if (Active.hasValue) {
      UE_LOG(LogTemp, Display, TEXT("Active NPC: %s (%s)"), *Active.value.Id,
             *Active.value.Persona);
    } else {
      UE_LOG(LogTemp, Display, TEXT("No active NPC"));
    }
    return Result::Success("Active NPC queried");
  }

  // ---- Memory ----
  if (CommandKey == TEXT("memory_list")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: memory_list <npcId>");
    }
    TArray<FMemoryItem> Items = Ops::MemoryList(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Found %d memories"), Items.Num());
    return Result::Success("Memories listed");
  }

  if (CommandKey == TEXT("memory_recall")) {
    if (Args.Num() < 2) {
      return Result::Failure("Usage: memory_recall <npcId> <query>");
    }
    TArray<FMemoryItem> Items = Ops::MemoryRecall(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("Recalled %d memories"), Items.Num());
    return Result::Success("Memories recalled");
  }

  if (CommandKey == TEXT("memory_store")) {
    if (Args.Num() < 2) {
      return Result::Failure("Usage: memory_store <npcId> <observation>");
    }
    Ops::MemoryStore(Store, Args[0], Args[1]);
    return Result::Success("Memory stored");
  }

  if (CommandKey == TEXT("memory_clear")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: memory_clear <npcId>");
    }
    Ops::MemoryClear(Store, Args[0]);
    return Result::Success("Memory cleared");
  }

  // ---- Cortex ----
  if (CommandKey == TEXT("cortex_init")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: cortex_init <model>");
    }
    FCortexStatus Status = Ops::InitCortex(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Cortex initialized: %s"),
           *DescribeCortexStatus(Status));
    return Result::Success("Cortex initialized");
  }

  if (CommandKey == TEXT("cortex_init_remote")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: cortex_init_remote <model> [authKey]");
    }
    FString AuthKey = Args.Num() > 1 ? Args[1] : TEXT("");
    FCortexStatus Status = Ops::InitRemoteCortex(Store, Args[0], AuthKey);
    UE_LOG(LogTemp, Display, TEXT("Remote cortex initialized: %s"),
           *DescribeCortexStatus(Status));
    return Result::Success("Remote cortex initialized");
  }

  if (CommandKey == TEXT("cortex_models")) {
    TArray<FCortexModelInfo> Models = Ops::ListCortexModels(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d models"), Models.Num());
    return Result::Success("Models listed");
  }

  if (CommandKey == TEXT("cortex_complete")) {
    if (Args.Num() < 2) {
      return Result::Failure("Usage: cortex_complete <cortexId> <prompt>");
    }
    FCortexResponse Resp =
        Ops::CompleteRemoteCortex(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("Completion: %s"), *Resp.Text);
    return Result::Success("Completion done");
  }

  // ---- Ghost ----
  if (CommandKey == TEXT("ghost_run")) {
    FString Suite = Args.Num() > 0 ? Args[0] : TEXT("default");
    int32 Duration = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 300;
    FGhostRunResponse Resp = Ops::GhostRun(Store, Suite, Duration);
    UE_LOG(LogTemp, Display, TEXT("Ghost session started: %s"),
           *Resp.SessionId);
    return Result::Success("Ghost run started");
  }

  if (CommandKey == TEXT("ghost_status")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: ghost_status <sessionId>");
    }
    FGhostStatusResponse Resp = Ops::GhostStatus(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Ghost status: %s"), *Resp.GhostStatus);
    return Result::Success("Ghost status retrieved");
  }

  if (CommandKey == TEXT("ghost_results")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: ghost_results <sessionId>");
    }
    FGhostResultsResponse Resp = Ops::GhostResults(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Ghost results retrieved"));
    return Result::Success("Ghost results retrieved");
  }

  if (CommandKey == TEXT("ghost_stop")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: ghost_stop <sessionId>");
    }
    FGhostStopResponse Resp = Ops::GhostStop(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Ghost stopped: %s"), *Resp.StopStatus);
    return Result::Success("Ghost stopped");
  }

  if (CommandKey == TEXT("ghost_history")) {
    int32 Limit = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 10;
    TArray<FGhostHistoryEntry> History = Ops::GhostHistory(Store, Limit);
    UE_LOG(LogTemp, Display, TEXT("Ghost history: %d entries"), History.Num());
    return Result::Success("Ghost history retrieved");
  }

  // ---- Bridge ----
  if (CommandKey == TEXT("bridge_validate")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: bridge_validate <actionJson>");
    }
    FAgentAction Action;
    Action.PayloadJson = Args[0];
    FBridgeValidationContext Context;
    FValidationResult VResult = Ops::ValidateBridge(Store, Action, Context);
    UE_LOG(LogTemp, Display, TEXT("Validation: %s"),
           VResult.bValid ? TEXT("PASS") : TEXT("FAIL"));
    return Result::Success("Bridge validation done");
  }

  if (CommandKey == TEXT("bridge_rules")) {
    TArray<FBridgeRule> Rules = Ops::BridgeRules(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d bridge rules"), Rules.Num());
    return Result::Success("Bridge rules listed");
  }

  if (CommandKey == TEXT("bridge_preset")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: bridge_preset <presetName>");
    }
    FDirectiveRuleSet Preset = Ops::BridgePreset(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Loaded preset: %s"), *Preset.Id);
    return Result::Success("Bridge preset loaded");
  }

  if (CommandKey == TEXT("rules_list")) {
    TArray<FDirectiveRuleSet> Rulesets = Ops::RulesList(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d rulesets"), Rulesets.Num());
    return Result::Success("Rulesets listed");
  }

  if (CommandKey == TEXT("rules_presets")) {
    TArray<FString> Presets = Ops::RulesPresets(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d presets"), Presets.Num());
    return Result::Success("Rule presets listed");
  }

  if (CommandKey == TEXT("rules_register")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: rules_register <rulesetJson>");
    }
    FDirectiveRuleSet Ruleset;
    Ruleset.Id = Args[0];
    FDirectiveRuleSet Registered = Ops::RulesRegister(Store, Ruleset);
    UE_LOG(LogTemp, Display, TEXT("Registered ruleset: %s"), *Registered.Id);
    return Result::Success("Ruleset registered");
  }

  if (CommandKey == TEXT("rules_delete")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: rules_delete <rulesetId>");
    }
    Ops::RulesDelete(Store, Args[0]);
    return Result::Success("Ruleset deleted");
  }

  // ---- Soul ----
  if (CommandKey == TEXT("soul_export")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: soul_export <npcId>");
    }
    FSoulExportResult Exported = Ops::ExportSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Soul exported: %s"), *Exported.TxId);
    return Result::Success("Soul exported");
  }

  if (CommandKey == TEXT("soul_import")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: soul_import <txId>");
    }
    FSoul Imported = Ops::ImportSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Soul imported: %s"), *Imported.Id);
    return Result::Success("Soul imported");
  }

  if (CommandKey == TEXT("soul_import_npc")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: soul_import_npc <txId>");
    }
    FImportedNpc Npc = Ops::ImportNpcFromSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("NPC imported from soul: %s"), *Npc.NpcId);
    return Result::Success("NPC imported from soul");
  }

  if (CommandKey == TEXT("soul_list")) {
    int32 Limit = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 50;
    TArray<FSoulListItem> Souls = Ops::ListSouls(Store, Limit);
    UE_LOG(LogTemp, Display, TEXT("Found %d souls"), Souls.Num());
    return Result::Success("Souls listed");
  }

  if (CommandKey == TEXT("soul_verify")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: soul_verify <txId>");
    }
    FSoulVerifyResult Verified = Ops::VerifySoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Soul verification: %s"),
           Verified.bValid ? TEXT("VALID") : TEXT("INVALID"));
    return Result::Success("Soul verified");
  }

  // ---- Config ----
  if (CommandKey == TEXT("config_set")) {
    if (Args.Num() < 2) {
      return Result::Failure("Usage: config_set <key> <value>");
    }
    Ops::ConfigSet(Args[0], Args[1]);
    return Result::Success("Config updated");
  }

  if (CommandKey == TEXT("config_get")) {
    if (Args.Num() < 1) {
      return Result::Failure("Usage: config_get <key>");
    }
    FString Value = Ops::ConfigGet(Args[0]);
    UE_LOG(LogTemp, Display, TEXT("%s = %s"), *Args[0], *Value);
    return Result::Success("Config retrieved");
  }

  // ---- Vector ----
  if (CommandKey == TEXT("vector_init")) {
    Ops::InitVector(Store);
    return Result::Success("Vector initialized");
  }

  // ---- Unknown ----
  return Result::Failure(
      TCHAR_TO_UTF8(*FString::Printf(TEXT("Unknown command: %s"), *CommandKey)));
  } catch (const std::exception &Error) {
    return Result::Failure(std::string(Error.what()));
  }
}

} // namespace CLIOps
