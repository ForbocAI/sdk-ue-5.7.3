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

struct FStoreState {
  NPCSlice::FNPCSliceState NPCs;
  MemorySlice::FMemorySliceState Memory;
  DirectiveSlice::FDirectiveSliceState Directives;
  BridgeSlice::FBridgeSliceState Bridge;
  CortexSlice::FCortexSliceState Cortex;
  SoulSlice::FSoulSliceState Soul;
  GhostSlice::FGhostSliceState Ghost;
  APISlice::FAPIState API;
};

namespace StoreInternal {

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

} // namespace StoreInternal

inline FStoreState StoreReducer(const FStoreState &State,
                                const rtk::AnyAction &Action) {
  FStoreState Next = State;
  Next.NPCs = StoreInternal::GetNPCSlice().Reducer(State.NPCs, Action);
  Next.Memory = StoreInternal::GetMemorySlice().Reducer(State.Memory, Action);
  Next.Directives =
      StoreInternal::GetDirectiveSlice().Reducer(State.Directives, Action);
  Next.Bridge = StoreInternal::GetBridgeSlice().Reducer(State.Bridge, Action);
  Next.Cortex = StoreInternal::GetCortexSlice().Reducer(State.Cortex, Action);
  Next.Soul = StoreInternal::GetSoulSlice().Reducer(State.Soul, Action);
  Next.Ghost = StoreInternal::GetGhostSlice().Reducer(State.Ghost, Action);
  Next.API = StoreInternal::GetAPISlice().Reducer(State.API, Action);
  return Next;
}

inline rtk::Middleware<FStoreState> createNpcRemovalListener() {
  return [](const rtk::MiddlewareApi<FStoreState> &Api)
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

inline rtk::EnhancedStore<FStoreState>
createStore(func::Maybe<FStoreState> PreloadedState =
                func::nothing<FStoreState>()) {
  std::vector<rtk::Middleware<FStoreState>> Middlewares;
  Middlewares.push_back(createNpcRemovalListener());

  return rtk::configureStore<FStoreState>(
      &StoreReducer,
      PreloadedState.hasValue ? PreloadedState.value : FStoreState(),
      Middlewares);
}

inline rtk::EnhancedStore<FStoreState> ConfigureStore() {
  static rtk::EnhancedStore<FStoreState> GlobalStore = createStore();
  return GlobalStore;
}
