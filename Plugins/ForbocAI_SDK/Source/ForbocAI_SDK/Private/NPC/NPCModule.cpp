#include "NPC/NPCModule.h"
#include "Containers/UnrealString.h"
#include "Core/functional_core.hpp"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Guid.h"
#include "RuntimeConfig.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// ==========================================
// AGENT FACTORY — Pure construction functions
// ==========================================

namespace {

FString ToBase36(uint64 Value) {
  static const TCHAR Digits[] = TEXT("0123456789abcdefghijklmnopqrstuvwxyz");

  if (Value == 0) {
    return TEXT("0");
  }

  FString Encoded;
  while (Value > 0) {
    const int32 DigitIndex = static_cast<int32>(Value % 36ull);
    Encoded.InsertAt(0, FString(1, &Digits[DigitIndex]));
    Value /= 36ull;
  }

  return Encoded;
}

FString GenerateNPCId() {
  return FString::Printf(TEXT("ag_%s"),
                         *ToBase36(FDateTime::UtcNow().ToUnixTimestamp()));
}

FString SerializeContextMap(const TMap<FString, FString> &Context) {
  if (Context.IsEmpty()) {
    return TEXT("{}");
  }

  const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
  for (const TPair<FString, FString> &Entry : Context) {
    Root->SetStringField(Entry.Key, Entry.Value);
  }

  FString Json;
  const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
      TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
  FJsonSerializer::Serialize(Root, Writer);
  return Json;
}

} // namespace

FAgent AgentFactory::Create(const FAgentConfig &Config) {
  FAgent Agent;
  Agent.Id = Config.Id.IsEmpty() ? GenerateNPCId() : Config.Id;
  Agent.Persona = Config.Persona;
  Agent.State = Config.InitialState;
  Agent.ApiUrl =
      Config.ApiUrl.IsEmpty() ? SDKConfig::GetApiUrl() : Config.ApiUrl;
  return Agent;
}

FAgent AgentFactory::FromSoul(const FSoul &Soul, const FString &ApiUrl) {
  FAgent Agent;
  Agent.Id = Soul.Id;
  Agent.Persona = Soul.Persona;
  Agent.State = Soul.State;
  Agent.Memories = Soul.Memories;
  Agent.ApiUrl =
      ApiUrl.IsEmpty() ? SDKConfig::GetApiUrl() : ApiUrl;
  return Agent;
}

// ==========================================
// AGENT OPERATIONS — Pure free functions
// ==========================================

FAgent AgentOps::WithState(const FAgent &Agent, const FAgentState &NewState) {
  FAgent Updated = Agent;
  Updated.State = NewState;
  return Updated;
}

FAgent AgentOps::WithMemories(const FAgent &Agent,
                              const TArray<FMemoryItem> &NewMemories) {
  FAgent Updated = Agent;
  Updated.Memories = NewMemories;
  return Updated;
}

FAgentState AgentOps::CalculateNewState(const FAgentState &Current,
                                        const FAgentState &Updates) {
  // Generic State Update Strategy:
  // Since we are using raw JSON, we can either replace or merge.
  // For this FP implementation without a heavy JSON library dependency here,
  // we will assume the API returns the FULL new state in Updates.JsonData.
  // If Updates.JsonData is empty/invalid, keep Current.

  if (Updates.JsonData.IsEmpty() || Updates.JsonData == TEXT("{}")) {
    return Current;
  }

  return TypeFactory::AgentState(Updates.JsonData);
}

#include "RuntimeStore.h"
#include "Protocol/ProtocolThunks.h"

AgentTypes::AsyncResult<FAgentResponse>
AgentOps::Process(const FAgent &Agent, const FString &Input,
                  const TMap<FString, FString> &Context) {
  return AgentTypes::AsyncResult<FAgentResponse>::create(
      [Agent, Input, Context](std::function<void(FAgentResponse)> resolve,
                              std::function<void(std::string)> reject) {
        SDKConfig::SetApiConfig(Agent.ApiUrl, SDKConfig::GetApiKey());
        auto Store = MakeShared<rtk::EnhancedStore<FStoreState>>(createStore());

        Store->dispatch(rtk::processNPC(Agent.Id, Input,
                                        SerializeContextMap(Context),
                                        Agent.Persona, Agent.State,
                                        rtk::LocalProtocolRuntime()))
            .then([resolve, Store](const FAgentResponse &Response) {
              resolve(Response);
            })
            .catch_([reject, Store](std::string Error) { reject(Error); })
            .execute();
      });
}

FSoul AgentOps::Export(const FAgent &Agent) {
  return TypeFactory::Soul(Agent.Id, TEXT("1.0.0"), TEXT("NPC"),
                           Agent.Persona, Agent.State, Agent.Memories);
}
