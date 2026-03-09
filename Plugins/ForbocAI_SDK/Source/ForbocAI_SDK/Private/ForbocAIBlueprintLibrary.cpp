#include "ForbocAIBlueprintLibrary.h"
#include "CLI/SDKOps.h"
#include "SDKStore.h"

namespace {
rtk::EnhancedStore<FSDKState> &GetBPStore() {
  static rtk::EnhancedStore<FSDKState> Store = ConfigureSDKStore();
  return Store;
}
} // namespace

FString UForbocAIBlueprintLibrary::CheckApiStatus() {
  FApiStatusResponse Resp = SDKOps::CheckApiStatus(GetBPStore());
  return Resp.Status;
}

FString UForbocAIBlueprintLibrary::CreateNpc(const FString &Persona) {
  FNPCInternalState Npc = SDKOps::CreateNpc(GetBPStore(), Persona);
  return Npc.Id;
}

FString UForbocAIBlueprintLibrary::ProcessNpc(const FString &NpcId,
                                               const FString &Text) {
  FAgentResponse Resp = SDKOps::ProcessNpc(GetBPStore(), NpcId, Text);
  return Resp.Dialogue;
}

bool UForbocAIBlueprintLibrary::HasActiveNpc() {
  func::Maybe<FNPCInternalState> Active = SDKOps::GetActiveNpc(GetBPStore());
  return Active.hasValue;
}

void UForbocAIBlueprintLibrary::MemoryStore(const FString &NpcId,
                                             const FString &Observation) {
  SDKOps::MemoryStore(GetBPStore(), NpcId, Observation);
}

void UForbocAIBlueprintLibrary::MemoryClear(const FString &NpcId) {
  SDKOps::MemoryClear(GetBPStore(), NpcId);
}

FString UForbocAIBlueprintLibrary::GhostRun(const FString &TestSuite,
                                             int32 Duration) {
  FGhostRunResponse Resp = SDKOps::GhostRun(GetBPStore(), TestSuite, Duration);
  return Resp.SessionId;
}

FString UForbocAIBlueprintLibrary::GhostStop(const FString &SessionId) {
  FGhostStopResponse Resp = SDKOps::GhostStop(GetBPStore(), SessionId);
  return Resp.Status;
}

FString UForbocAIBlueprintLibrary::ExportSoul(const FString &NpcId) {
  FSoulExportResult Result = SDKOps::ExportSoul(GetBPStore(), NpcId);
  return Result.TxId;
}

FString UForbocAIBlueprintLibrary::ImportSoul(const FString &TxId) {
  FSoul Soul = SDKOps::ImportSoul(GetBPStore(), TxId);
  return Soul.Id;
}

bool UForbocAIBlueprintLibrary::VerifySoul(const FString &TxId) {
  FSoulVerifyResult Result = SDKOps::VerifySoul(GetBPStore(), TxId);
  return Result.bValid;
}

bool UForbocAIBlueprintLibrary::ValidateBridgeAction(const FString &ActionJson) {
  FAgentAction Action;
  Action.PayloadJson = ActionJson;
  FBridgeValidationContext Context;
  FValidationResult Result = SDKOps::ValidateBridge(GetBPStore(), Action, Context);
  return Result.bValid;
}

void UForbocAIBlueprintLibrary::ConfigSet(const FString &Key,
                                           const FString &Value) {
  SDKOps::ConfigSet(Key, Value);
}

FString UForbocAIBlueprintLibrary::ConfigGet(const FString &Key) {
  return SDKOps::ConfigGet(Key);
}
