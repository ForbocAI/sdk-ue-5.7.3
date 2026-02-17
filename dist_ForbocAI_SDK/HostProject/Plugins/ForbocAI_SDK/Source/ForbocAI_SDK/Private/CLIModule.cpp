#include "CLIModule.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

// Sends an HTTP request and pumps the HTTP tick loop until
// the response arrives or timeout is reached. This is required
// because commandlets do not run the engine's main loop.
void SendRequestAndWait(
    const FString &Url, const FString &Verb, const FString &Content,
    TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> OnComplete,
    double TimeoutSeconds = 10.0) {
  bool bCompleted = false;

  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
      FHttpModule::Get().CreateRequest();
  Request->SetURL(Url);
  Request->SetVerb(Verb);
  Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
  if (!Content.IsEmpty()) {
    Request->SetContentAsString(Content);
  }
  Request->OnProcessRequestComplete().BindLambda(
      [&bCompleted, &OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Res,
                                 bool bSuccess) {
        OnComplete(Req, Res, bSuccess);
        bCompleted = true;
      });
  Request->ProcessRequest();

  // Pump the HTTP module tick loop until complete or timeout
  const double StartTime = FPlatformTime::Seconds();
  while (!bCompleted &&
         (FPlatformTime::Seconds() - StartTime) < TimeoutSeconds) {
    FHttpModule::Get().GetHttpManager().Tick(0.05f);
    FPlatformProcess::Sleep(0.05f);
  }

  if (!bCompleted) {
    UE_LOG(LogTemp, Error, TEXT("HTTP request timed out: %s %s"), *Verb, *Url);
  }
}

namespace CLIOps {
void Doctor(const FString &ApiUrl) {
  UE_LOG(LogTemp, Display, TEXT("Running Doctor check on %s..."), *ApiUrl);

  SendRequestAndWait(
      ApiUrl + TEXT("/status"), TEXT("GET"), TEXT(""),
      [](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        if (bSuccess && Res.IsValid() && Res->GetResponseCode() == 200) {
          UE_LOG(LogTemp, Display, TEXT("API Status: ONLINE"));
          UE_LOG(LogTemp, Display, TEXT("Response: %s"),
                 *Res->GetContentAsString());
        } else if (bSuccess && Res.IsValid()) {
          UE_LOG(LogTemp, Warning, TEXT("API Status: HTTP %d"),
                 Res->GetResponseCode());
          UE_LOG(LogTemp, Warning, TEXT("Response: %s"),
                 *Res->GetContentAsString());
        } else {
          UE_LOG(LogTemp, Error, TEXT("API Status: OFFLINE or Error"));
        }
      });
}

void ListAgents(const FString &ApiUrl) {
  UE_LOG(LogTemp, Display, TEXT("Listing Agents..."));

  SendRequestAndWait(
      ApiUrl + TEXT("/agents"), TEXT("GET"), TEXT(""),
      [](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        if (bSuccess && Res.IsValid()) {
          UE_LOG(LogTemp, Display, TEXT("Agents: %s"),
                 *Res->GetContentAsString());
        } else {
          UE_LOG(LogTemp, Error, TEXT("Failed to list agents"));
        }
      });
}

void CreateAgent(const FString &ApiUrl, const FString &Persona) {
  UE_LOG(LogTemp, Display, TEXT("Creating Agent with Persona: %s"), *Persona);

  FString JsonPayload = FString::Printf(
      TEXT("{\"createPersona\": \"%s\", \"cortexRef\": \"ue-cli\"}"), *Persona);

  SendRequestAndWait(
      ApiUrl + TEXT("/agents"), TEXT("POST"), JsonPayload,
      [](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        if (bSuccess && Res.IsValid()) {
          UE_LOG(LogTemp, Display, TEXT("Created: %s"),
                 *Res->GetContentAsString());
        } else {
          UE_LOG(LogTemp, Error, TEXT("Failed to create agent"));
        }
      });
}

void ProcessAgent(const FString &ApiUrl, const FString &AgentId,
                  const FString &Input) {
  UE_LOG(LogTemp, Display, TEXT("Processing Agent %s Input: %s"), *AgentId,
         *Input);

  FString JsonPayload = FString::Printf(
      TEXT("{\"input\": \"%s\", \"context\": [[\"source\", \"ue-cli\"]] }"),
      *Input);

  SendRequestAndWait(
      ApiUrl + TEXT("/agents/") + AgentId + TEXT("/process"), TEXT("POST"),
      JsonPayload,
      [](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        if (bSuccess && Res.IsValid()) {
          UE_LOG(LogTemp, Display, TEXT("Response: %s"),
                 *Res->GetContentAsString());
        } else {
          UE_LOG(LogTemp, Error, TEXT("Failed to process agent"));
        }
      });
}

void ExportSoul(const FString &ApiUrl, const FString &AgentId) {
  UE_LOG(LogTemp, Display, TEXT("Exporting Soul for Agent %s"), *AgentId);

  FString JsonPayload =
      FString::Printf(TEXT("{\"agentIdRef\": \"%s\"}"), *AgentId);

  SendRequestAndWait(
      ApiUrl + TEXT("/agents/") + AgentId + TEXT("/soul/export"), TEXT("POST"),
      JsonPayload,
      [](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        if (bSuccess && Res.IsValid()) {
          UE_LOG(LogTemp, Display, TEXT("Exported: %s"),
                 *Res->GetContentAsString());
        } else {
          UE_LOG(LogTemp, Error, TEXT("Failed to export soul"));
        }
      });
}
} // namespace CLIOps
