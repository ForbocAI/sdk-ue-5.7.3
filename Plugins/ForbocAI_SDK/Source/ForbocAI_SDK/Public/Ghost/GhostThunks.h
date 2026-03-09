#pragma once

#include "Core/ThunkDetail.h"
#include "Errors.h"
#include "Ghost/GhostSlice.h"
#include "RuntimeConfig.h"

namespace rtk {

// ---------------------------------------------------------------------------
// Ghost thunks (mirrors TS ghostSlice.ts)
// ---------------------------------------------------------------------------

inline ThunkAction<FGhostRunResponse, FSDKState>
startGhostThunk(const FGhostConfig &Config) {
  return [Config](std::function<AnyAction(const AnyAction &)> Dispatch,
                  std::function<FSDKState()> GetState)
             -> func::AsyncResult<FGhostRunResponse> {
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FGhostRunResponse>(ApiKeyError.value);
    }
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
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FGhostStatusResponse>(ApiKeyError.value);
    }
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
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FGhostResultsResponse>(ApiKeyError.value);
    }

    return func::AsyncChain::then<FGhostResultsResponse,
                                  FGhostResultsResponse>(
        APISlice::Endpoints::getGhostResults(SessionId)(Dispatch, GetState),
        [Dispatch](const FGhostResultsResponse &Response) {
          FGhostTestReport Report;
          Report.SessionId = Response.ResultsSessionId;
          Report.TotalTests = Response.ResultsTotalTests;
          Report.PassedTests = Response.ResultsPassed;
          Report.FailedTests = Response.ResultsFailed;
          Report.SkippedTests = Response.ResultsSkipped;
          Report.Duration = Response.ResultsDuration;
          Report.Coverage = Response.ResultsCoverage;
          Report.Metrics = Response.ResultsMetrics;
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
            TestResult.Screenshot = Record.TestScreenshot;
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
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<FGhostStopResponse>(ApiKeyError.value);
    }
    return func::AsyncChain::then<FGhostStopResponse, FGhostStopResponse>(
        APISlice::Endpoints::postGhostStop(SessionId)(Dispatch, GetState),
        [Dispatch, SessionId](const FGhostStopResponse &Response) {
          const bool bStopped =
              Response.bStopped ||
              Response.StopStatus.Equals(TEXT("stopped"),
                                         ESearchCase::IgnoreCase);
          if (bStopped) {
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
    const auto ApiKeyError = Errors::requireApiKeyGuidance(
        SDKConfig::GetApiUrl(), SDKConfig::GetApiKey());
    if (ApiKeyError.hasValue) {
      return detail::RejectAsync<TArray<FGhostHistoryEntry>>(ApiKeyError.value);
    }
    return func::AsyncChain::then<FGhostHistoryResponse,
                                  TArray<FGhostHistoryEntry>>(
        APISlice::Endpoints::getGhostHistory(Limit)(Dispatch, GetState),
        [Dispatch](const FGhostHistoryResponse &Response) {
          Dispatch(GhostSlice::Actions::GhostHistoryLoaded(Response.Sessions));
          return detail::ResolveAsync(Response.Sessions);
        });
  };
}

} // namespace rtk
