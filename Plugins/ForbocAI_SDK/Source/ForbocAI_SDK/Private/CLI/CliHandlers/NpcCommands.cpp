#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleNpc(rtk::EnhancedStore<FStoreState> &Store,
                       const FString &CommandKey,
                       const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("npc_create")) {
    FString Persona = Args.Num() > 0 ? Args[0] : TEXT("default");
    FNPCInternalState Npc = Ops::CreateNpc(Store, Persona);
    UE_LOG(LogTemp, Display, TEXT("Created NPC: %s"), *Npc.Id);
    return just(Result::Success("NPC created"));
  }

  if (CommandKey == TEXT("npc_list")) {
    TArray<FNPCInternalState> Npcs = Ops::ListNpcs(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d NPCs"), Npcs.Num());
    for (const FNPCInternalState &Npc : Npcs) {
      UE_LOG(LogTemp, Display, TEXT("- %s (%s)"), *Npc.Id, *Npc.Persona);
    }
    return just(Result::Success("NPCs listed"));
  }

  if (CommandKey == TEXT("npc_process")) {
    if (Args.Num() < 2) {
      return just(Result::Failure("Usage: npc_process <npcId> <text>"));
    }
    FAgentResponse Resp = Ops::ProcessNpc(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("Verdict: %s"), *Resp.Dialogue);
    return just(Result::Success("Protocol complete"));
  }

  if (CommandKey == TEXT("npc_active")) {
    func::Maybe<FNPCInternalState> Active = Ops::GetActiveNpc(Store);
    if (Active.hasValue) {
      UE_LOG(LogTemp, Display, TEXT("Active NPC: %s (%s)"), *Active.value.Id,
             *Active.value.Persona);
    } else {
      UE_LOG(LogTemp, Display, TEXT("No active NPC"));
    }
    return just(Result::Success("Active NPC queried"));
  }

  if (CommandKey == TEXT("npc_state")) {
    func::Maybe<FNPCInternalState> Active = Ops::GetActiveNpc(Store);
    if (!Active.hasValue) {
      UE_LOG(LogTemp, Display, TEXT("No active NPC"));
      return just(Result::Success("No active NPC"));
    }
    UE_LOG(LogTemp, Display, TEXT("NPC ID:      %s"), *Active.value.Id);
    UE_LOG(LogTemp, Display, TEXT("Persona:     %s"), *Active.value.Persona);
    return just(Result::Success("NPC state printed"));
  }

  if (CommandKey == TEXT("npc_update")) {
    if (Args.Num() < 2) {
      return just(Result::Failure(
          "Usage: npc_update <npcId> [-Mood=<value>] [-Inventory=<item,item>]"));
    }
    FAgentState Delta;
    FString Json =
        Args.Num() > 2
            ? FString::Printf(TEXT("{\"Mood\":\"%s\",\"Inventory\":\"%s\"}"),
                              *Args[1], *Args[2])
            : FString::Printf(TEXT("{\"Mood\":\"%s\"}"), *Args[1]);
    Delta.JsonData = Json;
    Ops::UpdateNpc(Store, Args[0], Delta);
    UE_LOG(LogTemp, Display, TEXT("NPC %s updated"), *Args[0]);
    return just(Result::Success("NPC updated"));
  }

  if (CommandKey == TEXT("npc_import")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: npc_import <txId>"));
    }
    FImportedNpc Npc = Ops::ImportNpcFromSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("NPC imported from soul: %s"), *Npc.NpcId);
    return just(Result::Success("NPC imported from soul"));
  }

  if (CommandKey == TEXT("npc_chat")) {
    if (Args.Num() < 2) {
      return just(Result::Failure("Usage: npc_chat <npcId> <message>"));
    }
    UE_LOG(LogTemp, Display, TEXT("> You: %s"), *Args[1]);
    FAgentResponse Resp = Ops::ProcessNpc(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("> NPC: %s"), *Resp.Dialogue);
    if (!Resp.Action.Type.IsEmpty()) {
      UE_LOG(LogTemp, Display, TEXT("> Action: %s"), *Resp.Action.Type);
    }
    return just(Result::Success("Chat turn complete"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
