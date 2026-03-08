#pragma once

#include "API/APISlice.h"
#include "Async/Async.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "DirectiveSlice.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Memory/MemorySlice.h"
#include "Misc/Paths.h"
#include "NPC/NPCSlice.h"
#include "NativeEngine.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"
#include "Soul/SoulSlice.h"

namespace rtk {

inline ThunkAction<FCortexResponse, FSDKState>
completeNodeCortexThunk(const FString &Prompt,
                        const FCortexConfig &Config = FCortexConfig());

inline ThunkAction<FMemoryItem, FSDKState>
nodeMemoryStoreThunk(const FMemoryItem &Item);

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
nodeMemoryRecallThunk(const FMemoryRecallRequest &Request);

namespace detail {

inline Native::Llama::Context &NodeCortexHandle() {
  static Native::Llama::Context Handle = nullptr;
  return Handle;
}

inline Native::Sqlite::DB &NodeMemoryHandle() {
  static Native::Sqlite::DB Handle = nullptr;
  return Handle;
}

template <typename T>
inline func::AsyncResult<T> ResolveAsync(const T &Value) {
  return func::AsyncResult<T>::create(
      [Value](std::function<void(T)> Resolve,
              std::function<void(std::string)> Reject) { Resolve(Value); });
}

template <typename T>
inline func::AsyncResult<T> RejectAsync(const FString &Error) {
  const std::string Utf8Error = TCHAR_TO_UTF8(*Error);
  return func::AsyncResult<T>::create(
      [Utf8Error](std::function<void(T)> Resolve,
                  std::function<void(std::string)> Reject) {
        Reject(Utf8Error);
      });
}

inline FString SafeJson(const FString &Json) {
  return Json.IsEmpty() ? TEXT("{}") : Json;
}

inline FString SafeStateJson(const FAgentState &State) {
  return SafeJson(State.JsonData);
}

inline bool HasStatePayload(const FAgentState &State) {
  return !State.JsonData.IsEmpty() && State.JsonData != TEXT("{}");
}

inline FString JsonObjectToString(const TSharedPtr<FJsonObject> &Object) {
  FString Output;
  TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
  FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
  return Output;
}

inline FString SerializeIdentifyActorResult(const FNPCActorInfo &Actor) {
  const TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
  ActorObject->SetStringField(TEXT("npcId"), Actor.NpcId);
  ActorObject->SetStringField(TEXT("persona"), Actor.Persona);
  ActorObject->SetStringField(TEXT("data"), SafeStateJson(Actor.Data));

  const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("type"), TEXT("IdentifyActorResult"));
  Root->SetObjectField(TEXT("actor"), ActorObject);
  return JsonObjectToString(Root);
}

inline FString SerializeInferenceResult(const FString &GeneratedOutput) {
  const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("type"), TEXT("ExecuteInferenceResult"));
  Root->SetStringField(TEXT("generatedOutput"), GeneratedOutput);
  return JsonObjectToString(Root);
}

inline FString SerializeQueryVectorResult(const TArray<FMemoryItem> &Memories) {
  TArray<TSharedPtr<FJsonValue>> MemoryValues;
  for (int32 Index = 0; Index < Memories.Num(); ++Index) {
    const FMemoryItem &Memory = Memories[Index];
    const TSharedPtr<FJsonObject> MemoryObject = MakeShared<FJsonObject>();
    MemoryObject->SetStringField(TEXT("text"), Memory.Text);
    MemoryObject->SetStringField(TEXT("type"), Memory.Type);
    MemoryObject->SetNumberField(TEXT("importance"), Memory.Importance);
    MemoryObject->SetNumberField(TEXT("similarity"), Memory.Similarity);
    MemoryValues.Add(MakeShared<FJsonValueObject>(MemoryObject));
  }

  const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("type"), TEXT("QueryVectorResult"));
  Root->SetArrayField(TEXT("memories"), MemoryValues);
  return JsonObjectToString(Root);
}

inline FString DefaultNodeMemoryPath() {
  return FPaths::ProjectSavedDir() + TEXT("ForbocAI_Memory.sqlite");
}

inline Native::Sqlite::DB EnsureNodeMemoryDatabase() {
  Native::Sqlite::DB &Handle = NodeMemoryHandle();
  if (!Handle) {
    Handle = Native::Sqlite::Open(DefaultNodeMemoryPath());
  }
  return Handle;
}

inline FMemoryItem MakeMemoryItem(const FMemoryStoreInstruction &Instruction) {
  FMemoryItem Item;
  Item.Id = FGuid::NewGuid().ToString();
  Item.Text = Instruction.Text;
  Item.Type = Instruction.Type.IsEmpty() ? TEXT("observation")
                                         : Instruction.Type;
  Item.Importance = Instruction.Importance;
  Item.Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
  return Item;
}

inline FAgentResponse BuildAgentResponse(const FNPCInstruction &Instruction) {
  FAgentResponse Response;
  Response.Dialogue = Instruction.Dialogue;
  Response.Thought = Instruction.Dialogue;
  if (Instruction.bHasAction) {
    Response.Action = Instruction.Action;
  }
  return Response;
}

inline func::AsyncResult<rtk::FEmptyPayload>
PersistMemoryInstructions(const TArray<FMemoryStoreInstruction> &Instructions,
                          int32 Index,
                          std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState);

