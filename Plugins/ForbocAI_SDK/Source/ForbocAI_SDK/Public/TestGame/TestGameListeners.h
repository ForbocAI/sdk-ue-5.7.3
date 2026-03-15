#pragma once
/**
 * Test-game cross-slice listener middleware — mirrors TS listeners.ts
 * Reactive side effects: NPC movement → UI message
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

#include "CoreMinimal.h"
#include "Core/rtk.hpp"
#include "TestGame/TestGameSlices.h"
#include "TestGame/TestGameStore.h"

namespace TestGame {

/**
 * Creates listener middleware that logs NPC movement to the UI message box.
 * Mirrors TS: npcsActions.moveNPC → uiActions.addMessage
 * User Story: As test-game reactive UI, I need listener middleware so NPC
 * movement and verdict application emit readable terminal messages.
 */
inline rtk::Middleware<FTestGameState> createGameListenerMiddleware() {
  return [](const rtk::MiddlewareApi<FTestGameState> &Api)
             -> std::function<rtk::Dispatcher(rtk::Dispatcher)> {
    return [Api](rtk::Dispatcher Next) -> rtk::Dispatcher {
      return [Api, Next](const rtk::AnyAction &Action) -> rtk::AnyAction {
        /**
         * Let reducers run first
         * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
         */
        const rtk::AnyAction Result = Next(Action);

        /**
         * React to NPC movement
         * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
         */
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

        /**
         * React to NPC verdict application
         * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
         */
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
 * User Story: As game-run setup, I need a store factory with listeners attached
 * so transcript and UI side effects are active by default.
 */
inline rtk::EnhancedStore<FTestGameState> createTestGameStoreWithListeners() {
  std::vector<rtk::Middleware<FTestGameState>> Middlewares;
  Middlewares.push_back(createGameListenerMiddleware());
  return rtk::configureStore<FTestGameState>(&TestGameReducer,
                                             FTestGameState(), Middlewares);
}

} // namespace TestGame
