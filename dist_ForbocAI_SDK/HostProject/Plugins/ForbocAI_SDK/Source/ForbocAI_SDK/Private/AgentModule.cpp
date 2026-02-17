#include "AgentModule.h"
#include "Containers/UnrealString.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Guid.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

// ==========================================
// AGENT FACTORY — Pure construction functions
// ==========================================

FAgent AgentFactory::Create(const FAgentConfig &Config) {
  const FString NewId =
      FString::Printf(TEXT("agent_%s"), *FGuid::NewGuid().ToString());
  const FString Url =
      Config.ApiUrl.IsEmpty() ? TEXT("http://localhost:8080") : Config.ApiUrl;

  return FAgent{NewId, Config.Persona, Config.InitialState,
                TArray<FMemoryItem>(), Url};
}

FAgent AgentFactory::FromSoul(const FSoul &Soul, const FString &ApiUrl) {
  const FString Url = ApiUrl.IsEmpty() ? TEXT("http://localhost:8080") : ApiUrl;

  return FAgent{Soul.Id, Soul.Persona, Soul.State, Soul.Memories, Url};
}

// ==========================================
// AGENT OPERATIONS — Pure free functions
// ==========================================

// ==========================================
// AGENT OPS implementation
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

void AgentOps::Process(const FAgent &Agent, const FString &Input,
                       const TMap<FString, FString> &Context,
                       TFunction<void(FAgentResponse)> Callback) {
  // =========================================================
  // 7-Step Async Pipeline (Neuro-Symbolic Protocol)
  // =========================================================

  // 1. OBSERVE: Pack Context into JSON
  // API expects context as Maybe [(Text, Text)] -> Array of [Key, Value] arrays
  // or Array of Objects.
  // Let's use Array of Objects for clarity: [{"key": "k", "val": "v"}]
  // Actually, Haskell Aeson for `[(Text, Text)]` expects `[[ "key", "val" ], [
  // "key2", "val2" ]]` by default. Let's try simpler object serialization
  // first, assuming API can handle Map or we adjust API.
  // Correction: Haskell `FromJSON` for `[(Text, Text)]` usually expects an
  // Array of Arrays [[k,v]]. Let's stick to `object` in API if possible, but
  // `Types.hs` defines `context :: Maybe [(Text, Text)]`.
  // To be safe and compatible with standard JSON object `{"key": "value"}`, the
  // API type should probably be `Map Text Text` or `Object`.
  // For now, let's send a standard JSON Object and assume the API handles it
  // (Aeson `Map` support).

  TSharedPtr<FJsonObject> ContextJson = MakeShareable(new FJsonObject);
  for (const auto &Elem : Context) {
    ContextJson->SetStringField(Elem.Key, Elem.Value);
  }

  // Wrap in Directive Request structure
  TSharedPtr<FJsonObject> DirectiveReq = MakeShareable(new FJsonObject);
  // Note: API field is `dirContext`
  // We will serialize as an Array of Arrays to be safe for `[(Text, Text)]`
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
      [Agent, Input, Callback](FHttpRequestPtr Request,
                               FHttpResponsePtr Response, bool bSuccess) {
        if (!bSuccess || !Response.IsValid() ||
            Response->GetResponseCode() != 200) {
          // Fallback / Error
          Callback(FAgentResponse{
              TEXT("Error connecting to Mind."),
              TypeFactory::Action(TEXT("ERROR"), TEXT("NetFail")),
              TEXT("Network Error")});
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
        // Ideally: Call local LLM here using the Directive.
        // For now, we simulate the SLM adhering to the instruction.
        FString GeneratedDialogue = FString::Printf(
            TEXT("I will %s because %s."), *Instruction, *DirectiveReason);
        FAgentAction Action = TypeFactory::Action(Instruction, Target);

        // 4. BUNDLE VERDICT REQUEST (Step 4)

        // Prepare Verdict Payload
        TSharedPtr<FJsonObject> VerdictReq = MakeShareable(new FJsonObject);

        // Re-construct Directive Response inside (required by API signature)
        VerdictReq->SetObjectField(TEXT("verDirective"), DirJson);

        // Construct Action
        TSharedPtr<FJsonObject> ActionJson = MakeShareable(new FJsonObject);
        ActionJson->SetStringField(TEXT("gaType"), Action.Type);
        ActionJson->SetStringField(TEXT("actionTarget"), Action.Target);
        // Note: Field names must match Types.hs `GenericAction`
        // `data GenericAction = { gaType :: Text ... }`

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
        VerRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        VerRequest->SetHeader(TEXT("Authorization"),
                              TEXT("Bearer sk_test_key"));
        VerRequest->SetContentAsString(VerdictBody);

        VerRequest->OnProcessRequestComplete().BindLambda(
            [GeneratedDialogue, Action, Callback](
                FHttpRequestPtr VReq, FHttpResponsePtr VRes, bool bVSuccess) {
              FString Signature = TEXT("unsigned");
              bool bValid = false;

              if (bVSuccess && VRes.IsValid() &&
                  VRes->GetResponseCode() == 200) {
                TSharedPtr<FJsonObject> VerJson;
                TSharedRef<TJsonReader<>> VReader =
                    TJsonReaderFactory<>::Create(VRes->GetContentAsString());

                if (FJsonSerializer::Deserialize(VReader, VerJson)) {
                  bValid = VerJson->GetBoolField(TEXT("verValid"));
                  Signature = VerJson->GetStringField(TEXT("verSignature"));
                }
              }

              // 6. SIGN & EXECUTE (Step 6 & 7)
              if (!bValid) {
                // If invalid, we return a BLOCKED response.
                Callback(FAgentResponse{
                    TEXT("..."),
                    TypeFactory::Action(TEXT("BLOCKED"), TEXT("Protocol")),
                    TEXT("Blocked by Protocol")});
              } else {
                // Return Validated Action
                Callback(FAgentResponse{
                    GeneratedDialogue, Action,
                    FString::Printf(TEXT("Signed: %s"), *Signature)});
              }
            });

        VerRequest->ProcessRequest();
      });

  DirRequest->ProcessRequest();
}

FSoul AgentOps::Export(const FAgent &Agent) {
  return TypeFactory::Soul(Agent.Id, TEXT("1.0.0"), TEXT("Agent"),
                           Agent.Persona, Agent.State, Agent.Memories);
}
