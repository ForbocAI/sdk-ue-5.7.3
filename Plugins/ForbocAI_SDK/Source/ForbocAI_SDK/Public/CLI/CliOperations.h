#pragma once

#include "CoreMinimal.h"
#include "Core/functional_core.hpp"
#include "NPC/NPCId.h"
#include "NPC/NPCSlice.h"
#include "RuntimeConfig.h"
#include "RuntimeStore.h"
#include "Thunks.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include <stdexcept>

/**
 * SDK Operations — Free functions mirroring TS sdkOps.ts
 *
 * Every CLI operation dispatches thunks through the store.
 * No direct HTTP calls, no logic — just store dispatch + await.
 */
namespace SDKOps {

// ---------------------------------------------------------------------------
// Shared utilities
// ---------------------------------------------------------------------------

/**
 * Pumps the HTTP tick loop until the AsyncResult completes.
 * Allows async thunks to be used in synchronous CLI context.
 */
template <typename T>
T WaitForResult(func::AsyncResult<T> &&Async, double TimeoutSeconds = 15.0) {
  bool bCompleted = false;
  T Result{};
  FString Error;

  Async.then([&bCompleted, &Result](T Value) {
    Result = Value;
    bCompleted = true;
  }).catch_([&bCompleted, &Error](std::string Message) {
    Error = UTF8_TO_TCHAR(Message.c_str());
    bCompleted = true;
  });
  Async.execute();

  const double StartTime = FPlatformTime::Seconds();
  while (!bCompleted &&
         (FPlatformTime::Seconds() - StartTime) < TimeoutSeconds) {
    FHttpModule::Get().GetHttpManager().Tick(0.05f);
    FPlatformProcess::Sleep(0.05f);
  }

  if (!Error.IsEmpty()) {
    throw std::runtime_error(TCHAR_TO_UTF8(*Error));
  }

  if (!bCompleted) {
    throw std::runtime_error("Timed out waiting for async result");
  }

  return Result;
}

struct FRuntimeConfig {
  FString ApiUrl;
  FString ApiKey;
};

inline FRuntimeConfig RuntimeConfig() {
  FRuntimeConfig Cfg;
  Cfg.ApiUrl = SDKConfig::GetApiUrl();
  Cfg.ApiKey = SDKConfig::GetApiKey();
  return Cfg;
}

// ---------------------------------------------------------------------------
// System ops
// ---------------------------------------------------------------------------

inline FApiStatusResponse CheckApiStatus(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::checkApiStatusThunk()));
}

// ---------------------------------------------------------------------------
// NPC ops
// ---------------------------------------------------------------------------

inline FNPCInternalState CreateNpc(rtk::EnhancedStore<FSDKState> &Store,
                                   const FString &Persona) {
  FString Id = NPCId::GenerateNPCId();
  FNPCInternalState Info;
  Info.Id = Id;
  Info.Persona = Persona;
  Store.dispatch(NPCSlice::Actions::SetNPCInfo(Info));
  func::Maybe<FNPCInternalState> Active =
      NPCSlice::SelectActiveNPC(Store.getState().NPCs);
  return Active.hasValue ? Active.value : Info;
}

inline func::Maybe<FNPCInternalState>
GetActiveNpc(rtk::EnhancedStore<FSDKState> &Store) {
  return NPCSlice::SelectActiveNPC(Store.getState().NPCs);
}

inline func::Maybe<FNPCInternalState>
UpdateNpc(rtk::EnhancedStore<FSDKState> &Store, const FString &NpcId,
          const FAgentState &Delta) {
  Store.dispatch(NPCSlice::Actions::UpdateNPCState(NpcId, Delta));
  return NPCSlice::SelectNPCById(Store.getState().NPCs, NpcId);
}

inline FAgentResponse ProcessNpc(rtk::EnhancedStore<FSDKState> &Store,
                                 const FString &NpcId, const FString &Text) {
  return WaitForResult(
      Store.dispatch(rtk::processNPC(NpcId, Text, TEXT("{}"), TEXT(""),
                                     FAgentState(),
                                     rtk::LocalProtocolRuntime())));
}

inline TArray<FNPCInternalState>
ListNpcs(rtk::EnhancedStore<FSDKState> &Store) {
  return NPCSlice::SelectAllNPCs(Store.getState().NPCs);
}

// ---------------------------------------------------------------------------
// Memory ops (remote — API-backed)
// ---------------------------------------------------------------------------

