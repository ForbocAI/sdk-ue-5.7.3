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
    const FString &ApiKey,
    TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> OnComplete,
    double TimeoutSeconds = 10.0) {
  bool bCompleted = false;

  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
      FHttpModule::Get().CreateRequest();
  Request->SetURL(Url);
  Request->SetVerb(Verb);
  Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
  if (!ApiKey.IsEmpty()) {
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + ApiKey);
  }
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

// Functional implementation of CLI commands

CLITypes::TestResult<void> Doctor(const FString &ApiUrl) {
  return Doctor(ApiUrl, TEXT(""));
}

CLITypes::TestResult<void> Doctor(const FString &ApiUrl, const FString &ApiKey) {
  UE_LOG(LogTemp, Display, TEXT("Running Doctor check on %s..."), *ApiUrl);

  auto requestPipeline = func::pipe(ApiUrl + TEXT("/status"))
      | [ApiKey](const FString& url) {
          return func::lazy([url, ApiKey]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("GET"), TEXT(""), ApiKey,
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
      return CLITypes::TestResult<void>::Success(std::string("Doctor check completed successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::Failure(std::string(TCHAR_TO_UTF8(*result)));
  }
}

CLITypes::TestResult<void> ListAgents(const FString &ApiUrl) {
  return ListAgents(ApiUrl, TEXT(""));
}

CLITypes::TestResult<void> ListAgents(const FString &ApiUrl, const FString &ApiKey) {
  UE_LOG(LogTemp, Display, TEXT("Listing Agents..."));

  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents"))
      | [ApiKey](const FString& url) {
          return func::lazy([url, ApiKey]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("GET"), TEXT(""), ApiKey,
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
      return CLITypes::TestResult<void>::Success(std::string("Agent listing completed successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::Failure(std::string(TCHAR_TO_UTF8(*result)));
  }
}

CLITypes::TestResult<void> CreateAgent(const FString &ApiUrl, const FString &Persona) {
  return CreateAgent(ApiUrl, Persona, TEXT(""));
}

CLITypes::TestResult<void> CreateAgent(const FString &ApiUrl, const FString &Persona, const FString &ApiKey) {
  UE_LOG(LogTemp, Display, TEXT("Creating Agent with Persona: %s"), *Persona);

  FString JsonPayload = FString::Printf(
      TEXT("{\"createPersona\": \"%s\", \"cortexRef\": \"ue-cli\"}"), *Persona);

  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents"))
      | [JsonPayload, ApiKey](const FString& url) {
          return func::lazy([url, JsonPayload, ApiKey]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("POST"), JsonPayload, ApiKey,
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
      return CLITypes::TestResult<void>::Success(std::string("Agent created successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::Failure(std::string(TCHAR_TO_UTF8(*result)));
  }
}

// ProcessAgent uses the lightweight /speak endpoint (conversational, no full directive cycle).
// For full multi-round protocol, use the agent.process() flow from AgentModule.
CLITypes::TestResult<void> ProcessAgent(const FString &ApiUrl, const FString &AgentId,
                                          const FString &Input) {
  return ProcessAgent(ApiUrl, AgentId, Input, TEXT(""));
}

CLITypes::TestResult<void> ProcessAgent(const FString &ApiUrl, const FString &AgentId,
                                          const FString &Input, const FString &ApiKey) {
  UE_LOG(LogTemp, Display, TEXT("Speaking to Agent %s: %s"), *AgentId, *Input);

  FString JsonPayload = FString::Printf(
      TEXT("{\"speakMessage\": \"%s\", \"speakAgentState\": {}}"), *Input);

  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents/") + AgentId + TEXT("/speak"))
      | [JsonPayload, ApiKey](const FString& url) {
          return func::lazy([url, JsonPayload, ApiKey]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("POST"), JsonPayload, ApiKey,
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
      return CLITypes::TestResult<void>::Success(std::string("Agent processed successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::Failure(std::string(TCHAR_TO_UTF8(*result)));
  }
}

CLITypes::TestResult<void> ExportSoul(const FString &ApiUrl, const FString &AgentId) {
  return ExportSoul(ApiUrl, AgentId, TEXT(""));
}

CLITypes::TestResult<void> ExportSoul(const FString &ApiUrl, const FString &AgentId, const FString &ApiKey) {
  UE_LOG(LogTemp, Display, TEXT("Exporting Soul for Agent %s"), *AgentId);

  FString JsonPayload =
      FString::Printf(TEXT("{\"agentIdRef\": \"%s\"}"), *AgentId);

  auto requestPipeline = func::pipe(ApiUrl + TEXT("/agents/") + AgentId + TEXT("/soul/export"))
      | [JsonPayload, ApiKey](const FString& url) {
          return func::lazy([url, JsonPayload, ApiKey]() {
              FString result;
              SendRequestAndWait(
                  url, TEXT("POST"), JsonPayload, ApiKey,
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
      return CLITypes::TestResult<void>::Success(std::string("Soul exported successfully"));
  } else {
      UE_LOG(LogTemp, Error, TEXT("%s"), *result);
      return CLITypes::TestResult<void>::Failure(std::string(TCHAR_TO_UTF8(*result)));
  }
}

} // namespace CLIOps
