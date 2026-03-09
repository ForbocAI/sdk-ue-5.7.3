#pragma once

#include "../Core/AsyncHttp.h"
#include "../Core/rtk.hpp"
#include "../SDKConfig.h"
#include "../Types.h"
#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "JsonObjectConverter.h"

struct FSDKState;

namespace APISlice {

using namespace rtk;

struct FAPIState {
  FString LastEndpoint;
  FString Status;
  FString Error;

  FAPIState() : Status(TEXT("idle")) {}
};

extern rtk::ApiSlice<FSDKState> ForbocAiApi;

namespace Detail {

template <typename T> inline FString ToJson(const T &Value) {
  FString Json;
  FJsonObjectConverter::UStructToJsonObjectString(Value, Json);
  return Json;
}

inline FString Encode(const FString &Value) {
  return FGenericPlatformHttp::UrlEncode(Value);
}

template <typename Arg, typename Result>
inline ThunkAction<Result, FSDKState>
MakeEndpoint(const FString &EndpointName, const Arg &ArgValue,
             std::function<func::AsyncResult<func::HttpResult<Result>>(
                 const Arg &)>
                 RequestBuilder,
             const TArray<FApiEndpointTag> &ProvidesTags =
                 TArray<FApiEndpointTag>(),
             const TArray<FApiEndpointTag> &InvalidatesTags =
                 TArray<FApiEndpointTag>()) {
  ApiEndpoint<Arg, Result> Endpoint;
  Endpoint.EndpointName = EndpointName;
  Endpoint.ProvidesTags = ProvidesTags;
  Endpoint.InvalidatesTags = InvalidatesTags;
  Endpoint.RequestBuilder = RequestBuilder;
  return ForbocAiApi.injectEndpoint(Endpoint)(ArgValue);
}

template <typename Result>
inline ThunkAction<Result, FSDKState>
MakeGet(const FString &EndpointName, const FString &Url,
        const TArray<FApiEndpointTag> &Tags = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<rtk::FEmptyPayload, Result>(
      EndpointName, rtk::FEmptyPayload{},
      [Url](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Get<Result>(Url, SDKConfig::GetApiKey());
      },
      Tags);
}

template <typename Request, typename Result>
inline ThunkAction<Result, FSDKState>
MakePost(const FString &EndpointName, const FString &Url,
         const Request &RequestValue,
         const TArray<FApiEndpointTag> &Invalidates = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<Request, Result>(
      EndpointName, RequestValue,
      [Url](const Request &Arg) {
        return func::AsyncHttp::Post<Result>(Url, ToJson(Arg),
                                             SDKConfig::GetApiKey());
      },
      TArray<FApiEndpointTag>(), Invalidates);
}

template <typename Result>
inline ThunkAction<Result, FSDKState>
MakeDelete(const FString &EndpointName, const FString &Url,
           const TArray<FApiEndpointTag> &Invalidates = TArray<FApiEndpointTag>()) {
  return MakeEndpoint<rtk::FEmptyPayload, Result>(
      EndpointName, rtk::FEmptyPayload{},
      [Url](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Delete<Result>(Url, SDKConfig::GetApiKey());
      },
      TArray<FApiEndpointTag>(), Invalidates);
}

} // namespace Detail

namespace Endpoints {

inline auto getNPCs() {
  TArray<FApiEndpointTag> Tags;
  Tags.Add(FApiEndpointTag{TEXT("NPC"), TEXT("LIST")});
  return Detail::MakeGet<TArray<FAgent>>(TEXT("getNPCs"),
                                         SDKConfig::GetApiUrl() +
                                             TEXT("/npcs"),
                                         Tags);
}

inline auto getNPC(const FString &NpcId) {
  return Detail::MakeGet<FAgent>(TEXT("getNPC"),
                                 SDKConfig::GetApiUrl() + TEXT("/npcs/") +
                                     Detail::Encode(NpcId));
}

inline auto postNPC(const FAgentConfig &Config) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("NPC"), TEXT("LIST")});
  return Detail::MakePost<FAgentConfig, FAgent>(
      TEXT("postNPC"), SDKConfig::GetApiUrl() + TEXT("/npcs"), Config,
      Invalidates);
}

inline auto getApiStatus() {
  return Detail::MakeGet<FApiStatusResponse>(TEXT("getApiStatus"),
                                             SDKConfig::GetApiUrl() +
                                                 TEXT("/status"));
}