inline TArray<FMemoryItem> MemoryList(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &NpcId) {
  return WaitForResult(Store.dispatch(rtk::listMemoryRemoteThunk(NpcId)));
}

inline TArray<FMemoryItem> MemoryRecall(rtk::EnhancedStore<FSDKState> &Store,
                                        const FString &NpcId,
                                        const FString &Query,
                                        int32 Limit = 10,
                                        float Threshold = 0.7f) {
  TArray<FMemoryItem> Results = WaitForResult(
      Store.dispatch(rtk::recallMemoryRemoteThunk(NpcId, Query, Threshold)));
  if (Limit >= 0 && Results.Num() > Limit) {
    Results.SetNum(Limit);
  }
  return Results;
}

inline rtk::FEmptyPayload MemoryStore(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &NpcId,
                                      const FString &Observation,
                                      float Importance = 0.8f) {
  return WaitForResult(
      Store.dispatch(rtk::storeMemoryRemoteThunk(NpcId, Observation, Importance)));
}

inline rtk::FEmptyPayload MemoryClear(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &NpcId) {
  return WaitForResult(Store.dispatch(rtk::clearMemoryRemoteThunk(NpcId)));
}

// ---------------------------------------------------------------------------
// Memory ops (local — node-backed)
// ---------------------------------------------------------------------------

inline rtk::FEmptyPayload InitNodeMemory(rtk::EnhancedStore<FSDKState> &Store,
                                          const FString &DatabasePath = TEXT("")) {
  return WaitForResult(Store.dispatch(rtk::initNodeMemoryThunk(DatabasePath)));
}

inline FMemoryItem StoreNodeMemory(rtk::EnhancedStore<FSDKState> &Store,
                                   const FString &Text,
                                   float Importance = 0.8f) {
  return WaitForResult(
      Store.dispatch(
          rtk::storeNodeMemoryThunk(Text, TEXT("observation"), Importance)));
}

inline TArray<FMemoryItem>
RecallNodeMemory(rtk::EnhancedStore<FSDKState> &Store, const FString &Query,
                 int32 Limit = 10, float Threshold = 0.7f) {
  return WaitForResult(
      Store.dispatch(rtk::recallNodeMemoryThunk(Query, Limit, Threshold)));
}

inline rtk::FEmptyPayload
ClearNodeMemory(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::clearNodeMemoryThunk()));
}

// ---------------------------------------------------------------------------
// Cortex ops (local)
// ---------------------------------------------------------------------------

inline FCortexStatus InitCortex(rtk::EnhancedStore<FSDKState> &Store,
                                const FString &Model) {
  return WaitForResult(Store.dispatch(rtk::initNodeCortexThunk(Model)));
}

inline FCortexResponse CompleteCortex(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &Prompt) {
  return WaitForResult(Store.dispatch(rtk::completeNodeCortexThunk(Prompt)));
}

// ---------------------------------------------------------------------------
// Cortex ops (remote)
// ---------------------------------------------------------------------------

inline FCortexStatus InitRemoteCortex(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &Model,
                                      const FString &AuthKey = TEXT("")) {
  return WaitForResult(
      Store.dispatch(rtk::initRemoteCortexThunk(Model, AuthKey)));
}

inline TArray<FCortexModelInfo>
ListCortexModels(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::listCortexModelsThunk()));
}

inline FCortexResponse
CompleteRemoteCortex(rtk::EnhancedStore<FSDKState> &Store,
                     const FString &CortexId, const FString &Prompt) {
  return WaitForResult(
      Store.dispatch(rtk::completeRemoteThunk(CortexId, Prompt)));
}

// ---------------------------------------------------------------------------
// Ghost ops
// ---------------------------------------------------------------------------

inline FGhostRunResponse GhostRun(rtk::EnhancedStore<FSDKState> &Store,
                                  const FString &TestSuite,
                                  int32 Duration = 300) {
  FGhostConfig Config;
  Config.TestSuite = TestSuite;
  Config.Duration = Duration;
  return WaitForResult(Store.dispatch(rtk::startGhostThunk(Config)));
}

inline FGhostStatusResponse
GhostStatus(rtk::EnhancedStore<FSDKState> &Store, const FString &SessionId) {
  return WaitForResult(Store.dispatch(rtk::getGhostStatusThunk(SessionId)));
}

