#pragma once

#include "APICodecs.h"

namespace APISlice {

namespace Endpoints {

inline auto getNPCs() {
  TArray<FApiEndpointTag> Tags;
  Tags.Add(FApiEndpointTag{TEXT("NPC"), TEXT("LIST")});
  return Detail::MakeGet<TArray<FAgent>>(
      TEXT("getNPCs"), SDKConfig::GetApiUrl() + TEXT("/npcs"), Tags);
}

inline auto getNPC(const FString &NpcId) {
  TArray<FApiEndpointTag> Tags;
  Tags.Add(FApiEndpointTag{TEXT("NPC"), NpcId});
  return Detail::MakeGet<FAgent>(TEXT("getNPC"), SDKConfig::GetApiUrl() +
                                                     TEXT("/npcs/") +
                                                     Detail::Encode(NpcId),
                                 Tags);
}

inline auto postNPC(const FAgentConfig &Config) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("NPC"), TEXT("LIST")});
  if (!Config.Id.IsEmpty()) {
    Invalidates.Add(FApiEndpointTag{TEXT("NPC"), Config.Id});
  }
  return Detail::MakePost<FAgentConfig, FAgent>(
      TEXT("postNPC"), SDKConfig::GetApiUrl() + TEXT("/npcs"), Config,
      Invalidates);
}

inline auto getApiStatus() {
  return Detail::MakeGet<FApiStatusResponse>(
      TEXT("getApiStatus"), SDKConfig::GetApiUrl() + TEXT("/status"));
}

inline auto getCortexModels() {
  return Detail::MakeGet<TArray<FCortexModelInfo>>(
      TEXT("getCortexModels"), SDKConfig::GetApiUrl() + TEXT("/cortex/models"));
}

inline auto postCortexInit(const FCortexInitRequest &Request) {
  return Detail::MakePost<FCortexInitRequest, FCortexInitResponse>(
      TEXT("postCortexInit"), SDKConfig::GetApiUrl() + TEXT("/cortex/init"),
      Request);
}

inline auto postCortexComplete(const FString &CortexId,
                               const FCortexCompleteRequest &Request) {
  return Detail::MakeEndpoint<FCortexCompleteRequest, FCortexResponse>(
      TEXT("postCortexComplete"), Request,
      [CortexId](const FCortexCompleteRequest &Arg) {
        return func::AsyncHttp::Post<FCortexResponse>(
            SDKConfig::GetApiUrl() + TEXT("/cortex/") +
                Detail::Encode(CortexId) + TEXT("/complete"),
            Detail::BuildCortexCompletePayload(Arg), SDKConfig::GetApiKey());
      });
}

