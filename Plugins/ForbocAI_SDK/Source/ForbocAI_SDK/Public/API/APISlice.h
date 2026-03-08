#pragma once

#include "../Core/AsyncHttp.h"
#include "../Core/rtk.hpp"
#include "../SDKConfig.h"
#include "../Types.h"
#include "CoreMinimal.h"

struct FSDKState; // Forward declaration to break circularity

namespace APISlice {

using namespace rtk;
using namespace func;

// Forward declaration of the API instance (defined in SDKStore.h)
extern rtk::ApiSlice<FSDKState> ForbocAiApi;

// --- Endpoints ---
namespace Endpoints {

/** Soul Endpoints */
inline auto getSoul(FString SoulId) {
  ApiEndpoint<FString, FSoul> Ep;
  Ep.EndpointName = TEXT("getSoul");
  Ep.ProvidesTags = {{TEXT("Soul"), SoulId}};
  Ep.RequestBuilder = [SoulId](const FString &Arg) {
    return AsyncHttp::Get<FSoul>(SDKConfig::GetApiUrl() + TEXT("/souls/") +
                                     SoulId,
                                 SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(SoulId);
}

inline auto getSouls() {
  ApiEndpoint<FEmptyPayload, TArray<FSoul>> Ep;
  Ep.EndpointName = TEXT("getSouls");
  Ep.ProvidesTags = {{TEXT("Soul"), TEXT("LIST")}};
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FSoul>>(
        SDKConfig::GetApiUrl() + TEXT("/souls"), SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

/** NPC Endpoints */
inline auto getNPC(FString NpcId) {
  ApiEndpoint<FString, FAgent> Ep;
  Ep.EndpointName = TEXT("getNPC");
  Ep.ProvidesTags = {{TEXT("NPC"), NpcId}};
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Get<FAgent>(SDKConfig::GetApiUrl() + TEXT("/npcs/") +
                                      NpcId,
                                  SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(NpcId);
}

inline auto getNPCs() {
  ApiEndpoint<FEmptyPayload, TArray<FAgent>> Ep;
  Ep.EndpointName = TEXT("getNPCs");
  Ep.ProvidesTags = {{TEXT("NPC"), TEXT("LIST")}};
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FAgent>>(
        SDKConfig::GetApiUrl() + TEXT("/npcs"), SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto postNPC(FAgentConfig Config) {
  ApiEndpoint<FAgentConfig, FAgent> Ep;
  Ep.EndpointName = TEXT("postNPC");
  Ep.InvalidatesTags = {{TEXT("NPC"), TEXT("LIST")}};
  Ep.RequestBuilder = [Config](const FAgentConfig &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FAgent>(SDKConfig::GetApiUrl() + TEXT("/npcs"),
                                   Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Config);
}

/** Memory Endpoints */
inline auto searchMemories(FString Query) {
  ApiEndpoint<FString, TArray<FMemoryItem>> Ep;
  Ep.EndpointName = TEXT("searchMemories");
  Ep.ProvidesTags = {{TEXT("Memory"), TEXT("SEARCH")}};
  Ep.RequestBuilder = [Query](const FString &Arg) {
    return AsyncHttp::Get<TArray<FMemoryItem>>(
        SDKConfig::GetApiUrl() + TEXT("/memories?query=") + Query,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Query);
}

inline auto postMemory(FMemoryItem Item) {
  ApiEndpoint<FMemoryItem, FMemoryItem> Ep;
  Ep.EndpointName = TEXT("postMemory");
  Ep.InvalidatesTags = {{TEXT("Memory"), TEXT("SEARCH")},
                        {TEXT("Memory"), TEXT("LIST")}};
  Ep.RequestBuilder = [Item](const FMemoryItem &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FMemoryItem>(SDKConfig::GetApiUrl() +
                                            TEXT("/memories"),
                                        Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Item);
}

/** Directive Endpoints */
inline auto getDirectives() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("getDirectives");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(
        SDKConfig::GetApiUrl() + TEXT("/directives"), SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto getDirectiveRuns(FString NpcId) {
  ApiEndpoint<FString, TArray<FDirectiveRun>> Ep;
  Ep.EndpointName = TEXT("getDirectiveRuns");
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Get<TArray<FDirectiveRun>>(
        SDKConfig::GetApiUrl() + TEXT("/directives/runs?npcId=") + NpcId,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(NpcId);
}

/** Cortex Endpoints */
inline auto getCortexModels() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("getCortexModels");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/cortex/models"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto postCortexInit(const FCortexInitRequest &Req) {
  ApiEndpoint<FCortexInitRequest, FCortexInitResponse> Ep;
  Ep.EndpointName = TEXT("postCortexInit");
  Ep.RequestBuilder = [Req](const FCortexInitRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FCortexInitResponse>(
        SDKConfig::GetApiUrl() + TEXT("/cortex/init"), Payload,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto postCortexComplete(const FCortexCompleteRequest &Req) {
  ApiEndpoint<FCortexCompleteRequest, FString> Ep;
  Ep.EndpointName = TEXT("postCortexComplete");
  Ep.RequestBuilder = [Req](const FCortexCompleteRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FString>(SDKConfig::GetApiUrl() +
                                        TEXT("/cortex/complete"),
                                    Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto getCortexStatus() {
  ApiEndpoint<FEmptyPayload, FCortexStatus> Ep;
  Ep.EndpointName = TEXT("getCortexStatus");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<FCortexStatus>(SDKConfig::GetApiUrl() +
                                             TEXT("/cortex/status"),
                                         SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

/** Ghost Endpoints */
inline auto getGhostSessions() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("getGhostSessions");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/ghost/sessions"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto createGhostSession(FGhostConfig Config) {
  ApiEndpoint<FGhostConfig, FString> Ep;
  Ep.EndpointName = TEXT("createGhostSession");
  Ep.RequestBuilder = [Config](const FGhostConfig &Arg) {
    FString Payload;
    // Note: FGhostConfig may need a custom mapper if nesting is complex
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FString>(SDKConfig::GetApiUrl() +
                                        TEXT("/ghost/sessions"),
                                    Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Config);
}

/** Bridge Endpoints */
inline auto postBridgePreset(FString PresetId) {
  ApiEndpoint<FString, FEmptyPayload> Ep;
  Ep.EndpointName = TEXT("postBridgePreset");
  Ep.RequestBuilder = [PresetId](const FString &Arg) {
    FString Payload = FString::Printf(TEXT("{\"presetId\":\"%s\"}"), *PresetId);
    return AsyncHttp::Post<FEmptyPayload>(SDKConfig::GetApiUrl() +
                                              TEXT("/bridge/presets"),
                                          Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(PresetId);
}

inline auto postBridgeValidate(FString PolicyId, FString Content) {
  struct FPolicyRequest {
    FString id;
    FString content;
  };
  ApiEndpoint<FPolicyRequest, FValidationResult> Ep;
  Ep.EndpointName = TEXT("postBridgeValidate");
  Ep.RequestBuilder = [PolicyId, Content](const FPolicyRequest &Arg) {
    FString Payload = FString::Printf(
        TEXT("{\"id\":\"%s\",\"content\":\"%s\"}"), *PolicyId, *Content);
    return AsyncHttp::Post<FValidationResult>(SDKConfig::GetApiUrl() +
                                                  TEXT("/bridge/validate"),
                                              Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)({PolicyId, Content});
}

/** Protocol Endpoints */
inline auto postNpcProcess(const FNPCProcessRequest &Req) {
  ApiEndpoint<FNPCProcessRequest, FNPCInstruction> Ep;
  Ep.EndpointName = TEXT("postNpcProcess");
  Ep.InvalidatesTags = {{TEXT("NPC"), Req.NpcId}};
  Ep.RequestBuilder = [Req](const FNPCProcessRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FNPCInstruction>(SDKConfig::GetApiUrl() +
                                                TEXT("/npcs/process"),
                                            Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

/** NPC Import Endpoints */
inline auto postNpcImport(FString SoulId) {
  ApiEndpoint<FString, FAgent> Ep;
  Ep.EndpointName = TEXT("postNpcImport");
  Ep.InvalidatesTags = {{TEXT("NPC"), TEXT("LIST")}};
  Ep.RequestBuilder = [SoulId](const FString &Arg) {
    FString Payload = FString::Printf(TEXT("{\"soulId\":\"%s\"}"), *SoulId);
    return AsyncHttp::Post<FAgent>(SDKConfig::GetApiUrl() +
                                       TEXT("/npcs/import"),
                                   Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(SoulId);
}

inline auto postNpcImportConfirm(FString ImportId) {
  ApiEndpoint<FString, FAgent> Ep;
  Ep.EndpointName = TEXT("postNpcImportConfirm");
  Ep.InvalidatesTags = {{TEXT("NPC"), TEXT("LIST")}};
  Ep.RequestBuilder = [ImportId](const FString &Arg) {
    FString Payload = FString::Printf(TEXT("{\"importId\":\"%s\"}"), *ImportId);
    return AsyncHttp::Post<FAgent>(SDKConfig::GetApiUrl() +
                                       TEXT("/npcs/import/confirm"),
                                   Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(ImportId);
}

/** Legacy Protocol Endpoints (for compatibility/internal use) */
inline auto postDirective(const FDirectiveRequest &Req) {
  ApiEndpoint<FDirectiveRequest, FDirectiveResponse> Ep;
  Ep.EndpointName = TEXT("postDirective");
  Ep.RequestBuilder = [Req](const FDirectiveRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FDirectiveResponse>(SDKConfig::GetApiUrl() +
                                                   TEXT("/directives"),
                                               Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto postContext(const FContextRequest &Req) {
  ApiEndpoint<FContextRequest, FContextResponse> Ep;
  Ep.EndpointName = TEXT("postContext");
  Ep.RequestBuilder = [Req](const FContextRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FContextResponse>(SDKConfig::GetApiUrl() +
                                                 TEXT("/context"),
                                             Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto postVerdict(const FVerdictRequest &Req) {
  ApiEndpoint<FVerdictRequest, FVerdictResponse> Ep;
  Ep.EndpointName = TEXT("postVerdict");
  Ep.RequestBuilder = [Req](const FVerdictRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FVerdictResponse>(SDKConfig::GetApiUrl() +
                                                 TEXT("/verdict"),
                                             Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

/** Memory Management Endpoints */
inline auto getMemoryList(FString NpcId) {
  ApiEndpoint<FString, TArray<FMemoryItem>> Ep;
  Ep.EndpointName = TEXT("getMemoryList");
  Ep.ProvidesTags = {{TEXT("Memory"), TEXT("LIST")}};
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Get<TArray<FMemoryItem>>(
        SDKConfig::GetApiUrl() + TEXT("/memories?npcId=") + NpcId,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(NpcId);
}

inline auto postMemoryRecall(const FMemoryRecallRequest &Req) {
  ApiEndpoint<FMemoryRecallRequest, TArray<FMemoryItem>> Ep;
  Ep.EndpointName = TEXT("postMemoryRecall");
  Ep.ProvidesTags = {{TEXT("Memory"), TEXT("RECALL")}};
  Ep.RequestBuilder = [Req](const FMemoryRecallRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<TArray<FMemoryItem>>(
        SDKConfig::GetApiUrl() + TEXT("/memories/recall"), Payload,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto deleteMemoryClear(FString NpcId) {
  ApiEndpoint<FString, FEmptyPayload> Ep;
  Ep.EndpointName = TEXT("deleteMemoryClear");
  Ep.InvalidatesTags = {{TEXT("Memory"), TEXT("LIST")},
                        {TEXT("Memory"), TEXT("SEARCH")}};
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Delete(SDKConfig::GetApiUrl() + TEXT("/memories?npcId=") +
                                 NpcId,
                             SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(NpcId);
}

/** Ghost Running Endpoints */
inline auto postGhostRun(const FGhostConfig &Config) {
  ApiEndpoint<FGhostConfig, FString> Ep;
  Ep.EndpointName = TEXT("postGhostRun");
  Ep.InvalidatesTags = {{TEXT("Ghost"), TEXT("LIST")}};
  Ep.RequestBuilder = [Config](const FGhostConfig &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FString>(SDKConfig::GetApiUrl() + TEXT("/ghost/run"),
                                    Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Config);
}

inline auto postGhostStop(FString SessionId) {
  ApiEndpoint<FString, FEmptyPayload> Ep;
  Ep.EndpointName = TEXT("postGhostStop");
  Ep.RequestBuilder = [SessionId](const FString &Arg) {
    FString Payload =
        FString::Printf(TEXT("{\"sessionId\":\"%s\"}"), *SessionId);
    return AsyncHttp::Post<FEmptyPayload>(SDKConfig::GetApiUrl() +
                                              TEXT("/ghost/stop"),
                                          Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(SessionId);
}

inline auto getGhostResults(FString SessionId) {
  ApiEndpoint<FString, FGhostTestReport> Ep;
  Ep.EndpointName = TEXT("getGhostResults");
  Ep.RequestBuilder = [SessionId](const FString &Arg) {
    return AsyncHttp::Get<FGhostTestReport>(
        SDKConfig::GetApiUrl() + TEXT("/ghost/results/") + SessionId,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(SessionId);
}

inline auto getGhostHistory(FString NpcId) {
  ApiEndpoint<FString, TArray<FGhostHistoryEntry>> Ep;
  Ep.EndpointName = TEXT("getGhostHistory");
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Get<TArray<FGhostHistoryEntry>>(
        SDKConfig::GetApiUrl() + TEXT("/ghost/history?npcId=") + NpcId,
        SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(NpcId);
}

/** Soul Portability Endpoints */
inline auto postSoulExport(FString NpcId) {
  ApiEndpoint<FString, FSoulExportRequest> Ep;
  Ep.EndpointName = TEXT("postSoulExport");
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    FString Payload = FString::Printf(TEXT("{\"npcId\":\"%s\"}"), *NpcId);
    return AsyncHttp::Post<FSoulExportRequest>(SDKConfig::GetApiUrl() +
                                                   TEXT("/souls/export"),
                                               Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(NpcId);
}

inline auto postSoulExportConfirm(const FSoulExportConfirmRequest &Req) {
  ApiEndpoint<FSoulExportConfirmRequest, FSoul> Ep;
  Ep.EndpointName = TEXT("postSoulExportConfirm");
  Ep.InvalidatesTags = {{TEXT("Soul"), TEXT("LIST")}};
  Ep.RequestBuilder = [Req](const FSoulExportConfirmRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FSoul>(SDKConfig::GetApiUrl() +
                                      TEXT("/souls/export/confirm"),
                                  Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto postSoulVerify(const FSoulVerifyRequest &Req) {
  ApiEndpoint<FSoulVerifyRequest, FSoulVerifyResult> Ep;
  Ep.EndpointName = TEXT("postSoulVerify");
  Ep.RequestBuilder = [Req](const FSoulVerifyRequest &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FSoulVerifyResult>(SDKConfig::GetApiUrl() +
                                                  TEXT("/souls/verify"),
                                              Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Req);
}

inline auto getSoulImport(FString SoulId) {
  ApiEndpoint<FString, FSoul> Ep;
  Ep.EndpointName = TEXT("getSoulImport");
  Ep.RequestBuilder = [SoulId](const FString &Arg) {
    return AsyncHttp::Get<FSoul>(SDKConfig::GetApiUrl() + TEXT("/souls/") +
                                     SoulId + TEXT("/import"),
                                 SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(SoulId);
}

/** Bridge & Rule Endpoints */
inline auto getRulesets() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("getRulesets");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/bridge/rulesets"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto getRulePresets() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("getRulePresets");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/bridge/presets"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto postRuleRegister(const FBridgeRule &Rule) {
  ApiEndpoint<FBridgeRule, FBridgeRule> Ep;
  Ep.EndpointName = TEXT("postRuleRegister");
  Ep.InvalidatesTags = {{TEXT("Rule"), TEXT("LIST")}};
  Ep.RequestBuilder = [Rule](const FBridgeRule &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FBridgeRule>(SDKConfig::GetApiUrl() +
                                            TEXT("/bridge/rules"),
                                        Payload, SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(Rule);
}

inline auto deleteRule(FString RuleId) {
  ApiEndpoint<FString, FEmptyPayload> Ep;
  Ep.EndpointName = TEXT("deleteRule");
  Ep.InvalidatesTags = {{TEXT("Rule"), TEXT("LIST")}};
  Ep.RequestBuilder = [RuleId](const FString &Arg) {
    return AsyncHttp::Delete(SDKConfig::GetApiUrl() + TEXT("/bridge/rules/") +
                                 RuleId,
                             SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(RuleId);
}

/** System Endpoints */
inline auto getApiStatus() {
  ApiEndpoint<FEmptyPayload, FApiStatusResponse> Ep;
  Ep.EndpointName = TEXT("getApiStatus");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<FApiStatusResponse>(
        SDKConfig::GetApiUrl() + TEXT("/status"), SDKConfig::GetApiKey());
  };
  return ForbocAiApi.injectEndpoint(Ep)(FEmptyPayload{});
}

} // namespace Endpoints

// --- Slice Builder ---
inline Slice<FAPIState> CreateAPISlice() {
  return SliceBuilder<FAPIState>(TEXT("forbocApi"), FAPIState())
      .addCase<CacheUpdateAction>(
          [](FAPIState State, const CacheUpdateAction &Action) {
            State.Queries.Add(Action.Payload.Key, Action.Payload.Data);
            return State;
          })
      .build();
}

} // namespace APISlice
