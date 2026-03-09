#include "RuntimeSubsystem.h"
#include "NPC/NPCSlice.h"
#include "RuntimeConfig.h"
#include "RuntimeStore.h"
#include "Protocol/ProtocolThunks.h"
#include "Soul/SoulThunks.h"

void UForbocAISDKSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  // Initialize the store with a listener for actions
  std::vector<rtk::Middleware<FSDKState>> Middlewares;
  Middlewares.push_back(createNpcRemovalListener());

  // Action broadcast middleware
  Middlewares.push_back([this](const rtk::MiddlewareApi<FSDKState> &Api)
                            -> std::function<rtk::Dispatcher(rtk::Dispatcher)> {
    return [this](rtk::Dispatcher Next) -> rtk::Dispatcher {
      return [this, Next](const rtk::AnyAction &Action) {
        this->HandleAction(Action);
        return Next(Action);
      };
    };
  });

  SDKStore = MakeShared<rtk::EnhancedStore<FSDKState>>(
      rtk::configureStore<FSDKState>(&SDKReducer, FSDKState(), Middlewares));
}

void UForbocAISDKSubsystem::Deinitialize() {
  SDKStore.Reset();
  Super::Deinitialize();
}

void UForbocAISDKSubsystem::InitSDK(FString ApiKey, FString ApiUrl) {
  SDKConfig::SetApiConfig(ApiUrl, ApiKey);
}

void UForbocAISDKSubsystem::ProcessNPC(FString NpcId, FString Input) {
  if (!SDKStore.IsValid())
    return;

  OnTypingStart.Broadcast();

  SDKStore->dispatch(
              rtk::processNPC(NpcId, Input, TEXT("{}"), TEXT(""),
                              FAgentState(), rtk::LocalProtocolRuntime()))
      .then([this](const FAgentResponse &Result) {
        if (!Result.Dialogue.IsEmpty()) {
          OnMessageReceived.Broadcast(Result.Dialogue);
        }
        if (!Result.Action.Type.IsEmpty()) {
          OnNPCActionReceived.Broadcast(Result.Action);
        }
        OnTypingEnd.Broadcast();
      })
      .catch_([this](std::string Error) {
        static_cast<void>(Error);
        OnTypingEnd.Broadcast();
      })
      .execute();
}

void UForbocAISDKSubsystem::ExportSoul(FString AgentId) {
  if (!SDKStore.IsValid())
    return;

  SDKStore->dispatch(rtk::exportSoulThunk(AgentId))
      .then([this](const FSoulExportResult &Result) {
        OnSoulExportComplete.Broadcast(Result.TxId);
      })
      .execute();
}

FAgentState UForbocAISDKSubsystem::GetNPCState(FString NpcId) const {
  if (!SDKStore.IsValid())
    return FAgentState();

  auto State = SDKStore->getState();
  auto NPC = NPCSlice::SelectNPCById(State.NPCs, NpcId);
  if (NPC.hasValue) {
    return NPC.value.State;
  }
  return FAgentState();
}

void UForbocAISDKSubsystem::HandleAction(const rtk::AnyAction &Action) {
  if (NPCSlice::Actions::SetLastActionActionCreator().match(Action)) {
    const auto Payload =
        NPCSlice::Actions::SetLastActionActionCreator().extract(Action);
    if (Payload.hasValue) {
      OnNPCActionReceived.Broadcast(Payload.value.Action);
    }
  }
}