inline func::AsyncResult<FArweaveUploadResult>
UploadSignedSoul(const FArweaveUploadInstruction &Instruction,
                 const FString &SignedPayload) {
  return func::AsyncResult<FArweaveUploadResult>::create(
      [Instruction, SignedPayload](
          std::function<void(FArweaveUploadResult)> Resolve,
          std::function<void(std::string)> Reject) {
        const FString Url = !Instruction.UploadUrl.IsEmpty()
                                ? Instruction.UploadUrl
                                : Instruction.GatewayUrl;
        if (Url.IsEmpty()) {
          Reject("Missing Arweave upload URL");
          return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
            FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb(TEXT("POST"));
        Request->SetHeader(TEXT("Content-Type"),
                           Instruction.ContentType.IsEmpty()
                               ? TEXT("application/octet-stream")
                               : Instruction.ContentType);
        if (!Instruction.AuiAuthHeader.IsEmpty()) {
          Request->SetHeader(TEXT("Authorization"), Instruction.AuiAuthHeader);
        }
        if (!Instruction.TagsJson.IsEmpty()) {
          Request->SetHeader(TEXT("X-Forboc-Tags"), Instruction.TagsJson);
        }
        Request->SetContentAsString(SignedPayload);
        Request->OnProcessRequestComplete().BindLambda(
            [Instruction, Resolve, Reject](FHttpRequestPtr Req,
                                           FHttpResponsePtr Res,
                                           bool bWasSuccessful) {
              if (!bWasSuccessful || !Res.IsValid()) {
                Reject("Arweave upload failed");
                return;
              }

              FArweaveUploadResult Result;
              Result.ResponseJson = Res->GetContentAsString();
              Result.Status = TEXT("uploaded");
              Result.TxId = Res->GetHeader(TEXT("x-id"));
              if (Result.TxId.IsEmpty()) {
                Result.TxId = LexToString(GetTypeHash(Result.ResponseJson));
              }
              Result.ArweaveUrl = !Instruction.GatewayUrl.IsEmpty()
                                      ? Instruction.GatewayUrl + TEXT("/") +
                                            Result.TxId
                                      : TEXT("");
              Resolve(Result);
            });
        Request->ProcessRequest();
      });
}

inline func::AsyncResult<FArweaveDownloadResult>
DownloadSoulPayload(const FArweaveDownloadInstruction &Instruction) {
  return func::AsyncResult<FArweaveDownloadResult>::create(
      [Instruction](std::function<void(FArweaveDownloadResult)> Resolve,
                    std::function<void(std::string)> Reject) {
        FString Url = Instruction.DownloadUrl;
        if (Url.IsEmpty() && !Instruction.GatewayUrl.IsEmpty() &&
            !Instruction.TxId.IsEmpty()) {
          Url = Instruction.GatewayUrl + TEXT("/") + Instruction.TxId;
        }

        if (Url.IsEmpty()) {
          Reject("Missing Arweave download URL");
          return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
            FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb(TEXT("GET"));
        Request->OnProcessRequestComplete().BindLambda(
            [Instruction, Resolve, Reject](FHttpRequestPtr Req,
                                           FHttpResponsePtr Res,
                                           bool bWasSuccessful) {
              if (!bWasSuccessful || !Res.IsValid()) {
                Reject("Arweave download failed");
                return;
              }

              FArweaveDownloadResult Result;
              Result.TxId = Instruction.TxId;
              Result.Payload = Res->GetContentAsString();
              Result.Status = TEXT("downloaded");
              Result.ResponseJson = Result.Payload;
              Resolve(Result);
            });
        Request->ProcessRequest();
      });
}

inline func::AsyncResult<FAgentResponse>
RunProtocolTurn(const FString &NpcId, const FString &Input,
                const FString &RunId, const FNPCProcessTape &Tape,
                const FString &LastResultJson, bool bHasLastResult,
                int32 Turn,
                std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState) {
  if (Turn >= 12) {
    Dispatch(
        DirectiveSlice::Actions::DirectiveRunFailed(RunId, TEXT("Max turns exceeded")));
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
                                      Instruction.Constraints)(Dispatch, GetState),
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
      [Instructions, Index, Dispatch, GetState](const FMemoryItem &Stored) {
        return PersistMemoryInstructions(Instructions, Index + 1, Dispatch,
                                         GetState);
      });
}

} // namespace detail

