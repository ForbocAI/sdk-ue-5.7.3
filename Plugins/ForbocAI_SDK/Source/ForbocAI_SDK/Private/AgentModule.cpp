#include "AgentModule.h"
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

AgentTypes::AsyncResult<FAgentResponse>
AgentOps::Process(const FAgent &Agent, const FString &Input,
                  const TMap<FString, FString> &Context) {
  return AgentTypes::AsyncResult<FAgentResponse>::create(
      [Agent, Input, Context](std::function<void(FAgentResponse)> resolve,
                              std::function<void(std::string)> reject) {
        // =========================================================
        // 7-Step Async Pipeline (Neuro-Symbolic Protocol)
        // =========================================================

        // 1. OBSERVE: Pack Context into JSON
        TSharedPtr<FJsonObject> ContextJson = MakeShareable(new FJsonObject);
        for (const auto &Elem : Context) {
          ContextJson->SetStringField(Elem.Key, Elem.Value);
        }

        // Wrap in Directive Request structure
        TSharedPtr<FJsonObject> DirectiveReq = MakeShareable(new FJsonObject);
        TArray<TSharedPtr<FJsonValue>> ContextArray;
        for (const auto &Elem : Context) {
          TArray<TSharedPtr<FJsonValue>> Pair;
          Pair.Add(MakeShareable(new FJsonValueString(Elem.Key)));
          Pair.Add(MakeShareable(new FJsonValueString(Elem.Value)));
          ContextArray.Add(MakeShareable(new FJsonValueArray(Pair)));
        }
        DirectiveReq->SetArrayField(TEXT("dirContext"), ContextArray);

        FString DirectiveBody;
        TSharedRef<TJsonWriter<>> Writer =
            TJsonWriterFactory<>::Create(&DirectiveBody);
        FJsonSerializer::Serialize(DirectiveReq.ToSharedRef(), Writer);

        // 2. DIRECTIVE REQUEST (Step 2)
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DirRequest =
            FHttpModule::Get().CreateRequest();
        DirRequest->SetURL(Agent.ApiUrl + TEXT("/agents/") + Agent.Id +
                           TEXT("/directive"));
        DirRequest->SetVerb(TEXT("POST"));
        DirRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        DirRequest->SetHeader(TEXT("Authorization"),
                              TEXT("Bearer sk_test_key")); // Mock key
        DirRequest->SetContentAsString(DirectiveBody);

        DirRequest->OnProcessRequestComplete().BindLambda(
            [Agent, Input, resolve, reject](FHttpRequestPtr Request,
                                            FHttpResponsePtr Response,
                                            bool bSuccess) {
              if (!bSuccess || !Response.IsValid() ||
                  Response->GetResponseCode() != 200) {
                // Fallback / Error
                // In functional style, we reject on protocol failure
                reject("Error connecting to Mind or Network Error");
                return;
              }

              // Parse Directive
              TSharedPtr<FJsonObject> DirJson;
              TSharedRef<TJsonReader<>> Reader =
                  TJsonReaderFactory<>::Create(Response->GetContentAsString());

              FString Instruction = TEXT("IDLE");
              FString DirectiveReason = TEXT("No directive");
              FString Target = TEXT("");

              if (FJsonSerializer::Deserialize(Reader, DirJson)) {
                Instruction = DirJson->GetStringField(TEXT("dirInstruction"));
                DirectiveReason = DirJson->GetStringField(TEXT("dirReason"));
                Target = DirJson->GetStringField(TEXT("dirTarget"));
              }

              // 3. GENERATE (Local SLM Simulation) (Step 3)
              FString GeneratedDialogue =
                  FString::Printf(TEXT("I will %s because %s."), *Instruction,
                                  *DirectiveReason);
              FAgentAction Action = TypeFactory::Action(Instruction, Target);

              // 4. BUNDLE VERDICT REQUEST (Step 4)

              // Prepare Verdict Payload
              TSharedPtr<FJsonObject> VerdictReq =
                  MakeShareable(new FJsonObject);

              // Re-construct Directive Response inside (required by API
              // signature)
              VerdictReq->SetObjectField(TEXT("verDirective"), DirJson);

              // Construct Action
              TSharedPtr<FJsonObject> ActionJson =
                  MakeShareable(new FJsonObject);
              ActionJson->SetStringField(TEXT("gaType"), Action.Type);
              ActionJson->SetStringField(TEXT("actionTarget"), Action.Target);

              VerdictReq->SetObjectField(TEXT("verAction"), ActionJson);
              VerdictReq->SetStringField(TEXT("verThought"), GeneratedDialogue);

              FString VerdictBody;
              TSharedRef<TJsonWriter<>> VWriter =
                  TJsonWriterFactory<>::Create(&VerdictBody);
              FJsonSerializer::Serialize(VerdictReq.ToSharedRef(), VWriter);

              // 5. VERDICT REQUEST (Step 5)
              TSharedRef<IHttpRequest, ESPMode::ThreadSafe> VerRequest =
                  FHttpModule::Get().CreateRequest();
              VerRequest->SetURL(Agent.ApiUrl + TEXT("/agents/") + Agent.Id +
                                 TEXT("/verdict"));
              VerRequest->SetVerb(TEXT("POST"));
              VerRequest->SetHeader(TEXT("Content-Type"),
                                    TEXT("application/json"));
              VerRequest->SetHeader(TEXT("Authorization"),
                                    TEXT("Bearer sk_test_key"));
              VerRequest->SetContentAsString(VerdictBody);

              VerRequest->OnProcessRequestComplete().BindLambda(
                  [GeneratedDialogue, Action, resolve,
                   reject](FHttpRequestPtr VReq, FHttpResponsePtr VRes,
                           bool bVSuccess) {
                    FString Signature = TEXT("unsigned");
                    bool bValid = false;

                    if (bVSuccess && VRes.IsValid() &&
                        VRes->GetResponseCode() == 200) {
                      TSharedPtr<FJsonObject> VerJson;
                      TSharedRef<TJsonReader<>> VReader =
                          TJsonReaderFactory<>::Create(
                              VRes->GetContentAsString());

                      if (FJsonSerializer::Deserialize(VReader, VerJson)) {
                        bValid = VerJson->GetBoolField(TEXT("verValid"));
                        Signature =
                            VerJson->GetStringField(TEXT("verSignature"));
                      }
                    }

                    // 6. SIGN & EXECUTE (Step 6 & 7)
                    if (!bValid) {
                      // If invalid, we return a BLOCKED response.
                      resolve(
                          FAgentResponse{TEXT("..."),
                                         TypeFactory::Action(TEXT("BLOCKED"),
                                                             TEXT("Protocol")),
                                         TEXT("Blocked by Protocol")});
                    } else {
                      // Return Validated Action
                      resolve(FAgentResponse{
                          GeneratedDialogue, Action,
                          FString::Printf(TEXT("Signed: %s"), *Signature)});
                    }
                  });

              VerRequest->ProcessRequest();
            });

        DirRequest->ProcessRequest();
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