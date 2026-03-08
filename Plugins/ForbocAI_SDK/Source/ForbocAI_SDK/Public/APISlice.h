#pragma once

#include "Core/AsyncHttp.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.h"
#include "SDKConfig.h"

struct FSDKState; // Forward declaration to break circularity

namespace APISlice {

using namespace rtk;
using namespace func;

// Forward declaration of the API instance (defined in SDKStore.h)
extern rtk::ApiSlice<FSDKState> ForbocAiApi;

// --- Endpoints ---
namespace Endpoints {

/** Soul Endpoints */
inline auto fetchSoul(FString SoulId) {
  ApiEndpoint<FString, FSoul> Ep;
  Ep.EndpointName = TEXT("fetchSoul");
  Ep.ProvidesTags = {{TEXT("Soul"), SoulId}};
  Ep.RequestBuilder = [SoulId](const FString &Arg) {
    return AsyncHttp::Get<FSoul>(SDKConfig::GetApiUrl() + TEXT("/souls/") +
                                     SoulId,
                                 SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(SoulId);
}

inline auto fetchSouls() {
  ApiEndpoint<FEmptyPayload, TArray<FSoul>> Ep;
  Ep.EndpointName = TEXT("fetchSouls");
  Ep.ProvidesTags = {{TEXT("Soul"), TEXT("LIST")}};
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FSoul>>(
        SDKConfig::GetApiUrl() + TEXT("/souls"), SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(FEmptyPayload{});
}

/** NPC Endpoints */
inline auto fetchNPC(FString NpcId) {
  ApiEndpoint<FString, FAgent> Ep;
  Ep.EndpointName = TEXT("fetchNPC");
  Ep.ProvidesTags = {{TEXT("NPC"), NpcId}};
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Get<FAgent>(SDKConfig::GetApiUrl() + TEXT("/npcs/") +
                                      NpcId,
                                  SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(NpcId);
}

inline auto createNPC(FAgentConfig Config) {
  ApiEndpoint<FAgentConfig, FAgent> Ep;
  Ep.EndpointName = TEXT("createNPC");
  Ep.InvalidatesTags = {{TEXT("NPC"), TEXT("LIST")}};
  Ep.RequestBuilder = [Config](const FAgentConfig &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FAgent>(SDKConfig::GetApiUrl() + TEXT("/npcs"),
                                   Payload, SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(Config);
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
  return ForbocApi.injectEndpoint(Ep)(Query);
}

inline auto storeMemory(FMemoryItem Item) {
  ApiEndpoint<FMemoryItem, FMemoryItem> Ep;
  Ep.EndpointName = TEXT("storeMemory");
  Ep.InvalidatesTags = {{TEXT("Memory"), TEXT("SEARCH")},
                        {TEXT("Memory"), TEXT("LIST")}};
  Ep.RequestBuilder = [Item](const FMemoryItem &Arg) {
    FString Payload;
    FJsonObjectConverter::UStructToJsonObjectString(Arg, Payload);
    return AsyncHttp::Post<FMemoryItem>(SDKConfig::GetApiUrl() +
                                            TEXT("/memories"),
                                        Payload, SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(Item);
}

/** Directive Endpoints */
inline auto fetchDirectives() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("fetchDirectives");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(
        SDKConfig::GetApiUrl() + TEXT("/directives"), SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto fetchDirectiveRuns(FString NpcId) {
  ApiEndpoint<FString, TArray<FDirectiveRun>> Ep;
  Ep.EndpointName = TEXT("fetchDirectiveRuns");
  Ep.RequestBuilder = [NpcId](const FString &Arg) {
    return AsyncHttp::Get<TArray<FDirectiveRun>>(
        SDKConfig::GetApiUrl() + TEXT("/directives/runs?npcId=") + NpcId,
        SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(NpcId);
}

/** Cortex Endpoints */
inline auto fetchModels() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("fetchModels");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/cortex/models"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto getCortexStatus() {
  ApiEndpoint<FEmptyPayload, FCortexStatus> Ep;
  Ep.EndpointName = TEXT("getCortexStatus");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<FCortexStatus>(SDKConfig::GetApiUrl() +
                                             TEXT("/cortex/status"),
                                         SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(FEmptyPayload{});
}

/** Ghost Endpoints */
inline auto fetchGhostSessions() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("fetchGhostSessions");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/ghost/sessions"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(FEmptyPayload{});
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
  return ForbocApi.injectEndpoint(Ep)(Config);
}

/** Bridge Endpoints */
inline auto fetchPresets() {
  ApiEndpoint<FEmptyPayload, TArray<FString>> Ep;
  Ep.EndpointName = TEXT("fetchPresets");
  Ep.RequestBuilder = [](const FEmptyPayload &Arg) {
    return AsyncHttp::Get<TArray<FString>>(SDKConfig::GetApiUrl() +
                                               TEXT("/bridge/presets"),
                                           SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)(FEmptyPayload{});
}

inline auto validatePolicy(FString PolicyId, FString Content) {
  struct FPolicyRequest {
    FString id;
    FString content;
  };
  ApiEndpoint<FPolicyRequest, FValidationResult> Ep;
  Ep.EndpointName = TEXT("validatePolicy");
  Ep.RequestBuilder = [PolicyId, Content](const FPolicyRequest &Arg) {
    FString Payload = FString::Printf(
        TEXT("{\"id\":\"%s\",\"content\":\"%s\"}"), *PolicyId, *Content);
    return AsyncHttp::Post<FValidationResult>(SDKConfig::GetApiUrl() +
                                                  TEXT("/bridge/validate"),
                                              Payload, SDKConfig::GetApiKey());
  };
  return ForbocApi.injectEndpoint(Ep)({PolicyId, Content});
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
  return ForbocApi.injectEndpoint(Ep)(Req);
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
