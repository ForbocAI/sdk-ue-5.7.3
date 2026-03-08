#include "Subsystem.h"
#include "NPC/NPCSlice.h"
#include "SDKStore.h"
#include "Thunks.h"

void UForbocAISDKSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  // Initialize the store with a listener for actions
  std::vector<rtk::Middleware<FSDKState>> Middlewares;
  Middlewares.push_back(rtk::createNpcRemovalListener());

  // Action broadcast middleware
  Middlewares.push_back([this](auto dispatch, auto getState) {
    return [this](auto next) {
      return [this, next](const rtk::AnyAction &action) {
        this->HandleAction(action);
        return next(action);
      };
    };
  });

  SDKStore = MakeShared<rtk::EnhancedStore<FSDKState>>(
      rtk::configureStore<FSDKState>(SDKReducer, FSDKState(), Middlewares));
}

void UForbocAISDKSubsystem::Deinitialize() {
  SDKStore.Reset();
  Super::Deinitialize();
}

void UForbocAISDKSubsystem::InitSDK(FString ApiKey, FString ApiUrl) {
  // Config logic would go here
}

void UForbocAISDKSubsystem::ProcessNPC(FString NpcId) {
  if (!SDKStore.IsValid())
    return;

  rtk::processNPC(NpcId)(
      [this](auto action) { return SDKStore->dispatch(action); },
      [this]() { return SDKStore->getState(); })
      .then([this](FString Result) {
        // Handle result
      });
}

void UForbocAISDKSubsystem::ExportSoul(FString AgentId) {
  if (!SDKStore.IsValid())
    return;

  rtk::exportSoulThunk(
      AgentId, {[this](auto action) { return SDKStore->dispatch(action); },
                [this]() { return SDKStore->getState(); }})
      .then([this](FString TxId) { OnSoulExportComplete.Broadcast(TxId); });
}

FAgentState UForbocAISDKSubsystem::GetNPCState(FString NpcId) const {
  if (!SDKStore.IsValid())
    return FAgentState();

  auto State = SDKStore->getState();
  auto NPC = NPCSlice::GetNPCAdapter().selectById(State.NPCs.Entities, NpcId);
  if (NPC.hasValue) {
    return NPC.value.State;
  }
  return FAgentState();
}

void UForbocAISDKSubsystem::HandleAction(const rtk::AnyAction &Action) {
  if (Action.Type == NPCSlice::SetLastActionAction::Type) {
    // We need to extract the payload.
    // In this implementation, we'll assume the action creator is available
    // or just use the raw payload pointer if we know the structure.
    auto PayloadPtr = static_cast<NPCSlice::SetLastActionAction::FPayload *>(
        Action.PayloadWrapper.get());
    if (PayloadPtr) {
      OnNPCActionReceived.Broadcast(PayloadPtr->Action);
    }
  }
}