inline ThunkAction<FAgentResponse, FSDKState>
processNPC(const FString &NpcId, const FString &Input = TEXT(""),
           const FString &ContextJson = TEXT("{}")) {
  return [NpcId, Input, ContextJson](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<FAgentResponse> {
    const auto Npc =
        NPCSlice::SelectNPCById(GetState().NPCs, NpcId);
    if (!Npc.hasValue) {
      return detail::RejectAsync<FAgentResponse>(TEXT("NPC not found"));
    }

    const FString RunId = FString::Printf(TEXT("%s:%lld"), *NpcId,
                                          FDateTime::UtcNow().ToUnixTimestamp());
    Dispatch(DirectiveSlice::Actions::DirectiveRunStarted(RunId, NpcId, Input));

    FNPCProcessTape Tape =
        TypeFactory::ProcessTape(Input, ContextJson, Npc.value.State,
                                 Npc.value.Persona);
    Tape.Memories.Empty();
    Tape.bVectorQueried = false;

    return detail::RunProtocolTurn(NpcId, Input, RunId, Tape, TEXT(""), false,
                                   0, Dispatch, GetState);
  };
}

inline ThunkAction<FCortexStatus, FSDKState>
initNodeCortexThunk(const FString &ModelPath) {
  return [ModelPath](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexStatus> {
    Dispatch(CortexSlice::Actions::CortexInitPending(ModelPath));

    return func::AsyncResult<FCortexStatus>::create(
        [ModelPath, Dispatch](std::function<void(FCortexStatus)> Resolve,
                              std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [ModelPath, Dispatch, Resolve, Reject]() {
            Native::Llama::Context &Handle = detail::NodeCortexHandle();
            if (Handle) {
              Native::Llama::FreeModel(Handle);
              Handle = nullptr;
            }

            Handle = Native::Llama::LoadModel(ModelPath);

            FCortexStatus Status;
            Status.Id = TEXT("local-llama");
            Status.Model = ModelPath;
            Status.bReady = (Handle != nullptr);
            Status.Engine = ECortexEngine::NodeLlamaCpp;
            Status.DownloadProgress = Status.bReady ? 1.0f : 0.0f;
            Status.Error = Status.bReady ? TEXT("") : TEXT("Failed to load model");

            AsyncTask(ENamedThreads::GameThread, [Dispatch, Resolve, Reject, Status]() {
              if (Status.bReady) {
                Dispatch(CortexSlice::Actions::CortexInitFulfilled(Status));
                Resolve(Status);
              } else {
                Dispatch(CortexSlice::Actions::CortexInitRejected(Status.Error));
                Reject(TCHAR_TO_UTF8(*Status.Error));
              }
            });
          });
        });
  };
}

inline ThunkAction<FCortexResponse, FSDKState>
completeNodeCortexThunk(const FString &Prompt, const FCortexConfig &Config) {
  return [Prompt, Config](std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexResponse> {
    Dispatch(CortexSlice::Actions::CortexCompletePending(Prompt));

    return func::AsyncResult<FCortexResponse>::create(
        [Prompt, Config, Dispatch](std::function<void(FCortexResponse)> Resolve,
                                   std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Prompt, Config, Dispatch, Resolve, Reject]() {
            Native::Llama::Context Handle = detail::NodeCortexHandle();
            if (!Handle) {
              detail::NodeCortexHandle() =
                  Native::Llama::LoadModel(Config.Model.IsEmpty()
                                               ? TEXT("local-default")
                                               : Config.Model);
              Handle = detail::NodeCortexHandle();
            }

            if (!Handle) {
              const FString Error = TEXT("Local cortex is not initialized");
              AsyncTask(ENamedThreads::GameThread, [Dispatch, Reject, Error]() {
                Dispatch(CortexSlice::Actions::CortexCompleteRejected(Error));
                Reject(TCHAR_TO_UTF8(*Error));
              });
              return;
            }

            FCortexResponse Response;
            Response.Id = FGuid::NewGuid().ToString();
            Response.Text = Native::Llama::Infer(Handle, Prompt, Config.MaxTokens);
            Response.Stats = TEXT("local-node");

            AsyncTask(ENamedThreads::GameThread, [Dispatch, Resolve, Response]() {
              Dispatch(CortexSlice::Actions::CortexCompleteFulfilled(Response));
              Resolve(Response);
            });
          });
        });
  };
}

inline ThunkAction<TArray<float>, FSDKState>
generateNodeEmbeddingThunk(const FString &Text) {
  return [Text](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<float>> {
    return func::AsyncResult<TArray<float>>::create(
        [Text](std::function<void(TArray<float>)> Resolve,
               std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Text, Resolve]() {
            TArray<float> Embedding =
                Native::Llama::Embed(detail::NodeCortexHandle(), Text);
            AsyncTask(ENamedThreads::GameThread,
                      [Resolve, Embedding]() { Resolve(Embedding); });
          });
        });
  };
}

inline ThunkAction<FMemoryItem, FSDKState>
nodeMemoryStoreThunk(const FMemoryItem &Item) {
  return [Item](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<FMemoryItem> {
    Dispatch(MemorySlice::Actions::MemoryStoreStart());

    return func::AsyncResult<FMemoryItem>::create(
        [Item, Dispatch](std::function<void(FMemoryItem)> Resolve,
                         std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Item, Dispatch, Resolve, Reject]() {
            Native::Sqlite::DB Db = detail::EnsureNodeMemoryDatabase();
            if (!Db) {
              const FString Error = TEXT("Failed to open node memory database");
              AsyncTask(ENamedThreads::GameThread, [Dispatch, Reject, Error]() {
                Dispatch(MemorySlice::Actions::MemoryStoreFailed(Error));
                Reject(TCHAR_TO_UTF8(*Error));
              });
              return;
            }

            FMemoryItem Stored = Item;
            Stored.Embedding =
                Native::Llama::Embed(detail::NodeCortexHandle(), Stored.Text);
            const bool bStored =
                Native::Sqlite::Upsert(Db, Stored, Stored.Embedding);

            AsyncTask(ENamedThreads::GameThread,
                      [Dispatch, Resolve, Reject, Stored, bStored]() {
                        if (bStored) {
                          Dispatch(MemorySlice::Actions::MemoryStoreSuccess(
                              Stored));
                          Resolve(Stored);
                        } else {
                          const FString Error =
                              TEXT("Failed to store local memory");
                          Dispatch(MemorySlice::Actions::MemoryStoreFailed(
                              Error));
                          Reject(TCHAR_TO_UTF8(*Error));
                        }
                      });
          });
        });
  };
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
nodeMemoryRecallThunk(const FMemoryRecallRequest &Request) {
  return [Request](std::function<AnyAction(const AnyAction &)> Dispatch,
                   std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FMemoryItem>> {
    Dispatch(MemorySlice::Actions::MemoryRecallStart());

    return func::AsyncResult<TArray<FMemoryItem>>::create(
        [Request, Dispatch](std::function<void(TArray<FMemoryItem>)> Resolve,
                            std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Request, Dispatch, Resolve, Reject]() {
            Native::Sqlite::DB Db = detail::EnsureNodeMemoryDatabase();
            if (!Db) {
              const FString Error = TEXT("Failed to open node memory database");
              AsyncTask(ENamedThreads::GameThread, [Dispatch, Reject, Error]() {
                Dispatch(MemorySlice::Actions::MemoryRecallFailed(Error));
                Reject(TCHAR_TO_UTF8(*Error));
              });
              return;
            }

            const TArray<float> QueryEmbedding =
                Native::Llama::Embed(detail::NodeCortexHandle(), Request.Query);
            TArray<FMemoryItem> Results =
                Native::Sqlite::Search(Db, QueryEmbedding, Request.Limit);

            if (Request.Threshold > 0.0f) {
              Results.RemoveAll([Request](const FMemoryItem &Item) {
                return Item.Similarity < Request.Threshold;
              });
            }

            AsyncTask(ENamedThreads::GameThread,
                      [Dispatch, Resolve, Results]() {
                        Dispatch(MemorySlice::Actions::MemoryRecallSuccess(
                            Results));
                        Resolve(Results);
                      });
          });
        });
  };
}