inline auto postNpcProcess(const FString &NpcId,
                           const FNPCProcessRequest &Request) {
  return Detail::MakePostWithCodec<FNPCProcessRequest, FNPCProcessResponse>(
      TEXT("postNpcProcess"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/process"),
      Request, Detail::EncodeNpcProcessRequest,
      Detail::DecodeNpcProcessResponse);
}

inline auto postDirective(const FString &NpcId,
                          const FDirectiveRequest &Request) {
  return Detail::MakePostWithCodec<FDirectiveRequest, FDirectiveResponse>(
      TEXT("postDirective"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/directive"),
      Request, Detail::EncodeDirectiveRequest, Detail::DecodeDirectiveResponse);
}

inline auto postContext(const FString &NpcId, const FContextRequest &Request) {
  return Detail::MakePostWithCodec<FContextRequest, FContextResponse>(
      TEXT("postContext"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/context"),
      Request, Detail::EncodeContextRequest, Detail::DecodeContextResponse);
}

inline auto postVerdict(const FString &NpcId, const FVerdictRequest &Request) {
  return Detail::MakePostWithCodec<FVerdictRequest, FVerdictResponse>(
      TEXT("postVerdict"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/verdict"),
      Request, Detail::EncodeVerdictRequest, Detail::DecodeVerdictResponse);
}

inline auto postMemoryStore(const FString &NpcId,
                            const FRemoteMemoryStoreRequest &Request) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Memory"), NpcId});
  return Detail::MakePost<FRemoteMemoryStoreRequest, rtk::FEmptyPayload>(
      TEXT("postMemoryStore"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory"),
      Request, Invalidates);
}

inline auto getMemoryList(const FString &NpcId) {
  TArray<FApiEndpointTag> Tags;
  Tags.Add(FApiEndpointTag{TEXT("Memory"), NpcId});
  return Detail::MakeGet<TArray<FMemoryItem>>(
      TEXT("getMemoryList"), SDKConfig::GetApiUrl() + TEXT("/npcs/") +
                                 Detail::Encode(NpcId) + TEXT("/memory"),
      Tags);
}

inline auto postMemoryRecall(const FString &NpcId,
                             const FRemoteMemoryRecallRequest &Request) {
  return Detail::MakePost<FRemoteMemoryRecallRequest, TArray<FMemoryItem>>(
      TEXT("postMemoryRecall"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory/recall"),
      Request);
}

inline auto deleteMemoryClear(const FString &NpcId) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Memory"), NpcId});
  return Detail::MakeDelete<rtk::FEmptyPayload>(
      TEXT("deleteMemoryClear"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory/clear"),
      Invalidates);
}

inline auto postBridgeValidate(const FString &NpcId,
                               const FString &PayloadJson) {
  const FString Url = NpcId.IsEmpty()
                          ? SDKConfig::GetApiUrl() + TEXT("/bridge/validate")
                          : SDKConfig::GetApiUrl() + TEXT("/bridge/validate/") +
                                Detail::Encode(NpcId);
  return Detail::MakePostRawWithCodec<FValidationResult>(
      TEXT("postBridgeValidate"), Url, PayloadJson,
      Detail::DecodeValidationResult);
}

inline auto postBridgeValidate(const FString &NpcId,
                               const FBridgeValidateRequest &Request) {
  const FString Url = NpcId.IsEmpty()
                          ? SDKConfig::GetApiUrl() + TEXT("/bridge/validate")
                          : SDKConfig::GetApiUrl() + TEXT("/bridge/validate/") +
                                Detail::Encode(NpcId);
  return Detail::MakePostWithCodec<FBridgeValidateRequest, FValidationResult>(
      TEXT("postBridgeValidate"), Url, Request,
      Detail::EncodeBridgeValidateRequest, Detail::DecodeValidationResult);
}

inline auto getBridgeRules() {
  return Detail::MakeGetWithCodec<TArray<FBridgeRule>>(
      TEXT("getBridgeRules"), SDKConfig::GetApiUrl() + TEXT("/bridge/rules"),
      Detail::DecodeBridgeRulesResponse);
}

inline auto postBridgePreset(const FString &PresetName) {
  return Detail::MakePostRawWithCodec<FDirectiveRuleSet>(
      TEXT("postBridgePreset"),
      SDKConfig::GetApiUrl() + TEXT("/rules/presets/") +
          Detail::Encode(PresetName),
      TEXT("{}"), Detail::DecodeDirectiveRuleSetResponse);
}

inline auto getRulesets() {
  return Detail::MakeGetWithCodec<TArray<FDirectiveRuleSet>>(
      TEXT("getRulesets"), SDKConfig::GetApiUrl() + TEXT("/rules"),
      Detail::DecodeDirectiveRuleSetListResponse);
}

inline auto getRulePresets() {
  return Detail::MakeGet<TArray<FString>>(
      TEXT("getRulePresets"), SDKConfig::GetApiUrl() + TEXT("/rules/presets"));
}

inline auto postRuleRegister(const FDirectiveRuleSet &Ruleset) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Rule"), TEXT("LIST")});
  return Detail::MakePostWithCodec<FDirectiveRuleSet, FDirectiveRuleSet>(
      TEXT("postRuleRegister"), SDKConfig::GetApiUrl() + TEXT("/rules"),
      Ruleset, Detail::ToJson<FDirectiveRuleSet>,
      Detail::DecodeDirectiveRuleSetResponse, Invalidates);
}

inline auto deleteRule(const FString &RulesetId) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Rule"), TEXT("LIST")});
  return Detail::MakeDelete<rtk::FEmptyPayload>(
      TEXT("deleteRule"),
      SDKConfig::GetApiUrl() + TEXT("/rules/") + Detail::Encode(RulesetId),
      Invalidates);
}

inline auto postGhostRun(const FGhostRunRequest &Request) {
  return Detail::MakePostWithCodec<FGhostRunRequest, FGhostRunResponse>(
      TEXT("postGhostRun"), SDKConfig::GetApiUrl() + TEXT("/ghost/run"),
      Request, Detail::ToJson<FGhostRunRequest>,
      Detail::DecodeGhostRunResponse);
}

inline auto postGhostRun(const FGhostConfig &Config) {
  return postGhostRun(
      TypeFactory::GhostRunRequest(Config.TestSuite, Config.Duration));
}

inline auto getGhostStatus(const FString &SessionId) {
  return Detail::MakeGetWithCodec<FGhostStatusResponse>(
      TEXT("getGhostStatus"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/status"),
      Detail::DecodeGhostStatusResponse);
}

inline auto getGhostResults(const FString &SessionId) {
  return Detail::MakeGetWithCodec<FGhostResultsResponse>(
      TEXT("getGhostResults"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/results"),
      Detail::DecodeGhostResultsResponse);
}

inline auto postGhostStop(const FString &SessionId) {
  return Detail::MakePostRawWithCodec<FGhostStopResponse>(
      TEXT("postGhostStop"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/stop"),
      TEXT("{}"), Detail::DecodeGhostStopResponse);
}

inline auto getGhostHistory(int32 Limit = 10) {
  return Detail::MakeGetWithCodec<FGhostHistoryResponse>(
      TEXT("getGhostHistory"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/history?limit=") +
          FString::FromInt(Limit),
      Detail::DecodeGhostHistoryResponse);
}

inline auto postSoulExport(const FString &NpcId,
                           const FSoulExportPhase1Request &Request) {
  return Detail::MakePostWithCodec<FSoulExportPhase1Request,
                                   FSoulExportPhase1Response>(
      TEXT("postSoulExport"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/soul/export"),
      Request, Detail::EncodeSoulExportPhase1Request,
      Detail::DecodeSoulExportPhase1Response);
}

inline auto postSoulExportConfirm(const FString &NpcId,
                                  const FSoulExportConfirmRequest &Request) {
  return Detail::MakePostWithCodec<FSoulExportConfirmRequest,
                                   FSoulExportResponse>(
      TEXT("postSoulExportConfirm"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/soul/confirm"),
      Request, Detail::EncodeSoulExportConfirmRequest,
      Detail::DecodeSoulExportResponse);
}

inline auto getSouls(int32 Limit = 50) {
  return Detail::MakeGet<FSoulListResponse>(
      TEXT("getSouls"),
      SDKConfig::GetApiUrl() + TEXT("/souls?limit=") + FString::FromInt(Limit));
}

inline auto getSoulImport(const FString &TxId) {
  return Detail::MakeGet<FSoulImportPhase1Response>(
      TEXT("getSoulImport"),
      SDKConfig::GetApiUrl() + TEXT("/souls/") + Detail::Encode(TxId));
}

inline auto postSoulVerify(const FString &TxId) {
  return Detail::MakePostRawWithCodec<FSoulVerifyResult>(
      TEXT("postSoulVerify"),
      SDKConfig::GetApiUrl() + TEXT("/souls/") + Detail::Encode(TxId) +
          TEXT("/verify"),
      TEXT("{}"), Detail::DecodeSoulVerifyResponse);
}

inline auto postNpcImport(const FSoulImportPhase1Request &Request) {
  return Detail::MakePostWithCodec<FSoulImportPhase1Request,
                                   FSoulImportPhase1Response>(
      TEXT("postNpcImport"), SDKConfig::GetApiUrl() + TEXT("/npcs/import"),
      Request, Detail::EncodeSoulImportPhase1Request,
      Detail::DecodeSoulImportPhase1Response);
}

inline auto postNpcImportConfirm(const FSoulImportConfirmRequest &Request) {
  return Detail::MakePostWithCodec<FSoulImportConfirmRequest, FImportedNpc>(
      TEXT("postNpcImportConfirm"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/import/confirm"), Request,
      Detail::EncodeSoulImportConfirmRequest, Detail::DecodeImportedNpc);
}

} // namespace Endpoints

} // namespace APISlice
