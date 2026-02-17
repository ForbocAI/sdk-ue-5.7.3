#include "CLIModule.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Core/functional_core.hpp"

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
al      (FPlatformTime::Seconds() - StartTime) < TimeoutSeconds) {
    FHttpModule::Get().GetHttpManager().Tick(0.05f);
    FPlatformProcess::Sleep(0.05f);
  }

  if (!bCompleted) {
    UE_LOG(LogTemp, Error, TEXT("HTTP request timed out: %s %s"), *Verb, *Url);
  }
}

namespace CLIOps {

// Functional implementation of CLI commands
CLITypes::TestResult<void> Doctor(const FString &ApiUrl) {
  UE_LOG(LogTemp, Display, TEXT("Running Doctor check on %s..."), *ApiUrl);

  // Use functional pipeline for request processing
  auto requestPipeline = func::pipe(ApiUrl + TEXT("/status"))
      | [](const FString& url) {
          return func::lazy([url]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("GET"), TEXT(""),
                  [&result](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
                      if (bSuccess && Res.IsValid() && Res->GetResponseCode() == 200) {
                          result = FString::Printf(TEXT("API Status: ONLINE\nResponse: %s"), *Res->GetContentAsString());
                      } else if (bSuccess && Res.IsValid()) {
                          result = FString::Printf(TEXT("API Status: HTTP %d\nResponse: %s"),
                                                 Res->GetResponseCode(),
                                                 *Res->GetContentAsString());
                      } else {
                          result = FString(TEXT("API Status: OFFLINE or Error"));
                      }
                  });
              return result;
          });
      };

  FString result = func::eval(requestPipeline);
  
  if (result.Contains(TEXT("ONLINE"))) {
      UE_LOG(LogTemp, Display, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::success(TEXT("Doctor check completed successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::failure(result);
  }
}

CLITypes::TestResult<void> ListAgents(const FString &ApiUrl) {
  UE_LOG(LogTemp, Display, TEXT("Listing Agents..."));

  // Use functional pipeline for request processing
  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents"))
      | [](const FString& url) {
          return func::lazy([url]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("GET"), TEXT(""),
                  [&result](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
                      if (bSuccess && Res.IsValid()) {
                          result = FString::Printf(TEXT("Agents: %s"), *Res->GetContentAsString());
                      } else {
                          result = FString(TEXT("Failed to list agents"));
                      }
                  });
              return result;
          });
      };

  FString result = func::eval(requestPipeline);
  
  if (result.Contains(TEXT("Agents:"))) {
      UE_LOG(LogTemp, Display, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::success(TEXT("Agent listing completed successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::failure(result);
  }
}

CLITypes::TestResult<void> CreateAgent(const FString &ApiUrl, const FString &Persona) {
  UE_LOG(LogTemp, Display, TEXT("Creating Agent with Persona: %s"), *Persona);

  FString JsonPayload = FString::Printf(
      TEXT("{\"createPersona\": \"%s\", \"cortexRef\": \"ue-cli\"}"), *Persona);

  // Use functional pipeline for request processing
  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents"))
      | [JsonPayload](const FString& url) {
          return func::lazy([url, JsonPayload]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("POST"), JsonPayload,
                  [&result](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
                      if (bSuccess && Res.IsValid()) {
                          result = FString::Printf(TEXT("Created: %s"), *Res->GetContentAsString());
                      } else {
                          result = FString(TEXT("Failed to create agent"));
                      }
                  });
              return result;
          });
      };

  FString result = func::eval(requestPipeline);
  
  if (result.Contains(TEXT("Created:"))) {
      UE_LOG(LogTemp, Display, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::success(TEXT("Agent created successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::failure(result);
  }
}

CLITypes::TestResult<void> ProcessAgent(const FString &ApiUrl, const FString &AgentId,
                                          const FString &Input) {
  UE_LOG(LogTemp, Display, TEXT("Processing Agent %s Input: %s"), *AgentId,
         *Input);

  FString JsonPayload = FString::Printf(
      TEXT("{\"input\": \"%s\", \"context\": [[\"source\", \"ue-cli\"]] }"),
      *Input);

  // Use functional pipeline for request processing
  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents/") + AgentId + TEXT("/process"))
      | [JsonPayload](const FString& url) {
          return func::lazy([url, JsonPayload]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("POST"), JsonPayload,
                  [&result](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
                      if (bSuccess && Res.IsValid()) {
                          result = FString::Printf(TEXT("Response: %s"), *Res->GetContentAsString());
                      } else {
                          result = FString(TEXT("Failed to process agent"));
                      }
                  });
              return result;
          });
      };

  FString result = func::eval(requestPipeline);
  
  if (result.Contains(TEXT("Response:"))) {
      UE_LOG(LogTemp, Display, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::success(TEXT("Agent processed successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::failure(result);
  }
}

CLITypes::TestResult<void> ExportSoul(const FString &ApiUrl, const FString &AgentId) {
  UE_LOG(LogTemp, Display, TEXT("Exporting Soul for Agent %s"), *AgentId);

  FString JsonPayload =
      FString::Printf(TEXT("{\"agentIdRef\": \"%s\"}"), *AgentId);

  // Use functional pipeline for request processing
  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents/") + AgentId + TEXT("/soul/export"))
      | [JsonPayload](const FString& url) {
          return func::lazy([url, JsonPayload]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("POST"), JsonPayload,
                  [&result](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
                      if (bSuccess && Res.IsValid()) {
                          result = FString::Printf(TEXT("Exported: %s"), *Res->GetContentAsString());
                      } else {
                          result = FString(TEXT("Failed to export soul"));
                      }
                  });
              return result;
          });
      };

  FString result = func::eval(requestPipeline);
  
  if (result.Contains(TEXT("Exported:"))) {
      UE_LOG(LogTemp, Display, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::success(TEXT("Soul exported successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::failure(result);
  }
}

} // namespace CLIOps