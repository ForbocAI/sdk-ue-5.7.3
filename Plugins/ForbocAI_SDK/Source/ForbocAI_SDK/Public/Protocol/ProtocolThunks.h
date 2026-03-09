#pragma once

#include "Core/ThunkDetail.h"
#include "Cortex/CortexThunks.h"
#include "DirectiveSlice.h"
#include "Memory/MemoryThunks.h"
#include "Protocol/ProtocolRequestTypes.h"

namespace rtk {
namespace detail {

inline func::AsyncResult<rtk::FEmptyPayload>
PersistMemoryInstructions(const TArray<FMemoryStoreInstruction> &Instructions,
                          int32 Index,
                          std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState);

inline func::AsyncResult<FAgentResponse>
RunProtocolTurn(const FString &NpcId, const FString &Input,
                const FString &RunId, const FNPCProcessTape &Tape,
                const FString &LastResultJson, bool bHasLastResult,
                int32 Turn,
                std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState) {
  if (Turn >= 12) {
    Dispatch(DirectiveSlice::Actions::DirectiveRunFailed(
        RunId, TEXT("Max turns exceeded")));
    return RejectAsync<FAgentResponse>(TEXT("Protocol loop exceeded max turns"));
  }

  FNPCProcessRequest Request;
  Request.Tape = Tape;
  Request.LastResultJson = LastResultJson;
  Request.bHasLastResult = bHasLastResult;

  return func::AsyncChain::then<FNPCProcessResponse, FAgentResponse>(
      APISlice::Endpoints::postNpcProcess(NpcId, Request)(Dispatch, GetState),
      [NpcId, Input, RunId, Tape, Turn, Dispatch,
       GetState](const FNPCProcessResponse &Response) {
        const FNPCInstruction &Instruction = Response.Instruction;

        if (Instruction.Type == ENPCInstructionType::IdentifyActor) {
          FNPCActorInfo Actor;
          Actor.NpcId = NpcId;
          Actor.Persona = Response.Tape.Persona;
          Actor.Data = Response.Tape.NpcState;
          return RunProtocolTurn(NpcId, Input, RunId, Response.Tape,
                                 SerializeIdentifyActorResult(Actor), true,
                                 Turn + 1, Dispatch, GetState);
        }

        if (Instruction.Type == ENPCInstructionType::QueryVector) {
          FDirectiveResponse Directive;
          Directive.MemoryRecall =
              TypeFactory::MemoryRecallInstruction(Instruction.Query,
                                                   Instruction.Limit,
                                                   Instruction.Threshold);
          Dispatch(DirectiveSlice::Actions::DirectiveReceived(RunId, Directive));

          FMemoryRecallRequest RecallRequest;
          RecallRequest.Query = Instruction.Query;
          RecallRequest.Limit = Instruction.Limit;
          RecallRequest.Threshold = Instruction.Threshold;

          return func::AsyncChain::then<TArray<FMemoryItem>, FAgentResponse>(
              nodeMemoryRecallThunk(RecallRequest)(Dispatch, GetState),
              [NpcId, Input, RunId, Response, Turn, Dispatch,
               GetState](const TArray<FMemoryItem> &Memories) {
                FNPCProcessTape NextTape = Response.Tape;
                NextTape.Memories.Empty();
                for (int32 Index = 0; Index < Memories.Num(); ++Index) {
                  FRecalledMemory Memory;
                  Memory.Text = Memories[Index].Text;
                  Memory.Type = Memories[Index].Type;
                  Memory.Importance = Memories[Index].Importance;
                  Memory.Similarity = Memories[Index].Similarity;
                  NextTape.Memories.Add(Memory);
                }
                NextTape.bVectorQueried = true;
                return RunProtocolTurn(NpcId, Input, RunId, NextTape,
                                       SerializeQueryVectorResult(Memories),
                                       true, Turn + 1, Dispatch, GetState);
              });
        }

        if (Instruction.Type == ENPCInstructionType::ExecuteInference) {
          Dispatch(DirectiveSlice::Actions::ContextComposed(
              RunId, Instruction.Prompt, Instruction.Constraints));

          return func::AsyncChain::then<FCortexResponse, FAgentResponse>(
              completeNodeCortexThunk(Instruction.Prompt,
                                      Instruction.Constraints)(Dispatch,
                                                               GetState),
              [NpcId, Input, RunId, Response, Turn, Dispatch,
               GetState](const FCortexResponse &Generated) {
                FNPCProcessTape NextTape = Response.Tape;
                NextTape.Prompt = Response.Instruction.Prompt;
                NextTape.Constraints = Response.Instruction.Constraints;
                NextTape.GeneratedOutput = Generated.Text;
                return RunProtocolTurn(NpcId, Input, RunId, NextTape,
                                       SerializeInferenceResult(Generated.Text),
                                       true, Turn + 1, Dispatch, GetState);
              });
        }

        if (Instruction.Type != ENPCInstructionType::Finalize) {
          const FString Error = FString::Printf(
              TEXT("Unsupported protocol instruction type: %d"),
              static_cast<int32>(Instruction.Type));
          Dispatch(DirectiveSlice::Actions::DirectiveRunFailed(RunId, Error));
          return RejectAsync<FAgentResponse>(Error);
        }

        FVerdictResponse Verdict;
        Verdict.bValid = Instruction.bValid;
        Verdict.Signature = Instruction.Signature;
        Verdict.MemoryStore = Instruction.MemoryStore;
        Verdict.StateDelta = Instruction.StateTransform;
        Verdict.Dialogue = Instruction.Dialogue;
        Verdict.bHasAction = Instruction.bHasAction;
        Verdict.Action = Instruction.Action;
        Dispatch(DirectiveSlice::Actions::VerdictValidated(RunId, Verdict));

        if (!Instruction.bValid) {
          Dispatch(NPCSlice::Actions::BlockAction(
              NpcId, Instruction.Dialogue.IsEmpty()
                         ? TEXT("Validation failed")
                         : Instruction.Dialogue));
          return ResolveAsync(BuildAgentResponse(Instruction));
        }

        return func::AsyncChain::then<rtk::FEmptyPayload, FAgentResponse>(
            PersistMemoryInstructions(Instruction.MemoryStore, 0, Dispatch,
                                      GetState),
            [NpcId, Input, Instruction, Dispatch,
             GetState](const rtk::FEmptyPayload &) {
              if (HasStatePayload(Instruction.StateTransform)) {
                Dispatch(NPCSlice::Actions::UpdateNPCState(
                    NpcId, Instruction.StateTransform));
              }

              Dispatch(NPCSlice::Actions::SetLastAction(
                  NpcId, Instruction.Action, Instruction.bHasAction));

              Dispatch(NPCSlice::Actions::AddToHistory(NpcId, TEXT("user"),
                                                       Input));
              Dispatch(NPCSlice::Actions::AddToHistory(
                  NpcId, TEXT("assistant"), Instruction.Dialogue));

              return ResolveAsync(BuildAgentResponse(Instruction));
            });
      })
      .catch_([RunId, Dispatch](std::string Error) {
        Dispatch(DirectiveSlice::Actions::DirectiveRunFailed(
            RunId, FString(UTF8_TO_TCHAR(Error.c_str()))));
      });
}

inline func::AsyncResult<rtk::FEmptyPayload>
PersistMemoryInstructions(const TArray<FMemoryStoreInstruction> &Instructions,
                          int32 Index,
                          std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState) {
  if (Index >= Instructions.Num()) {
    return ResolveAsync(rtk::FEmptyPayload{});
  }

  return func::AsyncChain::then<FMemoryItem, rtk::FEmptyPayload>(
      nodeMemoryStoreThunk(MakeMemoryItem(Instructions[Index]))(Dispatch,
                                                                GetState),
      [Instructions, Index, Dispatch,
       GetState](const FMemoryItem &Stored) {
        return PersistMemoryInstructions(Instructions, Index + 1, Dispatch,
                                         GetState);
      });
}

} // namespace detail

// ---------------------------------------------------------------------------
// Protocol thunks
// ---------------------------------------------------------------------------

inline ThunkAction<FAgentResponse, FSDKState>
processNPC(const FString &NpcId, const FString &Input = TEXT(""),
           const FString &ContextJson = TEXT("{}"),
           const FString &Persona = TEXT(""),
           const FAgentState &InitialState = FAgentState()) {
  return [NpcId, Input, ContextJson, Persona, InitialState](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<FAgentResponse> {
    const auto ExistingNpc = NPCSlice::SelectNPCById(GetState().NPCs, NpcId);
    FString ResolvedPersona = Persona;
    FAgentState CurrentState = InitialState;
    const bool bHasExplicitState =
        !InitialState.JsonData.IsEmpty() && InitialState.JsonData != TEXT("{}");

    if (ExistingNpc.hasValue) {
      if (ResolvedPersona.IsEmpty()) {
        ResolvedPersona = ExistingNpc.value.Persona;
      }
      if (!bHasExplicitState) {
        CurrentState = ExistingNpc.value.State;
      }
    }

    if (ResolvedPersona.IsEmpty()) {
      return detail::RejectAsync<FAgentResponse>(
          TEXT("No persona provided and no active NPC persona available"));
    }

    if (!ExistingNpc.hasValue) {
      FNPCInternalState Info;
      Info.Id = NpcId;
      Info.Persona = ResolvedPersona;
      Info.State = InitialState;
      Dispatch(NPCSlice::Actions::SetNPCInfo(Info));
    } else if (NPCSlice::SelectActiveNpcId(GetState().NPCs) != NpcId) {
      Dispatch(NPCSlice::Actions::SetActiveNPC(NpcId));
    }

    const FString RunId = FString::Printf(TEXT("%s:%lld"), *NpcId,
                                          FDateTime::UtcNow().ToUnixTimestamp());
    Dispatch(DirectiveSlice::Actions::DirectiveRunStarted(RunId, NpcId, Input));

    FNPCProcessTape Tape = TypeFactory::ProcessTape(Input, ContextJson,
                                                    CurrentState,
                                                    ResolvedPersona);
    Tape.Memories.Empty();
    Tape.bVectorQueried = false;

    return detail::RunProtocolTurn(NpcId, Input, RunId, Tape, TEXT(""), false,
                                   0, Dispatch, GetState);
  };
}

} // namespace rtk