inline FGhostResultsResponse
GhostResults(rtk::EnhancedStore<FSDKState> &Store, const FString &SessionId) {
  return WaitForResult(Store.dispatch(rtk::getGhostResultsThunk(SessionId)));
}

inline FGhostStopResponse GhostStop(rtk::EnhancedStore<FSDKState> &Store,
                                    const FString &SessionId) {
  return WaitForResult(Store.dispatch(rtk::stopGhostThunk(SessionId)));
}

inline TArray<FGhostHistoryEntry>
GhostHistory(rtk::EnhancedStore<FSDKState> &Store, int32 Limit = 10) {
  return WaitForResult(Store.dispatch(rtk::getGhostHistoryThunk(Limit)));
}

// ---------------------------------------------------------------------------
// Bridge ops
// ---------------------------------------------------------------------------

inline FValidationResult
ValidateBridge(rtk::EnhancedStore<FSDKState> &Store,
               const FAgentAction &Action,
               const FBridgeValidationContext &Context) {
  return WaitForResult(
      Store.dispatch(rtk::validateBridgeThunk(Action, Context)));
}

inline TArray<FBridgeRule>
BridgeRules(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::getBridgeRulesThunk()));
}

inline FDirectiveRuleSet BridgePreset(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &PresetName) {
  return WaitForResult(
      Store.dispatch(rtk::loadBridgePresetThunk(PresetName)));
}

inline TArray<FDirectiveRuleSet>
RulesList(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::listRulesetsThunk()));
}

inline TArray<FString> RulesPresets(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::listRulePresetsThunk()));
}

inline FDirectiveRuleSet
RulesRegister(rtk::EnhancedStore<FSDKState> &Store,
              const FDirectiveRuleSet &Ruleset) {
  return WaitForResult(Store.dispatch(rtk::registerRulesetThunk(Ruleset)));
}

inline rtk::FEmptyPayload RulesDelete(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &RulesetId) {
  return WaitForResult(Store.dispatch(rtk::deleteRulesetThunk(RulesetId)));
}

// ---------------------------------------------------------------------------
// Soul ops
// ---------------------------------------------------------------------------

inline FSoulExportResult ExportSoul(rtk::EnhancedStore<FSDKState> &Store,
                                    const FString &NpcId) {
  return WaitForResult(Store.dispatch(rtk::remoteExportSoulThunk(NpcId)));
}

inline FSoul ImportSoul(rtk::EnhancedStore<FSDKState> &Store,
                        const FString &TxId) {
  return WaitForResult(
      Store.dispatch(rtk::importSoulFromArweaveThunk(TxId)));
}

inline FImportedNpc ImportNpcFromSoul(rtk::EnhancedStore<FSDKState> &Store,
                                      const FString &TxId) {
  return WaitForResult(Store.dispatch(rtk::importNpcFromSoulThunk(TxId)));
}

inline TArray<FSoulListItem> ListSouls(rtk::EnhancedStore<FSDKState> &Store,
                                       int32 Limit = 50) {
  return WaitForResult(Store.dispatch(rtk::getSoulListThunk(Limit)));
}

inline FSoulVerifyResult VerifySoul(rtk::EnhancedStore<FSDKState> &Store,
                                    const FString &TxId) {
  return WaitForResult(Store.dispatch(rtk::verifySoulThunk(TxId)));
}

inline FSoul LocalExportSoul(rtk::EnhancedStore<FSDKState> &Store,
                              const FString &NpcId = TEXT("")) {
  return WaitForResult(Store.dispatch(rtk::localExportSoulThunk(NpcId)));
}

// ---------------------------------------------------------------------------
// Vector ops
// ---------------------------------------------------------------------------

inline rtk::FEmptyPayload InitVector(rtk::EnhancedStore<FSDKState> &Store) {
  return WaitForResult(Store.dispatch(rtk::initNodeVectorThunk()));
}

inline TArray<float> GenerateEmbedding(rtk::EnhancedStore<FSDKState> &Store,
                                       const FString &Text) {
  return WaitForResult(Store.dispatch(rtk::generateNodeEmbeddingThunk(Text)));
}

// ---------------------------------------------------------------------------
// Config ops (local file, no store dispatch needed)
// ---------------------------------------------------------------------------

inline void ConfigSet(const FString &Key, const FString &Value) {
  SDKConfig::SetConfigValue(Key, Value);
}

inline FString ConfigGet(const FString &Key) {
  return SDKConfig::GetConfigValue(Key);
}

} // namespace SDKOps
