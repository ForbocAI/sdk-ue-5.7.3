#pragma once
// Test-game store composition — mirrors TS test-game/src/store.ts
// Combines 13 game slices into a single Redux store

#include "CoreMinimal.h"
#include "Core/rtk.hpp"
#include "TestGame/TestGameScenarios.h"
#include "TestGame/TestGameSlices.h"

namespace TestGame {

// -------------------------------------------------------------------------
// Root state — all 13 slice states
// -------------------------------------------------------------------------

struct FTestGameState {
  // Domain 1: Entities
  FNPCsSliceState NPCs;
  FPlayerState Player;

  // Domain 2: Mechanics
  FGridState Grid;
  FStealthState Stealth;
  FSocialState Social;
  FBridgeRulesState Bridge;

  // Domain 3: Store
  FGameMemorySliceState Memory;
  FInventoryState Inventory;
  FSoulTrackingState Soul;

  // Domain 4: Terminal
  FUIState UI;
  FTranscriptState Transcript;

  // Domain 5: Autoplay
  FScenarioSliceState Scenario;
  FHarnessState Harness;
};

// -------------------------------------------------------------------------
// Slice singletons
// -------------------------------------------------------------------------

namespace GameSlices {

inline const rtk::Slice<FNPCsSliceState> &NPCs() {
  static const auto S = CreateNPCsSlice();
  return S;
}
inline const rtk::Slice<FPlayerState> &Player() {
  static const auto S = CreatePlayerSlice();
  return S;
}
inline const rtk::Slice<FGridState> &Grid() {
  static const auto S = CreateGridSlice();
  return S;
}
inline const rtk::Slice<FStealthState> &Stealth() {
  static const auto S = CreateStealthSlice();
  return S;
}
inline const rtk::Slice<FSocialState> &Social() {
  static const auto S = CreateSocialSlice();
  return S;
}
inline const rtk::Slice<FBridgeRulesState> &Bridge() {
  static const auto S = CreateGameBridgeSlice();
  return S;
}
inline const rtk::Slice<FGameMemorySliceState> &Memory() {
  static const auto S = CreateGameMemorySlice();
  return S;
}
inline const rtk::Slice<FInventoryState> &Inventory() {
  static const auto S = CreateInventorySlice();
  return S;
}
inline const rtk::Slice<FSoulTrackingState> &Soul() {
  static const auto S = CreateGameSoulSlice();
  return S;
}
inline const rtk::Slice<FUIState> &UI() {
  static const auto S = CreateUISlice();
  return S;
}
inline const rtk::Slice<FTranscriptState> &Transcript() {
  static const auto S = CreateTranscriptSlice();
  return S;
}
inline const rtk::Slice<FScenarioSliceState> &Scenario() {
  static const auto S = CreateScenarioSlice();
  return S;
}
inline const rtk::Slice<FHarnessState> &Harness() {
  static const auto S = CreateHarnessSlice();
  return S;
}

} // namespace GameSlices

// -------------------------------------------------------------------------
// Root reducer
// -------------------------------------------------------------------------

inline FTestGameState TestGameReducer(const FTestGameState &State,
                                      const rtk::AnyAction &Action) {
  FTestGameState Next;
  Next.NPCs = GameSlices::NPCs().Reducer(State.NPCs, Action);
  Next.Player = GameSlices::Player().Reducer(State.Player, Action);
  Next.Grid = GameSlices::Grid().Reducer(State.Grid, Action);
  Next.Stealth = GameSlices::Stealth().Reducer(State.Stealth, Action);
  Next.Social = GameSlices::Social().Reducer(State.Social, Action);
  Next.Bridge = GameSlices::Bridge().Reducer(State.Bridge, Action);
  Next.Memory = GameSlices::Memory().Reducer(State.Memory, Action);
  Next.Inventory = GameSlices::Inventory().Reducer(State.Inventory, Action);
  Next.Soul = GameSlices::Soul().Reducer(State.Soul, Action);
  Next.UI = GameSlices::UI().Reducer(State.UI, Action);
  Next.Transcript =
      GameSlices::Transcript().Reducer(State.Transcript, Action);
  Next.Scenario = GameSlices::Scenario().Reducer(State.Scenario, Action);
  Next.Harness = GameSlices::Harness().Reducer(State.Harness, Action);
  return Next;
}

// -------------------------------------------------------------------------
// Store factory — fresh instance per game run (deterministic)
// -------------------------------------------------------------------------

inline rtk::EnhancedStore<FTestGameState> createTestGameStore() {
  std::vector<rtk::Middleware<FTestGameState>> Middlewares;
  // Listener middleware will be added via TestGameListeners.h
  return rtk::configureStore<FTestGameState>(&TestGameReducer,
                                             FTestGameState(), Middlewares);
}

} // namespace TestGame
