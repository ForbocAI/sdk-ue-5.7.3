#include "NPC/NPCModule.h"
#include "Containers/UnrealString.h"
#include "Core/functional_core.hpp"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Guid.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

// ==========================================
// AGENT FACTORY — Pure construction functions
// ==========================================

AgentTypes::AgentCreationResult
AgentFactory::Create(const FAgentConfig &Config) {
  try {
    const FString NewId =
        FString::Printf(TEXT("agent_%s"), *FGuid::NewGuid().ToString());
    const FString Url =
        Config.ApiUrl.IsEmpty() ? TEXT("http://localhost:8080") : Config.ApiUrl;

    FAgent agent{NewId, Config.Persona, FAgentState(), TArray<FMemoryItem>(),
                 Url};
    return AgentTypes::make_right(FString(), agent);
  } catch (const std::exception &e) {
    return AgentTypes::make_left(FString(e.what()));
  }
}

AgentTypes::AgentCreationResult AgentFactory::FromSoul(const FSoul &Soul,
                                                       const FString &ApiUrl) {
  try {
    const FString Url =
        ApiUrl.IsEmpty() ? TEXT("http://localhost:8080") : ApiUrl;
    FAgent agent{Soul.Id, Soul.Persona, Soul.State, Soul.Memories, Url};
    return AgentTypes::make_right(FString(), agent);
  } catch (const std::exception &e) {
    return AgentTypes::make_left(FString(e.what()));
  }
}

// ==========================================
// AGENT OPERATIONS — Pure free functions
// ==========================================

FAgent AgentOps::WithState(const FAgent &Agent, const FAgentState &NewState) {
  return FAgent{Agent.Id, Agent.Persona, NewState, Agent.Memories,
                Agent.ApiUrl};
}

FAgent AgentOps::WithMemories(const FAgent &Agent,
                              const TArray<FMemoryItem> &NewMemories) {
  return FAgent{Agent.Id, Agent.Persona, Agent.State, NewMemories,
                Agent.ApiUrl};
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

#include "SDKStore.h"
#include "Thunks.h"

AgentTypes::AsyncResult<FAgentResponse>
AgentOps::Process(const FAgent &Agent, const FString &Input,
                  const TMap<FString, FString> &Context) {
  return AgentTypes::AsyncResult<FAgentResponse>::create(
      [Agent, Input, Context](std::function<void(FAgentResponse)> resolve,
                              std::function<void(std::string)> reject) {
        // Use the global SDK Store to dispatch the processNPC thunk
        auto Store = ConfigureSDKStore();

        // Dispatch the thunk with the initial input
        auto ThunkResult = processNPC(Agent.Id, Input)(
            [Store](const AnyAction &a) { return Store.dispatch(a); },
            [Store]() { return Store.getState(); });

        // Chain the result back to FAgentResponse
        func::AsyncChain::then<FString, void>(
            ThunkResult,
            [resolve, Agent](FString FinalVerdict) {
              FAgentResponse Res;
              Res.GeneratedDialogue = FinalVerdict;
              Res.Action =
                  TypeFactory::Action(TEXT("Dialogue"), TEXT("Character"));
              Res.Signature = TEXT("RTK_SIGNED");
              resolve(Res);
            })
            .catch_([reject](std::string Error) { reject(Error); });
      });
}

FSoul AgentOps::Export(const FAgent &Agent) {
  return TypeFactory::Soul(Agent.Id, TEXT("1.0.0"), TEXT("Agent"),
                           Agent.Persona, Agent.State, Agent.Memories);
}

// Functional helper implementations
namespace AgentHelpers {
// Implementation of lazy agent creation
AgentTypes::Lazy<FAgent> createLazyAgent(const FAgentConfig &config) {
  return func::lazy(
      [config]() -> FAgent { return AgentFactory::Create(config); });
}

// Implementation of agent state validation pipeline
AgentTypes::ValidationPipeline<FAgentState> agentStateValidationPipeline() {
  return func::validationPipeline<FAgentState>()
      .add([](const FAgentState &state)
               -> AgentTypes::Either<FString, FAgentState> {
        if (state.JsonData.IsEmpty() || state.JsonData == TEXT("{}")) {
          return AgentTypes::make_left(FString(TEXT("Empty agent state")));
        }
        return AgentTypes::make_right(state);
      })
      .add([](const FAgentState &state)
               -> AgentTypes::Either<FString, FAgentState> {
        // Add more validation rules here
        return AgentTypes::make_right(state);
      });
}

// Implementation of agent processing pipeline
AgentTypes::Pipeline<FAgent> agentProcessingPipeline(const FAgent &agent) {
  return func::pipe(agent);
}

// Implementation of curried agent creation
AgentTypes::Curried<
    1, std::function<AgentTypes::AgentCreationResult(FAgentConfig)>>
curriedAgentCreation() {
  return func::curry<1>(
      [](FAgentConfig config) -> AgentTypes::AgentCreationResult {
        try {
          FAgent agent = AgentFactory::Create(config);
          return AgentTypes::make_right(FString(), agent);
        } catch (const std::exception &e) {
          return AgentTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace AgentHelpers