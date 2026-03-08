#pragma once

#include "APISlice.h"
#include "BridgeSlice.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "CortexSlice.h"
#include "DirectiveSlice.h"
#include "GhostSlice.h"
#include "MemorySlice.h"
#include "NPCSlice.h"
#include "SoulSlice.h"
#include "Thunks.h"

// ===================================
// Root State Configuration
// ===================================

/**
 * Combined SDK State — Unifies all slices.
 */
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

/**
 * Root Reducer: Combines all slice reducers into a single functional unit.
 */
inline FSDKState SDKReducer(FSDKState State, const rtk::AnyAction &Action) {
  State.NPCs = NPCSlice::CreateNPCSlice().reducer(State.NPCs, Action);
  State.Memory = MemorySlice::CreateMemorySlice().reducer(State.Memory, Action);
  State.Directives =
      DirectiveSlice::CreateDirectiveSlice().reducer(State.Directives, Action);
  State.API = APISlice::CreateAPISlice().reducer(State.API, Action);

  State.Cortex = CortexSlice::CreateCortexSlice().reducer(State.Cortex, Action);
  State.Soul = SoulSlice::CreateSoulSlice().reducer(State.Soul, Action);
  State.Ghost = GhostSlice::CreateGhostSlice().reducer(State.Ghost, Action);
  State.API = APISlice::CreateAPISlice().reducer(State.API, Action);
  return State;
}

// --- Store Creation ---
/**
 * Factory for the SDK Store.
 * Configures reducers, middleware (Thunk, Listeners), and preloaded state.
 */
inline rtk::EnhancedStore<FSDKState> createSDKStore(
    rtk::Maybe<FSDKState> PreloadedState = rtk::nothing<FSDKState>()) {
  std::vector<rtk::Middleware<FSDKState>> Middlewares;
  Middlewares.push_back(rtk::createNpcRemovalListener());

  return rtk::configureStore<FSDKState>(
      SDKReducer, PreloadedState.hasValue ? PreloadedState.value : FSDKState(),
      Middlewares);
}

/**
 * Store Configuration: Creates and configures the global SDK Store.
 */
inline rtk::Store<FSDKState> ConfigureSDKStore() {
  // Use a static instance for the store to persist state across calls
  static rtk::Store<FSDKState> GlobalStore =
      rtk::configureStore<FSDKState>(&SDKReducer, FSDKState());
  return GlobalStore;
}
