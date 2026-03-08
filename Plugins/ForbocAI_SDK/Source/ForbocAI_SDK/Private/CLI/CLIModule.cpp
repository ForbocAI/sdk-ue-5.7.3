#include "CLI/CLIModule.h"
#include "Core/functional_core.hpp"
#include "HttpManager.h"
#include "HttpModule.h"
#include "SDKConfig.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"
#include "Thunks.h"

// WaitForResult pumps the HTTP tick loop until the AsyncResult completes.
// This allows asynchronous thunks to be used in a synchronous CLI command.
template <typename T>
T WaitForResult(func::AsyncResult<T> &&Async, double TimeoutSeconds = 15.0) {
  bool bCompleted = false;
  T Result{};

  Async.then([&bCompleted, &Result](T Value) {
    Result = Value;
    bCompleted = true;
  });

  const double StartTime = FPlatformTime::Seconds();
  while (!bCompleted &&
         (FPlatformTime::Seconds() - StartTime) < TimeoutSeconds) {
    FHttpModule::Get().GetHttpManager().Tick(0.05f);
    FPlatformProcess::Sleep(0.05f);
  }

  return Result;
}

namespace CLIOps {

CLITypes::TestResult<void> Doctor(const FString &ApiUrl) {
  UE_LOG(LogTemp, Display, TEXT("Running Doctor check via RTK Store..."));

  SDKConfig::SetApiConfig(ApiUrl, SDKConfig::GetApiKey());
  auto Store = ConfigureSDKStore();
  auto Result = WaitForResult(Store.dispatch(rtk::doctorThunk()));

  if (Result.Status == TEXT("ONLINE") || Result.Status == TEXT("ok")) {
    UE_LOG(LogTemp, Display, TEXT("API Status: %s (v%s)"), *Result.Status,
           *Result.Version);
    return CLITypes::TestResult<void>::Success(
        "Doctor check completed successfully");
  } else {
    return CLITypes::TestResult<void>::Failure("API is offline or unreachable");
  }
}

CLITypes::TestResult<void> ListAgents(const FString &ApiUrl) {
  UE_LOG(LogTemp, Display, TEXT("Listing Agents via RTK Store..."));

  SDKConfig::SetApiConfig(ApiUrl, SDKConfig::GetApiKey());
  auto Store = ConfigureSDKStore();
  auto Result = WaitForResult(Store.dispatch(APISlice::Endpoints::getNPCs()));

  UE_LOG(LogTemp, Display, TEXT("Found %d agents"), Result.Num());
  for (const auto &Agent : Result) {
    UE_LOG(LogTemp, Display, TEXT("- %s (%s)"), *Agent.Id, *Agent.Persona);
  }

  return CLITypes::TestResult<void>::Success("Agents listed");
}

CLITypes::TestResult<void> CreateAgent(const FString &ApiUrl,
                                       const FString &Persona) {
  UE_LOG(LogTemp, Display, TEXT("Creating Agent via RTK Store..."));

  FAgentConfig Config;
  Config.Persona = Persona;
  Config.ApiUrl = ApiUrl;

  SDKConfig::SetApiConfig(ApiUrl, SDKConfig::GetApiKey());
  auto Store = ConfigureSDKStore();
  auto Result =
      WaitForResult(Store.dispatch(APISlice::Endpoints::postNPC(Config)));

  UE_LOG(LogTemp, Display, TEXT("Created Agent: %s"), *Result.Id);
  return CLITypes::TestResult<void>::Success("Agent created");
}

CLITypes::TestResult<void> ProcessAgent(const FString &ApiUrl,
                                        const FString &AgentId,
                                        const FString &Input) {
  UE_LOG(LogTemp, Display, TEXT("Processing NPC Protocol via RTK Store..."));

  SDKConfig::SetApiConfig(ApiUrl, SDKConfig::GetApiKey());
  auto Store = ConfigureSDKStore();
  // Using the real processNPC thunk which handles the recursive loop
  auto Result = WaitForResult(Store.dispatch(rtk::processNPC(AgentId, Input)));

  UE_LOG(LogTemp, Display, TEXT("Final Verdict: %s"), *Result.Dialogue);
  return CLITypes::TestResult<void>::Success("Protocol complete");
}

CLITypes::TestResult<void> ExportSoul(const FString &ApiUrl,
                                      const FString &AgentId) {
  UE_LOG(LogTemp, Display, TEXT("Exporting Soul via RTK Store..."));

  SDKConfig::SetApiConfig(ApiUrl, SDKConfig::GetApiKey());
  auto Store = ConfigureSDKStore();
  auto Result = WaitForResult(Store.dispatch(rtk::exportSoulThunk(AgentId)));

  UE_LOG(LogTemp, Display, TEXT("Soul Exported to: %s"), *Result.TxId);
  return CLITypes::TestResult<void>::Success("Soul exported");
}

} // namespace CLIOps
