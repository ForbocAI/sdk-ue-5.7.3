#include "RuntimeSubsystem.h"
#include "NPC/NPCSlice.h"
#include "RuntimeConfig.h"
#include "RuntimeStore.h"
#include "Protocol/ProtocolThunks.h"
#include "Soul/SoulThunks.h"

void UForbocAISubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  // Initialize the store with a listener for actions
  std::vector<rtk::Middleware<FStoreState>> Middlewares;
  Middlewares.push_back(createNpcRemovalListener());

  // Action broadcast middleware
  Middlewares.push_back([this](const rtk::MiddlewareApi<FStoreState> &Api)
                            -> std::function<rtk::Dispatcher(rtk::Dispatcher)> {
    return [this](rtk::Dispatcher Next) -> rtk::Dispatcher {
      return [this, Next](const rtk::AnyAction &Action) {
        this->HandleAction(Action);
        return Next(Action);
      };
    };
  });

  Store = MakeShared<rtk::EnhancedStore<FStoreState>>(
      rtk::configureStore<FStoreState>(&StoreReducer, FStoreState(),
                                       Middlewares));
}

void UForbocAISubsystem::Deinitialize() {
  Store.Reset();
  Super::Deinitialize();
}

void UForbocAISubsystem::Init(FString ApiKey, FString ApiUrl) {
  SDKConfig::SetApiConfig(ApiUrl, ApiKey);
}

void UForbocAISubsystem::ProcessNPC(FString NpcId, FString Input) {
  if (!Store.IsValid())
    return;

  OnTypingStart.Broadcast();

  Store->dispatch(
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

void UForbocAISubsystem::ExportSoul(FString AgentId) {
  if (!Store.IsValid())
    return;

  Store->dispatch(rtk::exportSoulThunk(AgentId))
      .then([this](const FSoulExportResult &Result) {
        OnSoulExportComplete.Broadcast(Result.TxId);
      })
      .execute();
}

FAgentState UForbocAISubsystem::GetNPCState(FString NpcId) const {
  if (!Store.IsValid())
    return FAgentState();

  auto State = Store->getState();
  auto NPC = NPCSlice::SelectNPCById(State.NPCs, NpcId);
  if (NPC.hasValue) {
    return NPC.value.State;
  }
  return FAgentState();
}

void UForbocAISubsystem::HandleAction(const rtk::AnyAction &Action) {
  if (NPCSlice::Actions::SetLastActionActionCreator().match(Action)) {
    const auto Payload =
        NPCSlice::Actions::SetLastActionActionCreator().extract(Action);
    if (Payload.hasValue) {
      OnNPCActionReceived.Broadcast(Payload.value.Action);
    }
  }
}
