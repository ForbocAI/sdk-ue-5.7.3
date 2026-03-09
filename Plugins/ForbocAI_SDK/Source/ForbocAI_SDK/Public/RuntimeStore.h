#pragma once

#include "API/APISlice.h"
#include "Bridge/BridgeSlice.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Cortex/CortexSlice.h"
#include "DirectiveSlice.h"
#include "Ghost/GhostSlice.h"
#include "Memory/MemorySlice.h"
#include "NPC/NPCSlice.h"
#include "Soul/SoulSlice.h"

struct FSDKState {
  NPCSlice::FNPCSliceState NPCs;
  MemorySlice::FMemorySliceState Memory;
  DirectiveSlice::FDirectiveSliceState Directives;
  BridgeSlice::FBridgeSliceState Bridge;
  CortexSlice::FCortexSliceState Cortex;
  SoulSlice::FSoulSliceState Soul;
  GhostSlice::FGhostSliceState Ghost;
  APISlice::FAPIState API;
};

namespace SDKStoreInternal {

inline const rtk::Slice<NPCSlice::FNPCSliceState> &GetNPCSlice() {
  static const rtk::Slice<NPCSlice::FNPCSliceState> Slice =
      NPCSlice::CreateNPCSlice();
  return Slice;
}

inline const rtk::Slice<MemorySlice::FMemorySliceState> &GetMemorySlice() {
  static const rtk::Slice<MemorySlice::FMemorySliceState> Slice =
      MemorySlice::CreateMemorySlice();
  return Slice;
}

inline const rtk::Slice<DirectiveSlice::FDirectiveSliceState> &
GetDirectiveSlice() {
  static const rtk::Slice<DirectiveSlice::FDirectiveSliceState> Slice =
      DirectiveSlice::CreateDirectiveSlice();
  return Slice;
}

inline const rtk::Slice<BridgeSlice::FBridgeSliceState> &GetBridgeSlice() {
  static const rtk::Slice<BridgeSlice::FBridgeSliceState> Slice =
      BridgeSlice::CreateBridgeSlice();
  return Slice;
}

inline const rtk::Slice<CortexSlice::FCortexSliceState> &GetCortexSlice() {
  static const rtk::Slice<CortexSlice::FCortexSliceState> Slice =
      CortexSlice::CreateCortexSlice();
  return Slice;
}

inline const rtk::Slice<SoulSlice::FSoulSliceState> &GetSoulSlice() {
  static const rtk::Slice<SoulSlice::FSoulSliceState> Slice =
      SoulSlice::CreateSoulSlice();
  return Slice;
}

inline const rtk::Slice<GhostSlice::FGhostSliceState> &GetGhostSlice() {
  static const rtk::Slice<GhostSlice::FGhostSliceState> Slice =
      GhostSlice::CreateGhostSlice();
  return Slice;
}

inline const rtk::Slice<APISlice::FAPIState> &GetAPISlice() {
  static const rtk::Slice<APISlice::FAPIState> Slice =
      APISlice::CreateAPISlice();
  return Slice;
}

} // namespace SDKStoreInternal

inline FSDKState SDKReducer(const FSDKState &State,
                            const rtk::AnyAction &Action) {
  FSDKState Next = State;
  Next.NPCs = SDKStoreInternal::GetNPCSlice().Reducer(State.NPCs, Action);
  Next.Memory = SDKStoreInternal::GetMemorySlice().Reducer(State.Memory, Action);
  Next.Directives =
      SDKStoreInternal::GetDirectiveSlice().Reducer(State.Directives, Action);
  Next.Bridge =
      SDKStoreInternal::GetBridgeSlice().Reducer(State.Bridge, Action);
  Next.Cortex =
      SDKStoreInternal::GetCortexSlice().Reducer(State.Cortex, Action);
  Next.Soul = SDKStoreInternal::GetSoulSlice().Reducer(State.Soul, Action);
  Next.Ghost = SDKStoreInternal::GetGhostSlice().Reducer(State.Ghost, Action);
  Next.API = SDKStoreInternal::GetAPISlice().Reducer(State.API, Action);
  return Next;
}

inline rtk::Middleware<FSDKState> createNpcRemovalListener() {
  return [](const rtk::MiddlewareApi<FSDKState> &Api)
             -> std::function<rtk::Dispatcher(rtk::Dispatcher)> {
    return [Api](rtk::Dispatcher Next) -> rtk::Dispatcher {
      return [Api, Next](const rtk::AnyAction &Action) -> rtk::AnyAction {
        const FString ActiveNpcIdBefore = Api.getState().NPCs.ActiveNpcId;
        const rtk::AnyAction Result = Next(Action);

        if (NPCSlice::Actions::RemoveNPCActionCreator().match(Action)) {
          const auto RemovedNpcId =
              NPCSlice::Actions::RemoveNPCActionCreator().extract(Action);
          if (RemovedNpcId.hasValue) {
            Api.dispatch(
                DirectiveSlice::Actions::ClearDirectivesForNpc(RemovedNpcId.value));
            Api.dispatch(BridgeSlice::Actions::ClearBridgeValidation());
            Api.dispatch(GhostSlice::Actions::ClearGhostSession());
            Api.dispatch(SoulSlice::Actions::ClearSoulState());
            Api.dispatch(NPCSlice::Actions::ClearBlock(RemovedNpcId.value));

            if (RemovedNpcId.value == ActiveNpcIdBefore) {
              Api.dispatch(MemorySlice::Actions::MemoryClear());
            }
          }
        }

        return Result;
      };
    };
  };
}

inline rtk::EnhancedStore<FSDKState>
createSDKStore(func::Maybe<FSDKState> PreloadedState =
                   func::nothing<FSDKState>()) {
  std::vector<rtk::Middleware<FSDKState>> Middlewares;
  Middlewares.push_back(createNpcRemovalListener());

  return rtk::configureStore<FSDKState>(
      &SDKReducer,
      PreloadedState.hasValue ? PreloadedState.value : FSDKState(),
      Middlewares);
}

inline rtk::EnhancedStore<FSDKState> ConfigureSDKStore() {
  static rtk::EnhancedStore<FSDKState> GlobalStore = createSDKStore();
  return GlobalStore;
}
