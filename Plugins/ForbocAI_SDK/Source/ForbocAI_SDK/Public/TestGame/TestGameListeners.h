#pragma once
// Test-game cross-slice listener middleware — mirrors TS listeners.ts
// Reactive side effects: NPC movement → UI message

#include "CoreMinimal.h"
#include "Core/rtk.hpp"
#include "TestGame/TestGameSlices.h"
#include "TestGame/TestGameStore.h"

namespace TestGame {

/**
 * Creates listener middleware that logs NPC movement to the UI message box.
 * Mirrors TS: npcsActions.moveNPC → uiActions.addMessage
 */
inline rtk::Middleware<FTestGameState> createGameListenerMiddleware() {
  return [](const rtk::MiddlewareApi<FTestGameState> &Api)
             -> std::function<rtk::Dispatcher(rtk::Dispatcher)> {
    return [Api](rtk::Dispatcher Next) -> rtk::Dispatcher {
      return [Api, Next](const rtk::AnyAction &Action) -> rtk::AnyAction {
        // Let reducers run first
        const rtk::AnyAction Result = Next(Action);

        // React to NPC movement
        if (NPCsActions::MoveNPCActionCreator().match(Action)) {
          auto Payload = NPCsActions::MoveNPCActionCreator().extract(Action);
          if (Payload.hasValue) {
            const auto &State = Api.getState();
            auto MaybeNpc = GetNPCAdapter().getSelectors().selectById(
                State.NPCs.Entities, Payload.value.Id);
            if (MaybeNpc.hasValue) {
              FString Msg = FString::Printf(
                  TEXT("%s moved to %d,%d"), *MaybeNpc.value.Name,
                  Payload.value.Position.X, Payload.value.Position.Y);
              Api.dispatch(UIActions::AddMessage(Msg));
            }
          }
        }

        // React to NPC verdict application
        if (NPCsActions::ApplyNpcVerdictActionCreator().match(Action)) {
          auto Payload =
              NPCsActions::ApplyNpcVerdictActionCreator().extract(Action);
          if (Payload.hasValue) {
            const auto &State = Api.getState();
            auto MaybeNpc = GetNPCAdapter().getSelectors().selectById(
                State.NPCs.Entities, Payload.value.Id);
            if (MaybeNpc.hasValue) {
              FString Msg = FString::Printf(
                  TEXT("Verdict applied to %s (action: %s)"),
                  *MaybeNpc.value.Name, *Payload.value.ActionType);
              Api.dispatch(UIActions::AddMessage(Msg));
            }
          }
        }

        return Result;
      };
    };
  };
}

/**
 * Creates a test-game store with listener middleware registered.
 * This is the primary factory for game runs.
 */
inline rtk::EnhancedStore<FTestGameState> createTestGameStoreWithListeners() {
  std::vector<rtk::Middleware<FTestGameState>> Middlewares;
  Middlewares.push_back(createGameListenerMiddleware());
  return rtk::configureStore<FTestGameState>(&TestGameReducer,
                                             FTestGameState(), Middlewares);
}

} // namespace TestGame
