#pragma once

#include "APISlice.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "DirectiveSlice.h"
#include "NPCSlice.h"
#include "SDKStore.h"

// ==========================================================
// Async Thunks — Protocol Orchestration
// ==========================================================
// Implements the "processNPC" round-trip loop.
// Equivalent to `core/src/thunks.ts`.
// ===================================

namespace rtk {

/**
 * Main Protocol Thunk: processNPC
 * Orchestrates the recursive instruction loop with the remote API.
 */
inline ThunkAction<FString, FSDKState> processNPC(FString NpcId) {
  return [NpcId](auto dispatch, auto getState) -> func::AsyncResult<FString> {
    FString RunId = FGuid::NewGuid().ToString();

    // 1. Start the directive run
    dispatch(DirectiveSlice::Actions::DirectiveRunStarted({RunId, NpcId}));

    struct FLoop {
      static func::AsyncResult<FString>
      Run(FString NpcId, FString RunId, FSDKState State, TArray<FString> Tape,
          FString LastResult, int32 Turn,
          std::function<AnyAction(const AnyAction &)> dispatch,
          std::function<FSDKState()> getState) {
        if (Turn >= 12) {
          dispatch(DirectiveSlice::Actions::DirectiveRunFailed(
              {RunId, TEXT("Max turns exceeded")}));
          return func::AsyncResult<FString>::create(
              [](auto Resolve, auto Reject) { Reject("Max turns exceeded"); });
        }

        FNPCProcessRequest Req;
        Req.NpcId = NpcId;
        Req.Tape = Tape;
        Req.LastResult = LastResult;

        return func::AsyncChain::then<FNPCInstruction, FString>(
            Endpoints::postNpcProcess(Req)(dispatch, getState),
            [NpcId, RunId, Tape, Turn, dispatch,
             getState](FNPCInstruction Instruction) {
              dispatch(DirectiveSlice::Actions::DirectiveReceived(
                  {RunId, Instruction.Payload}));

              switch (Instruction.Type) {
              case ENPCInstructionType::IdentifyActor:
                dispatch(DirectiveSlice::Actions::ContextComposed(
                    {RunId, TEXT("Identifying actor context")}));
                return Run(NpcId, RunId, getState(), Instruction.Tape,
                           TEXT("Actor Identified"), Turn + 1, dispatch,
                           getState);

              case ENPCInstructionType::QueryVector:
                dispatch(DirectiveSlice::Actions::ContextComposed(
                    {RunId, TEXT("Querying memory vector space")}));
                return Run(NpcId, RunId, getState(), Instruction.Tape,
                           TEXT("Memories Recalled"), Turn + 1, dispatch,
                           getState);

              case ENPCInstructionType::ExecuteInference:
                dispatch(DirectiveSlice::Actions::ContextComposed(
                    {RunId, TEXT("Executing LLM inference")}));
                return Run(NpcId, RunId, getState(), Instruction.Tape,
                           TEXT("Inference Complete"), Turn + 1, dispatch,
                           getState);

              case ENPCInstructionType::Finalize:
                dispatch(DirectiveSlice::Actions::VerdictValidated(
                    {RunId, Instruction.Payload}));
                dispatch(NPCSlice::Actions::AddToHistory(
                    {NpcId, Instruction.Payload}));
                return func::AsyncResult<FString>::create(
                    [Instruction](auto Resolve, auto Reject) {
                      Resolve(Instruction.Payload);
                    });

              default:
                return func::AsyncResult<FString>::create(
                    [](auto Resolve, auto Reject) {
                      Reject("Unknown instruction type");
                    });
              }
            });
      }
    };

    return FLoop::Run(NpcId, RunId, getState(), {}, TEXT(""), 0, dispatch,
                      getState);
  };
}

/**
 * Phase D — Listener Middleware
 * Automated state cleanup when an NPC is removed.
 */
inline Middleware<FSDKState> createNpcRemovalListener() {
  return [](auto dispatch, auto getState) {
    return [dispatch, getState](auto next) {
      return [dispatch, getState, next](const AnyAction &action) {
        // Equivalent to `npcRemovalListener.ts`
        if (action.type == TEXT("npc/removeNPC")) {
          FString NpcId = action.getPayload<FString>();
          // Cascade: Clear directive runs, memory caches, and cortex context
          // for this NPC
          dispatch(DirectiveSlice::Actions::ClearRunsByNpc({NpcId}));
          dispatch(MemorySlice::Actions::ClearMemoryCache({NpcId}));
        }
        return next(action);
      };
    };
  };
}

/**
 * Thunk for initializing the Cortex engine (loading GGUF model).
 */
inline auto initNodeCortexThunk =
    createAsyncThunk<FCortexStatus, FString, FSDKState>(
        TEXT("cortex/init"),
        [](const FString &ModelPath, const ThunkApi<FSDKState> &api) {
          // Equivalent to `initNodeCortex` in TS
          FCortexStatus Status;
          Status.Id = TEXT("local-llama");
          Status.Model = ModelPath;
          Status.bReady = true;
          Status.Engine = TEXT("local");

          return func::AsyncResult<FCortexStatus>::create(
              [Status](auto Resolve, auto Reject) { Resolve(Status); });
        });

/**
 * Thunk for executing a completion request on the Cortex engine.
 */
inline auto completeNodeCortexThunk =
    createAsyncThunk<FString, FString, FSDKState>(
        TEXT("cortex/complete"),
        [](const FString &Prompt, const ThunkApi<FSDKState> &api) {
          // Equivalent to `complete` in TS
          return func::AsyncResult<FString>::create(
              [](auto Resolve, auto Reject) {
                Resolve(TEXT("Inference response for: ") + Prompt);
              });
        });

} // namespace rtk