inline auto getCortexModels() {
  return Detail::MakeGet<TArray<FCortexModelInfo>>(
      TEXT("getCortexModels"),
      SDKConfig::GetApiUrl() + TEXT("/cortex/models"));
}

inline auto postCortexInit(const FCortexInitRequest &Request) {
  return Detail::MakePost<FCortexInitRequest, FCortexInitResponse>(
      TEXT("postCortexInit"), SDKConfig::GetApiUrl() + TEXT("/cortex/init"),
      Request);
}

inline auto postCortexComplete(const FCortexCompleteRequest &Request) {
  return Detail::MakePost<FCortexCompleteRequest, FCortexResponse>(
      TEXT("postCortexComplete"),
      SDKConfig::GetApiUrl() + TEXT("/cortex/complete"), Request);
}

inline auto postNpcProcess(const FString &NpcId,
                           const FNPCProcessRequest &Request) {
  return Detail::MakePost<FNPCProcessRequest, FNPCProcessResponse>(
      TEXT("postNpcProcess"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/process"),
      Request);
}

inline auto postDirective(const FString &NpcId,
                          const FDirectiveRequest &Request) {
  return Detail::MakePost<FDirectiveRequest, FDirectiveResponse>(
      TEXT("postDirective"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/directive"),
      Request);
}

inline auto postContext(const FString &NpcId, const FContextRequest &Request) {
  return Detail::MakePost<FContextRequest, FContextResponse>(
      TEXT("postContext"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/context"),
      Request);
}

inline auto postVerdict(const FString &NpcId, const FVerdictRequest &Request) {
  return Detail::MakePost<FVerdictRequest, FVerdictResponse>(
      TEXT("postVerdict"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/verdict"),
      Request);
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
  return Detail::MakeGet<TArray<FMemoryItem>>(
      TEXT("getMemoryList"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/memory"));
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
  return Detail::MakeEndpoint<FString, FValidationResult>(
      TEXT("postBridgeValidate"), PayloadJson,
      [Url](const FString &Arg) {
        return func::AsyncHttp::Post<FValidationResult>(Url, Arg,
                                                        SDKConfig::GetApiKey());
      });
}

inline auto postBridgeValidate(const FString &NpcId,
                               const FBridgeValidateRequest &Request) {
  const FString Url = NpcId.IsEmpty()
                          ? SDKConfig::GetApiUrl() + TEXT("/bridge/validate")
                          : SDKConfig::GetApiUrl() + TEXT("/bridge/validate/") +
                                Detail::Encode(NpcId);
  return Detail::MakePost<FBridgeValidateRequest, FValidationResult>(
      TEXT("postBridgeValidate"), Url, Request);
}

inline auto getBridgeRules() {
  return Detail::MakeGet<TArray<FBridgeRule>>(TEXT("getBridgeRules"),
                                              SDKConfig::GetApiUrl() +
                                                  TEXT("/bridge/rules"));
}

inline auto postBridgePreset(const FString &PresetName) {
  return Detail::MakeEndpoint<rtk::FEmptyPayload, FDirectiveRuleSet>(
      TEXT("postBridgePreset"), rtk::FEmptyPayload{},
      [PresetName](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Post<FDirectiveRuleSet>(
            SDKConfig::GetApiUrl() + TEXT("/rules/presets/") +
                Detail::Encode(PresetName),
            TEXT("{}"), SDKConfig::GetApiKey());
      });
}

inline auto getRulesets() {
  return Detail::MakeGet<TArray<FDirectiveRuleSet>>(
      TEXT("getRulesets"), SDKConfig::GetApiUrl() + TEXT("/rules"));
}

inline auto getRulePresets() {
  return Detail::MakeGet<TArray<FString>>(TEXT("getRulePresets"),
                                          SDKConfig::GetApiUrl() +
                                              TEXT("/rules/presets"));
}

inline auto postRuleRegister(const FDirectiveRuleSet &Ruleset) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Rule"), TEXT("LIST")});
  return Detail::MakePost<FDirectiveRuleSet, FDirectiveRuleSet>(
      TEXT("postRuleRegister"), SDKConfig::GetApiUrl() + TEXT("/rules"),
      Ruleset, Invalidates);
}

inline auto deleteRule(const FString &RulesetId) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Rule"), TEXT("LIST")});
  return Detail::MakeDelete<rtk::FEmptyPayload>(TEXT("deleteRule"),
                                           SDKConfig::GetApiUrl() +
                                               TEXT("/rules/") +
                                               Detail::Encode(RulesetId),
                                           Invalidates);
}

inline auto postGhostRun(const FGhostRunRequest &Request) {
  return Detail::MakePost<FGhostRunRequest, FGhostRunResponse>(
      TEXT("postGhostRun"), SDKConfig::GetApiUrl() + TEXT("/ghost/run"),
      Request);
}

inline auto postGhostRun(const FGhostConfig &Config) {
  return postGhostRun(
      TypeFactory::GhostRunRequest(Config.TestSuite, Config.Duration));
}

inline auto getGhostStatus(const FString &SessionId) {
  return Detail::MakeGet<FGhostStatusResponse>(
      TEXT("getGhostStatus"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/status"));
}

inline auto getGhostResults(const FString &SessionId) {
  return Detail::MakeGet<FGhostResultsResponse>(
      TEXT("getGhostResults"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/") + Detail::Encode(SessionId) +
          TEXT("/results"));
}

inline auto postGhostStop(const FString &SessionId) {
  return Detail::MakeEndpoint<rtk::FEmptyPayload, FGhostStopResponse>(
      TEXT("postGhostStop"), rtk::FEmptyPayload{},
      [SessionId](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Post<FGhostStopResponse>(
            SDKConfig::GetApiUrl() + TEXT("/ghost/") +
                Detail::Encode(SessionId) + TEXT("/stop"),
            TEXT("{}"), SDKConfig::GetApiKey());
      });
}

inline auto getGhostHistory(int32 Limit = 10) {
  return Detail::MakeGet<FGhostHistoryResponse>(
      TEXT("getGhostHistory"),
      SDKConfig::GetApiUrl() + TEXT("/ghost/history?limit=") +
          FString::FromInt(Limit));
}

inline auto postSoulExport(const FString &NpcId,
                           const FSoulExportPhase1Request &Request) {
  return Detail::MakePost<FSoulExportPhase1Request, FSoulExportPhase1Response>(
      TEXT("postSoulExport"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/soul/export"),
      Request);
}

inline auto postSoulExportConfirm(const FString &NpcId,
                                  const FSoulExportConfirmRequest &Request) {
  return Detail::MakePost<FSoulExportConfirmRequest, FSoulExportResponse>(
      TEXT("postSoulExportConfirm"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/") + Detail::Encode(NpcId) +
          TEXT("/soul/confirm"),
      Request);
}

inline auto getSouls(int32 Limit = 50) {
  return Detail::MakeGet<FSoulListResponse>(
      TEXT("getSouls"), SDKConfig::GetApiUrl() + TEXT("/souls?limit=") +
                            FString::FromInt(Limit));
}

inline auto getSoulImport(const FString &TxId) {
  return Detail::MakeGet<FSoulImportPhase1Response>(
      TEXT("getSoulImport"),
      SDKConfig::GetApiUrl() + TEXT("/souls/") + Detail::Encode(TxId));
}

inline auto postSoulVerify(const FString &TxId) {
  return Detail::MakeEndpoint<rtk::FEmptyPayload, FSoulVerifyResult>(
      TEXT("postSoulVerify"), rtk::FEmptyPayload{},
      [TxId](const rtk::FEmptyPayload &) {
        return func::AsyncHttp::Post<FSoulVerifyResult>(
            SDKConfig::GetApiUrl() + TEXT("/souls/") + Detail::Encode(TxId) +
                TEXT("/verify"),
            TEXT("{}"), SDKConfig::GetApiKey());
      });
}

inline auto postNpcImport(const FSoulImportPhase1Request &Request) {
  return Detail::MakePost<FSoulImportPhase1Request, FSoulImportPhase1Response>(
      TEXT("postNpcImport"), SDKConfig::GetApiUrl() + TEXT("/npcs/import"),
      Request);
}

inline auto postNpcImportConfirm(const FSoulImportConfirmRequest &Request) {
  return Detail::MakePost<FSoulImportConfirmRequest, FImportedNpc>(
      TEXT("postNpcImportConfirm"),
      SDKConfig::GetApiUrl() + TEXT("/npcs/import/confirm"), Request);
}

} // namespace Endpoints

inline Slice<FAPIState> CreateAPISlice() {
  return SliceBuilder<FAPIState>(TEXT("forbocApi"), FAPIState()).build();
}

} // namespace APISlice
