#pragma once

#include "API/APISlice.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "DirectiveSlice.h"
#include "NPC/NPCSlice.h"
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
inline ThunkAction<FString, FSDKState> processNPC(FString NpcId,
                                                  FString Input = TEXT("")) {
  return [NpcId, Input](auto dispatch,
                        auto getState) -> func::AsyncResult<FString> {
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
                    {RunId, TEXT("Querying local memory vector space")}));
                // In a real app, we'd convert text to vector first (embedding)
                // For this parity demo, we'll pass a dummy vector
                return func::AsyncChain::then<TArray<FString>, FString>(
                    nodeMemoryThunk({0.1f, 0.2f})(dispatch, getState),
                    [NpcId, RunId, Instruction, Turn, dispatch,
                     getState](TArray<FString> Results) {
                      FString Memories = FString::Join(Results, TEXT(", "));
                      return Run(NpcId, RunId, getState(), Instruction.Tape,
                                 Memories, Turn + 1, dispatch, getState);
                    });

              case ENPCInstructionType::ExecuteInference:
                dispatch(DirectiveSlice::Actions::ContextComposed(
                    {RunId, TEXT("Executing local LLM inference")}));
                return func::AsyncChain::then<FCortexResponse, FString>(
                    completeNodeCortexThunk(Instruction.Payload)(dispatch,
                                                                 getState),
                    [NpcId, RunId, Instruction, Turn, dispatch,
                     getState](FCortexResponse Response) {
                      return Run(NpcId, RunId, getState(), Instruction.Tape,
                                 Response.Text, Turn + 1, dispatch, getState);
                    });

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

    return FLoop::Run(NpcId, RunId, getState(), {}, Input, 0, dispatch,
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

#include "NativeEngine.h"

/**
 * Thunk for initializing the Cortex engine (loading GGUF model).
 */
inline auto initNodeCortexThunk =
    createAsyncThunk<FCortexStatus, FString, FSDKState>(
        TEXT("cortex/init"),
        [](const FString &ModelPath, const ThunkApi<FSDKState> &api) {
          Native::Llama::Context Ctx = Native::Llama::LoadModel(ModelPath);

          FCortexStatus Status;
          Status.Id = TEXT("local-llama");
          Status.Model = ModelPath;
          Status.bReady = (Ctx != nullptr);
          Status.Engine = TEXT("local");

          if (Ctx) {
            // In a real app, we'd store the handle in the state or a global
            // registry For this functional demo, we'll assume the handle is
            // managed
          }

          return func::AsyncResult<FCortexStatus>::create(
              [Status](auto Resolve, auto Reject) {
                if (Status.bReady)
                  Resolve(Status);
                else
                  Reject("Failed to load model");
              });
        });

/**
 * Thunk for executing inference on the local SLM.
 */
inline auto completeNodeCortexThunk =
    createAsyncThunk<FCortexResponse, FString, FSDKState>(
        TEXT("cortex/complete"),
        [](const FString &Prompt, const ThunkApi<FSDKState> &api) {
          // Use the handle if available in state
          void *Handle =
              api.getState().Cortex.Status.bReady ? (void *)1 : nullptr;

          FString ResponseText = Native::Llama::Infer(Handle, Prompt);

          FCortexResponse Response;
          Response.Id = FGuid::NewGuid().ToString();
          Response.Text = ResponseText;
          Response.Stats = TEXT("Local-Inference-Llama");

          return func::AsyncResult<FCortexResponse>::create(
              [Response](auto Resolve, auto Reject) { Resolve(Response); });
        });

/**
 * Thunk for local vector memory search (Node).
 */
inline auto nodeMemoryRecallThunk =
    createAsyncThunk<TArray<FMemoryItem>, FString, FSDKState>(
        TEXT("memory/recallLocal"),
        [](const FString &Query, const ThunkApi<FSDKState> &api) {
          return func::AsyncChain::then<TArray<float>, TArray<FMemoryItem>>(
              generateNodeEmbeddingThunk(Query)(api.dispatch, api.getState),
              [api](TArray<float> Vector) {
                // Use the database handle from state (simulated as offset 2)
                void *Handle = (void *)2;

                TArray<FString> Results =
                    Native::Sqlite::Search(Handle, Vector);

                TArray<FMemoryItem> Items;
                for (const FString &Text : Results) {
                  FMemoryItem Item;
                  Item.Id = FString::Printf(TEXT("mem_%s"),
                                            *FGuid::NewGuid().ToString());
                  Item.Text = Text;
                  Item.Timestamp = FDateTime::Now().ToUnixTimestamp();
                  Items.Add(Item);
                }

                return func::AsyncResult<TArray<FMemoryItem>>::create(
                    [Items](auto Resolve, auto Reject) { Resolve(Items); });
              });
        });

/**
 * Thunk for storing a memory locally (Node).
 */
inline auto nodeMemoryStoreThunk =
    createAsyncThunk<FMemoryItem, FMemoryItem, FSDKState>(
        TEXT("memory/storeLocal"),
        [](const FMemoryItem &Item, const ThunkApi<FSDKState> &api) {
          FString TextToEmbed = Item.Text;
          return func::AsyncChain::then<TArray<float>, FMemoryItem>(
              generateNodeEmbeddingThunk(TextToEmbed)(api.dispatch,
                                                      api.getState),
              [Item, api](TArray<float> Vector) {
                void *Handle = (void *)2;
                Native::Sqlite::Store(Handle, Item.Id, Vector, Item.Type);

                return func::AsyncResult<FMemoryItem>::create(
                    [Item](auto Resolve, auto Reject) { Resolve(Item); });
              });
        });

#include "Soul/SoulModule.h"

/**
 * Thunk for exporting a Soul to Arweave.
 */
inline auto exportSoulThunk = createAsyncThunk<FString, FString, FSDKState>(
    TEXT("soul/export"),
    [](const FString &AgentId, const ThunkApi<FSDKState> &api) {
      const FSDKState &State = api.getState();

      // Find the NPC
      auto NPC =
          NPCSlice::GetNPCAdapter().selectById(State.NPCs.Entities, AgentId);
      if (!NPC.hasValue) {
        return func::AsyncResult<FString>::create(
            [](auto Res, auto Rej) { Rej("NPC not found"); });
      }

      // Extract memories (simplification: take all current memories)
      TArray<FMemoryItem> Memories;
      State.Memory.Entities.Ids.ForEach([&Memories, &State](const FString &Id) {
        auto Mem = MemorySlice::GetMemoryAdapter().selectById(
            State.Memory.Entities, Id);
        if (Mem.hasValue)
          Memories.Add(Mem.value);
      });

      // Create Soul
      auto SoulRes = SoulOps::FromAgent(NPC.value.State, Memories, AgentId,
                                        NPC.value.Persona);
      if (SoulRes.isLeft) {
        return func::AsyncResult<FString>::create(
            [SoulRes](auto Res, auto Rej) {
              Rej(TCHAR_TO_UTF8(*SoulRes.left));
            });
      }

      FSoulExportRequest Req;
      Req.Soul = SoulRes.right;

      // Call API
      return func::AsyncChain::then<FSoulExportResponse, FString>(
          Endpoints::postSoulExport(Req)(api.dispatch, api.getState),
          [](FSoulExportResponse Res) {
            return func::AsyncResult<FString>::create(
                [Res](auto Resolve, auto Reject) { Resolve(Res.TxId); });
          });
    });

/**
 * Thunk for importing a Soul.
 */
inline auto importSoulThunk = createAsyncThunk<FSoul, FString, FSDKState>(
    TEXT("soul/import"),
    [](const FString &Json, const ThunkApi<FSDKState> &api) {
      auto SoulRes = SoulOps::Deserialize(Json);
      return func::AsyncResult<FSoul>::create(
          [SoulRes](auto Resolve, auto Reject) {
            if (SoulRes.isRight)
              Resolve(SoulRes.right);
            else
              Reject(TCHAR_TO_UTF8(*SoulRes.left));
          });
    });

/**
 * Thunk for generating an embedding from text.
 */
inline auto generateNodeEmbeddingThunk =
    createAsyncThunk<TArray<float>, FString, FSDKState>(
        TEXT("vector/generate"),
        [](const FString &Text, const ThunkApi<FSDKState> &api) {
          // Use the handle if available (mocked for now)
          void *Handle = (void *)1;
          TArray<float> Vector = Native::Llama::Embed(Handle, Text);

          return func::AsyncResult<TArray<float>>::create(
              [Vector](auto Resolve, auto Reject) { Resolve(Vector); });
        });

/**
 * Thunk for system health check.
 */
inline auto doctorThunk =
    createAsyncThunk<FApiStatusResponse, FEmptyPayload, FSDKState>(
        TEXT("system/doctor"),
        [](const FEmptyPayload &Arg, const ThunkApi<FSDKState> &api) {
          return Endpoints::getApiStatus()(api.dispatch, api.getState);
        });

} // namespace rtk
