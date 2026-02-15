#include "AgentModule.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

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

FAgentResponse AgentOps::Process(const FAgent &Agent, const FString &Input,
                                 const TMap<FString, FString> &Context) {
  // =========================================================
  // 7-Step Neuro-Symbolic Protocol (Simulated Synchronous)
  // =========================================================

  // 1. OBSERVE: Pack Context into Directive Request
  // TODO: Use TJsonWriter to serialize Context to JSON
  FString DirectiveReqJson = TEXT("{}");

  // 2. DIRECTIVE: API Call (SDK -> API)
  // POST /agents/{id}/directive
  // TODO: FHttpModule::CreateRequest() ...
  // Simulation:
  FString Directive = TEXT("Autonomous behavior");
  FString Instruction = TEXT("IDLE");

  if (Context.Contains(TEXT("hp")) &&
      FCString::Atoi(*Context[TEXT("hp")]) < 20) {
    // NOTE: This fallback logic should be injected via Config/Delegate, not
    // hardcoded. Keeping generic placeholder for now.
    Directive = TEXT("Survival Instinct");
    Instruction = TEXT("FLEE");
  }

  // 3. GENERATE: Local Inference (SDK -> Cortex)
  // Using the Directive to guide the SLM
  FString Prompt = FString::Printf(TEXT("System: %s (%s)\nUser: %s"),
                                   *Instruction, *Directive, *Input);
  FString GeneratedContent =
      FString::Printf(TEXT("I am following instruction: %s"), *Instruction);

  // 4. BUNDLE: Prepare Verdict Request
  // Include Directive, Action, and Generated Content

  // 5. VERDICT: API Call (SDK -> API)
  // POST /agents/{id}/verdict
  // TODO: FHttpModule::CreateRequest() ...

  // 6. SIGN: Receive Validity and Signature
  FString Signature = TEXT("sig_simulated_valid");
  bool bValid = true;

  // 7. EXECUTE: Return Executable Packet
  if (!bValid) {
    return FAgentResponse{
        TEXT("..."),
        TypeFactory::Action(TEXT("BLOCKED"), TEXT("Protocol Violation")),
        TEXT("Blocked")};
  }

  FString Thought =
      FString::Printf(TEXT("Directive: %s | Sig: %s"), *Directive, *Signature);

  return FAgentResponse{GeneratedContent,
                        TypeFactory::Action(Instruction, Directive), Thought};
}

FSoul AgentOps::Export(const FAgent &Agent) {
  return TypeFactory::Soul(Agent.Id, TEXT("1.0.0"), TEXT("Agent"),
                           Agent.Persona, Agent.State, Agent.Memories);
}