inline ThunkAction<FSoulExportResult, FSDKState>
exportSoulThunk(const FString &NpcId) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<FSoulExportResult> {
    const auto Npc = NPCSlice::SelectNPCById(GetState().NPCs, NpcId);
    if (!Npc.hasValue) {
      return detail::RejectAsync<FSoulExportResult>(TEXT("NPC not found"));
    }

    Dispatch(SoulSlice::Actions::RemoteExportSoulPending());

    const FSoulExportPhase1Request Request =
        TypeFactory::SoulExportPhase1Request(NpcId, Npc.value.Persona,
                                             Npc.value.State);

    return func::AsyncChain::then<FSoulExportPhase1Response, FSoulExportResult>(
        APISlice::Endpoints::postSoulExport(NpcId, Request)(Dispatch, GetState),
        [NpcId, Dispatch, GetState](const FSoulExportPhase1Response &Phase1) {
          return func::AsyncChain::then<FArweaveUploadResult, FSoulExportResult>(
              detail::UploadSignedSoul(Phase1.se1Instruction,
                                       Phase1.se1SignedPayload),
              [NpcId, Phase1, Dispatch,
               GetState](const FArweaveUploadResult &UploadResult) {
                const FSoulExportConfirmRequest Confirm =
                    TypeFactory::SoulExportConfirmRequest(
                        UploadResult, Phase1.se1SignedPayload,
                        Phase1.se1Signature);

                return func::AsyncChain::then<FSoulExportResponse,
                                              FSoulExportResult>(
                    APISlice::Endpoints::postSoulExportConfirm(NpcId, Confirm)(
                        Dispatch, GetState),
                    [Dispatch](const FSoulExportResponse &Response) {
                      const FSoulExportResult Result =
                          TypeFactory::SoulExportResult(Response.TxId,
                                                        Response.ArweaveUrl,
                                                        Response.Soul);
                      Dispatch(
                          SoulSlice::Actions::RemoteExportSoulSuccess(Result));
                      return detail::ResolveAsync(Result);
                    });
              });
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(SoulSlice::Actions::RemoteExportSoulFailed(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FSoulExportResult, FSDKState>
exportSoulThunk(const FSoul &Soul) {
  return [Soul](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<FSoulExportResult> {
    if (Soul.Id.IsEmpty()) {
      return detail::RejectAsync<FSoulExportResult>(TEXT("Soul ID is required"));
    }

    Dispatch(SoulSlice::Actions::RemoteExportSoulPending());

    const FSoulExportPhase1Request Request =
        TypeFactory::SoulExportPhase1Request(Soul.Id, Soul.Persona, Soul.State);

    return func::AsyncChain::then<FSoulExportPhase1Response, FSoulExportResult>(
        APISlice::Endpoints::postSoulExport(Soul.Id, Request)(Dispatch, GetState),
        [Soul, Dispatch, GetState](const FSoulExportPhase1Response &Phase1) {
          return func::AsyncChain::then<FArweaveUploadResult, FSoulExportResult>(
              detail::UploadSignedSoul(Phase1.se1Instruction,
                                       Phase1.se1SignedPayload),
              [Soul, Phase1, Dispatch,
               GetState](const FArweaveUploadResult &UploadResult) {
                const FSoulExportConfirmRequest Confirm =
                    TypeFactory::SoulExportConfirmRequest(
                        UploadResult, Phase1.se1SignedPayload,
                        Phase1.se1Signature);

                return func::AsyncChain::then<FSoulExportResponse,
                                              FSoulExportResult>(
                    APISlice::Endpoints::postSoulExportConfirm(Soul.Id, Confirm)(
                        Dispatch, GetState),
                    [Soul, Dispatch](const FSoulExportResponse &Response) {
                      const FSoulExportResult Result =
                          TypeFactory::SoulExportResult(
                              Response.TxId, Response.ArweaveUrl,
                              Response.Soul.Id.IsEmpty() ? Soul : Response.Soul);
                      Dispatch(
                          SoulSlice::Actions::RemoteExportSoulSuccess(Result));
                      return detail::ResolveAsync(Result);
                    });
              });
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(SoulSlice::Actions::RemoteExportSoulFailed(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FSoul, FSDKState>
importSoulThunk(const FString &TxId) {
  return [TxId](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<FSoul> {
    Dispatch(SoulSlice::Actions::ImportSoulPending());

    return func::AsyncChain::then<FSoulImportPhase1Response, FSoul>(
        APISlice::Endpoints::postNpcImport(
            TypeFactory::SoulImportPhase1Request(TxId))(Dispatch, GetState),
        [TxId, Dispatch, GetState](const FSoulImportPhase1Response &Phase1) {
          return func::AsyncChain::then<FArweaveDownloadResult, FSoul>(
              detail::DownloadSoulPayload(Phase1.si1Instruction),
              [TxId, Dispatch,
               GetState](const FArweaveDownloadResult &DownloadResult) {
                return func::AsyncChain::then<FImportedNpc, FSoul>(
                    APISlice::Endpoints::postNpcImportConfirm(
                        TypeFactory::SoulImportConfirmRequest(
                            TxId, DownloadResult))(Dispatch, GetState),
                    [Dispatch, TxId](const FImportedNpc &ImportedNpc) {
                      FSoul Soul = TypeFactory::Soul(
                          ImportedNpc.NpcId.IsEmpty() ? TxId : ImportedNpc.NpcId,
                          TEXT("2.0.0"), ImportedNpc.NpcId,
                          ImportedNpc.Persona,
                          TypeFactory::AgentState(ImportedNpc.DataJson),
                          TArray<FMemoryItem>());
                      Dispatch(SoulSlice::Actions::ImportSoulSuccess(Soul));
                      return detail::ResolveAsync(Soul);
                    });
              });
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(SoulSlice::Actions::ImportSoulFailed(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FApiStatusResponse, FSDKState> doctorThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<FApiStatusResponse> {
    return APISlice::Endpoints::getApiStatus()(Dispatch, GetState);
  };
}

inline ThunkAction<FApiStatusResponse, FSDKState> checkApiStatusThunk() {
  return doctorThunk();
}

inline ThunkAction<TArray<FCortexModelInfo>, FSDKState>
listCortexModelsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FCortexModelInfo>> {
    return APISlice::Endpoints::getCortexModels()(Dispatch, GetState);
  };
}

inline ThunkAction<FCortexStatus, FSDKState>
initRemoteCortexThunk(const FString &Model = TEXT("api-integrated"),
                      const FString &AuthKey = TEXT("")) {
  return [Model, AuthKey](std::function<AnyAction(const AnyAction &)> Dispatch,
                          std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexStatus> {
    Dispatch(CortexSlice::Actions::CortexInitPending(Model));

    return func::AsyncChain::then<FCortexInitResponse, FCortexStatus>(
        APISlice::Endpoints::postCortexInit(
            TypeFactory::CortexInitRequest(Model, AuthKey))(Dispatch, GetState),
        [Model, Dispatch](const FCortexInitResponse &Response) {
          FCortexStatus Status;
          Status.Id = Response.CortexId;
          Status.Model = Model;
          Status.bReady = Response.State.Equals(TEXT("ready"),
                                                ESearchCase::IgnoreCase);
          Status.Engine = ECortexEngine::Remote;
          Status.DownloadProgress = Status.bReady ? 1.0f : 0.0f;
          Status.Error = Status.bReady ? TEXT("") : Response.State;
          if (Status.bReady) {
            Dispatch(CortexSlice::Actions::CortexInitFulfilled(Status));
            return detail::ResolveAsync(Status);
          }
          Dispatch(CortexSlice::Actions::CortexInitRejected(Status.Error));
          return detail::RejectAsync<FCortexStatus>(
              Status.Error.IsEmpty() ? TEXT("Remote cortex init failed")
                                     : Status.Error);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(CortexSlice::Actions::CortexInitRejected(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FCortexResponse, FSDKState>
completeRemoteThunk(const FString &CortexId, const FString &Prompt) {
  return [CortexId, Prompt](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<FCortexResponse> {
    Dispatch(CortexSlice::Actions::CortexCompletePending(Prompt));
    return func::AsyncChain::then<FCortexResponse, FCortexResponse>(
        APISlice::Endpoints::postCortexComplete(
            TypeFactory::CortexCompleteRequest(CortexId, Prompt))(Dispatch,
                                                                  GetState),
        [Dispatch](const FCortexResponse &Response) {
          Dispatch(CortexSlice::Actions::CortexCompleteFulfilled(Response));
          return detail::ResolveAsync(Response);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(CortexSlice::Actions::CortexCompleteRejected(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
initNodeMemoryThunk(const FString &DatabasePath = TEXT("")) {
  return [DatabasePath](std::function<AnyAction(const AnyAction &)> Dispatch,
                        std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncResult<rtk::FEmptyPayload>::create(
        [DatabasePath](std::function<void(rtk::FEmptyPayload)> Resolve,
                       std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [DatabasePath, Resolve, Reject]() {
            Native::Sqlite::DB &Handle = detail::NodeMemoryHandle();
            if (Handle) {
              Native::Sqlite::Close(Handle);
              Handle = nullptr;
            }

            const FString Path = DatabasePath.IsEmpty()
                                     ? detail::DefaultNodeMemoryPath()
                                     : DatabasePath;
            Handle = Native::Sqlite::Open(Path);

            AsyncTask(ENamedThreads::GameThread, [Handle, Resolve, Reject]() {
              if (Handle) {
                Resolve(rtk::FEmptyPayload{});
              } else {
                Reject("Failed to initialize node memory database");
              }
            });
          });
        });
  };
}

inline ThunkAction<FMemoryItem, FSDKState>
storeNodeMemoryThunk(const FString &Text,
                     const FString &Type = TEXT("observation"),
                     float Importance = 0.5f) {
  FMemoryStoreInstruction Instruction;
  Instruction.Text = Text;
  Instruction.Type = Type;
  Instruction.Importance = Importance;
  return nodeMemoryStoreThunk(detail::MakeMemoryItem(Instruction));
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
recallNodeMemoryThunk(const FString &Query, int32 Limit = 10,
                      float Threshold = 0.7f) {
  FMemoryRecallRequest Request;
  Request.Query = Query;
  Request.Limit = Limit;
  Request.Threshold = Threshold;
  return nodeMemoryRecallThunk(Request);
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState> clearNodeMemoryThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncResult<rtk::FEmptyPayload>::create(
        [Dispatch](std::function<void(rtk::FEmptyPayload)> Resolve,
                   std::function<void(std::string)> Reject) {
          Async(EAsyncExecution::Thread, [Dispatch, Resolve]() {
            Native::Sqlite::DB &Handle = detail::NodeMemoryHandle();
            if (Handle) {
              Native::Sqlite::Close(Handle);
              Handle = nullptr;
            }

            IFileManager::Get().Delete(*detail::DefaultNodeMemoryPath(), false,
                                       true, true);

            AsyncTask(ENamedThreads::GameThread, [Dispatch, Resolve]() {
              Dispatch(MemorySlice::Actions::MemoryClear());
              Resolve(rtk::FEmptyPayload{});
            });
          });
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState> initNodeVectorThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return detail::ResolveAsync(rtk::FEmptyPayload{});
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
storeMemoryRemoteThunk(const FString &NpcId, const FString &Observation,
                       float Importance = 0.8f) {
  return [NpcId, Observation, Importance](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return APISlice::Endpoints::postMemoryStore(
        NpcId, TypeFactory::RemoteMemoryStoreRequest(Observation, Importance))(
        Dispatch, GetState);
  };
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
listMemoryRemoteThunk(const FString &NpcId) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FMemoryItem>> {
    return func::AsyncChain::then<TArray<FMemoryItem>, TArray<FMemoryItem>>(
        APISlice::Endpoints::getMemoryList(NpcId)(Dispatch, GetState),
        [Dispatch](const TArray<FMemoryItem> &Items) {
          Dispatch(MemorySlice::Actions::MemoryRecallSuccess(Items));
          return detail::ResolveAsync(Items);
        });
  };
}

inline ThunkAction<TArray<FMemoryItem>, FSDKState>
recallMemoryRemoteThunk(const FString &NpcId, const FString &Query,
                        float Similarity = 0.0f) {
  return [NpcId, Query, Similarity](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FMemoryItem>> {
    Dispatch(MemorySlice::Actions::MemoryRecallStart());
    return func::AsyncChain::then<TArray<FMemoryItem>, TArray<FMemoryItem>>(
        APISlice::Endpoints::postMemoryRecall(
            NpcId, TypeFactory::RemoteMemoryRecallRequest(Query, Similarity))(
            Dispatch, GetState),
        [Dispatch](const TArray<FMemoryItem> &Items) {
          Dispatch(MemorySlice::Actions::MemoryRecallSuccess(Items));
          return detail::ResolveAsync(Items);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(MemorySlice::Actions::MemoryRecallFailed(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
clearMemoryRemoteThunk(const FString &NpcId) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncChain::then<rtk::FEmptyPayload, rtk::FEmptyPayload>(
        APISlice::Endpoints::deleteMemoryClear(NpcId)(Dispatch, GetState),
        [Dispatch](const rtk::FEmptyPayload &Payload) {
          Dispatch(MemorySlice::Actions::MemoryClear());
          return detail::ResolveAsync(Payload);
        });
  };
}

inline ThunkAction<FValidationResult, FSDKState>
validateBridgeThunk(const FAgentAction &Action,
                    const FBridgeValidationContext &Context,
                    const FString &NpcId = TEXT("")) {
  return [Action, Context, NpcId](
             std::function<AnyAction(const AnyAction &)> Dispatch,
             std::function<FSDKState()> GetState)
             -> func::AsyncResult<FValidationResult> {
    Dispatch(BridgeSlice::Actions::BridgeValidationPending());
    return func::AsyncChain::then<FValidationResult, FValidationResult>(
        APISlice::Endpoints::postBridgeValidate(
            NpcId, TypeFactory::BridgeValidateRequest(Action, Context))(
            Dispatch, GetState),
        [Dispatch](const FValidationResult &Result) {
          Dispatch(BridgeSlice::Actions::BridgeValidationSuccess(Result));
          return detail::ResolveAsync(Result);
        })
        .catch_([Dispatch](std::string Error) {
          Dispatch(BridgeSlice::Actions::BridgeValidationFailure(
              FString(UTF8_TO_TCHAR(Error.c_str()))));
        });
  };
}

inline ThunkAction<FDirectiveRuleSet, FSDKState>
loadBridgePresetThunk(const FString &PresetName) {
  return [PresetName](std::function<AnyAction(const AnyAction &)> Dispatch,
                      std::function<FSDKState()> GetState)
             -> func::AsyncResult<FDirectiveRuleSet> {
    return func::AsyncChain::then<FDirectiveRuleSet, FDirectiveRuleSet>(
        APISlice::Endpoints::postBridgePreset(PresetName)(Dispatch, GetState),
        [Dispatch, PresetName](const FDirectiveRuleSet &Ruleset) {
          Dispatch(BridgeSlice::Actions::AddActivePresetId(
              Ruleset.Id.IsEmpty() ? PresetName : Ruleset.Id));
          return detail::ResolveAsync(Ruleset);
        });
  };
}

inline ThunkAction<TArray<FBridgeRule>, FSDKState> getBridgeRulesThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FBridgeRule>> {
    return APISlice::Endpoints::getBridgeRules()(Dispatch, GetState);
  };
}

inline ThunkAction<TArray<FDirectiveRuleSet>, FSDKState> listRulesetsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FDirectiveRuleSet>> {
    return func::AsyncChain::then<TArray<FDirectiveRuleSet>,
                                  TArray<FDirectiveRuleSet>>(
        APISlice::Endpoints::getRulesets()(Dispatch, GetState),
        [Dispatch](const TArray<FDirectiveRuleSet> &Rulesets) {
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Rulesets);
        });
  };
}

inline ThunkAction<TArray<FString>, FSDKState> listRulePresetsThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FString>> {
    return func::AsyncChain::then<TArray<FString>, TArray<FString>>(
        APISlice::Endpoints::getRulePresets()(Dispatch, GetState),
        [Dispatch](const TArray<FString> &PresetIds) {
          Dispatch(BridgeSlice::Actions::SetAvailablePresetIds(PresetIds));
          return detail::ResolveAsync(PresetIds);
        });
  };
}

inline ThunkAction<FDirectiveRuleSet, FSDKState>
registerRulesetThunk(const FDirectiveRuleSet &Ruleset) {
  return [Ruleset](std::function<AnyAction(const AnyAction &)> Dispatch,
                   std::function<FSDKState()> GetState)
             -> func::AsyncResult<FDirectiveRuleSet> {
    return func::AsyncChain::then<FDirectiveRuleSet, FDirectiveRuleSet>(
        APISlice::Endpoints::postRuleRegister(Ruleset)(Dispatch, GetState),
        [Dispatch, GetState](const FDirectiveRuleSet &Registered) {
          TArray<FDirectiveRuleSet> Rulesets = GetState().Bridge.AvailableRulesets;
          bool bReplaced = false;
          for (int32 Index = 0; Index < Rulesets.Num(); ++Index) {
            if (Rulesets[Index].Id == Registered.Id) {
              Rulesets[Index] = Registered;
              bReplaced = true;
              break;
            }
          }
          if (!bReplaced) {
            Rulesets.Add(Registered);
          }
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Registered);
        });
  };
}

inline ThunkAction<rtk::FEmptyPayload, FSDKState>
deleteRulesetThunk(const FString &RulesetId) {
  return [RulesetId](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<rtk::FEmptyPayload> {
    return func::AsyncChain::then<rtk::FEmptyPayload, rtk::FEmptyPayload>(
        APISlice::Endpoints::deleteRule(RulesetId)(Dispatch, GetState),
        [Dispatch, GetState, RulesetId](const rtk::FEmptyPayload &Payload) {
          TArray<FDirectiveRuleSet> Rulesets = GetState().Bridge.AvailableRulesets;
          Rulesets.RemoveAll([RulesetId](const FDirectiveRuleSet &Ruleset) {
            return Ruleset.Id == RulesetId || Ruleset.RulesetId == RulesetId;
          });
          Dispatch(BridgeSlice::Actions::SetAvailableRulesets(Rulesets));
          return detail::ResolveAsync(Payload);
        });
  };
}

inline ThunkAction<FGhostRunResponse, FSDKState>
startGhostThunk(const FGhostConfig &Config) {
  return [Config](std::function<AnyAction(const AnyAction &)> Dispatch,
                  std::function<FSDKState()> GetState)
             -> func::AsyncResult<FGhostRunResponse> {
    return func::AsyncChain::then<FGhostRunResponse, FGhostRunResponse>(
        APISlice::Endpoints::postGhostRun(Config)(Dispatch, GetState),
        [Dispatch](const FGhostRunResponse &Response) {
          Dispatch(GhostSlice::Actions::GhostSessionStarted(
              Response.SessionId, Response.RunStatus));
          return detail::ResolveAsync(Response);
        });
  };
}

inline ThunkAction<FGhostStatusResponse, FSDKState>
getGhostStatusThunk(const FString &SessionId) {
  return [SessionId](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<FGhostStatusResponse> {
    return func::AsyncChain::then<FGhostStatusResponse, FGhostStatusResponse>(
        APISlice::Endpoints::getGhostStatus(SessionId)(Dispatch, GetState),
        [Dispatch](const FGhostStatusResponse &Response) {
          Dispatch(GhostSlice::Actions::GhostSessionProgress(
              Response.GhostSessionId.IsEmpty() ? TEXT("") : Response.GhostSessionId,
              Response.GhostStatus, Response.GhostProgress));
          return detail::ResolveAsync(Response);
        });
  };
}

inline ThunkAction<FGhostResultsResponse, FSDKState>
getGhostResultsThunk(const FString &SessionId) {
  return [SessionId](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<FGhostResultsResponse> {
    return func::AsyncChain::then<FGhostResultsResponse,
                                  FGhostResultsResponse>(
        APISlice::Endpoints::getGhostResults(SessionId)(Dispatch, GetState),
        [Dispatch](const FGhostResultsResponse &Response) {
          FGhostTestReport Report;
          Report.TotalTests = Response.ResultsTotalTests;
          Report.PassedTests = Response.ResultsPassed;
          Report.FailedTests = Response.ResultsFailed;
          Report.SuccessRate =
              Response.ResultsTotalTests > 0
                  ? static_cast<float>(Response.ResultsPassed) /
                        static_cast<float>(Response.ResultsTotalTests)
                  : 0.0f;
          Report.Summary = FString::Printf(TEXT("%d/%d passed"),
                                           Response.ResultsPassed,
                                           Response.ResultsTotalTests);

          for (int32 Index = 0; Index < Response.ResultsTests.Num(); ++Index) {
            const FGhostResultRecord &Record = Response.ResultsTests[Index];
            FGhostTestResult TestResult;
            TestResult.Scenario = Record.TestName;
            TestResult.bPassed = Record.bTestPassed;
            TestResult.Duration = Record.TestDuration;
            TestResult.ErrorMessage = Record.TestError;
            Report.Results.Add(TestResult);
          }

          Dispatch(GhostSlice::Actions::GhostSessionCompleted(Report));
          return detail::ResolveAsync(Response);
        });
  };
}

inline ThunkAction<FGhostStopResponse, FSDKState>
stopGhostThunk(const FString &SessionId) {
  return [SessionId](std::function<AnyAction(const AnyAction &)> Dispatch,
                     std::function<FSDKState()> GetState)
             -> func::AsyncResult<FGhostStopResponse> {
    return func::AsyncChain::then<FGhostStopResponse, FGhostStopResponse>(
        APISlice::Endpoints::postGhostStop(SessionId)(Dispatch, GetState),
        [Dispatch, SessionId](const FGhostStopResponse &Response) {
          if (Response.bStopped) {
            Dispatch(GhostSlice::Actions::GhostSessionProgress(
                Response.StopSessionId.IsEmpty() ? SessionId
                                                 : Response.StopSessionId,
                Response.StopStatus.IsEmpty() ? TEXT("stopped")
                                              : Response.StopStatus,
                1.0f));
          }
          return detail::ResolveAsync(Response);
        });
  };
}

inline ThunkAction<TArray<FGhostHistoryEntry>, FSDKState>
getGhostHistoryThunk(int32 Limit = 10) {
  return [Limit](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FGhostHistoryEntry>> {
    return func::AsyncChain::then<FGhostHistoryResponse,
                                  TArray<FGhostHistoryEntry>>(
        APISlice::Endpoints::getGhostHistory(Limit)(Dispatch, GetState),
        [Dispatch](const FGhostHistoryResponse &Response) {
          Dispatch(GhostSlice::Actions::GhostHistoryLoaded(Response.Sessions));
          return detail::ResolveAsync(Response.Sessions);
        });
  };
}

inline ThunkAction<FSoul, FSDKState>
localExportSoulThunk(const FString &NpcId = TEXT("")) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<FSoul> {
    const FString TargetNpcId =
        NpcId.IsEmpty() ? NPCSlice::SelectActiveNpcId(GetState().NPCs) : NpcId;
    const auto Npc = NPCSlice::SelectNPCById(GetState().NPCs, TargetNpcId);
    if (!Npc.hasValue) {
      return detail::RejectAsync<FSoul>(TEXT("NPC not found"));
    }

    return detail::ResolveAsync(TypeFactory::Soul(
        TargetNpcId, SDKConfig::GetSdkVersion(),
        Npc.value.Persona.IsEmpty() ? TargetNpcId : Npc.value.Persona,
        Npc.value.Persona, Npc.value.State,
        MemorySlice::SelectAllMemories(GetState().Memory)));
  };
}

inline ThunkAction<FSoulExportResult, FSDKState>
remoteExportSoulThunk(const FString &NpcId = TEXT("")) {
  return [NpcId](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<FSoulExportResult> {
    const FString TargetNpcId =
        NpcId.IsEmpty() ? NPCSlice::SelectActiveNpcId(GetState().NPCs) : NpcId;
    return exportSoulThunk(TargetNpcId)(Dispatch, GetState);
  };
}

inline ThunkAction<FSoul, FSDKState>
importSoulFromArweaveThunk(const FString &TxId) {
  return importSoulThunk(TxId);
}

inline ThunkAction<TArray<FSoulListItem>, FSDKState>
getSoulListThunk(int32 Limit = 50) {
  return [Limit](std::function<AnyAction(const AnyAction &)> Dispatch,
                 std::function<FSDKState()> GetState)
             -> func::AsyncResult<TArray<FSoulListItem>> {
    return func::AsyncChain::then<FSoulListResponse, TArray<FSoulListItem>>(
        APISlice::Endpoints::getSouls(Limit)(Dispatch, GetState),
        [Dispatch](const FSoulListResponse &Response) {
          Dispatch(SoulSlice::Actions::SetSoulList(Response.Souls));
          return detail::ResolveAsync(Response.Souls);
        });
  };
}

inline ThunkAction<FSoulVerifyResult, FSDKState>
verifySoulThunk(const FString &TxId) {
  return [TxId](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<FSoulVerifyResult> {
    return APISlice::Endpoints::postSoulVerify(TxId)(Dispatch, GetState);
  };
}

inline ThunkAction<FImportedNpc, FSDKState>
importNpcFromSoulThunk(const FString &TxId) {
  return [TxId](std::function<AnyAction(const AnyAction &)> Dispatch,
                std::function<FSDKState()> GetState)
             -> func::AsyncResult<FImportedNpc> {
    return func::AsyncChain::then<FSoulImportPhase1Response, FImportedNpc>(
        APISlice::Endpoints::postNpcImport(
            TypeFactory::SoulImportPhase1Request(TxId))(Dispatch, GetState),
        [TxId, Dispatch, GetState](const FSoulImportPhase1Response &Phase1) {
          return func::AsyncChain::then<FArweaveDownloadResult, FImportedNpc>(
              detail::DownloadSoulPayload(Phase1.si1Instruction),
              [TxId, Dispatch,
               GetState](const FArweaveDownloadResult &DownloadResult) {
                return func::AsyncChain::then<FImportedNpc, FImportedNpc>(
                    APISlice::Endpoints::postNpcImportConfirm(
                        TypeFactory::SoulImportConfirmRequest(
                            TxId, DownloadResult))(Dispatch, GetState),
                    [Dispatch, TxId](const FImportedNpc &ImportedNpc) {
                      FNPCInternalState Npc;
                      Npc.Id = ImportedNpc.NpcId.IsEmpty() ? TxId
                                                           : ImportedNpc.NpcId;
                      Npc.Persona = ImportedNpc.Persona;
                      Npc.State = TypeFactory::AgentState(ImportedNpc.DataJson);
                      Dispatch(NPCSlice::Actions::SetNPCInfo(Npc));
                      return detail::ResolveAsync(ImportedNpc);
                    });
              });
        });
  };
}

} // namespace rtk
