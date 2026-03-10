#include "RuntimeBlueprintLibrary.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace {
rtk::EnhancedStore<FStoreState> &GetBPStore() {
  static rtk::EnhancedStore<FStoreState> Store = ConfigureStore();
  return Store;
}
} // namespace

FString UForbocAIBlueprintLibrary::CheckApiStatus() {
  FApiStatusResponse Resp = Ops::CheckApiStatus(GetBPStore());
  return Resp.Status;
}

FString UForbocAIBlueprintLibrary::CreateNpc(const FString &Persona) {
  FNPCInternalState Npc = Ops::CreateNpc(GetBPStore(), Persona);
  return Npc.Id;
}

FString UForbocAIBlueprintLibrary::ProcessNpc(const FString &NpcId,
                                               const FString &Text) {
  FAgentResponse Resp = Ops::ProcessNpc(GetBPStore(), NpcId, Text);
  return Resp.Dialogue;
}

FString UForbocAIBlueprintLibrary::ChatNpc(const FString &NpcId,
                                            const FString &Message) {
  FAgentResponse Resp = Ops::ProcessNpc(GetBPStore(), NpcId, Message);
  return Resp.Dialogue;
}

bool UForbocAIBlueprintLibrary::HasActiveNpc() {
  func::Maybe<FNPCInternalState> Active = Ops::GetActiveNpc(GetBPStore());
  return Active.hasValue;
}

void UForbocAIBlueprintLibrary::MemoryStore(const FString &NpcId,
                                             const FString &Observation) {
  Ops::MemoryStore(GetBPStore(), NpcId, Observation);
}

void UForbocAIBlueprintLibrary::MemoryClear(const FString &NpcId) {
  Ops::MemoryClear(GetBPStore(), NpcId);
}

FString UForbocAIBlueprintLibrary::GhostRun(const FString &TestSuite,
                                             int32 Duration) {
  FGhostRunResponse Resp = Ops::GhostRun(GetBPStore(), TestSuite, Duration);
  return Resp.SessionId;
}

FString UForbocAIBlueprintLibrary::GhostStop(const FString &SessionId) {
  FGhostStopResponse Resp = Ops::GhostStop(GetBPStore(), SessionId);
  return Resp.StopStatus;
}

FString UForbocAIBlueprintLibrary::ExportSoul(const FString &NpcId) {
  FSoulExportResult Result = Ops::ExportSoul(GetBPStore(), NpcId);
  return Result.TxId;
}

FString UForbocAIBlueprintLibrary::ImportSoul(const FString &TxId) {
  FSoul Soul = Ops::ImportSoul(GetBPStore(), TxId);
  return Soul.Id;
}

bool UForbocAIBlueprintLibrary::VerifySoul(const FString &TxId) {
  FSoulVerifyResult Result = Ops::VerifySoul(GetBPStore(), TxId);
  return Result.bValid;
}

bool UForbocAIBlueprintLibrary::ValidateBridgeAction(const FString &ActionJson) {
  FAgentAction Action;
  Action.PayloadJson = ActionJson;
  FBridgeValidationContext Context;
  FValidationResult Result = Ops::ValidateBridge(GetBPStore(), Action, Context);
  return Result.bValid;
}

void UForbocAIBlueprintLibrary::ConfigSet(const FString &Key,
                                           const FString &Value) {
  Ops::ConfigSet(Key, Value);
}

FString UForbocAIBlueprintLibrary::ConfigGet(const FString &Key) {
  return Ops::ConfigGet(Key);
}
