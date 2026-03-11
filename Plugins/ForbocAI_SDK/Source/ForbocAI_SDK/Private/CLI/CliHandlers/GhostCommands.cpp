#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleGhost(rtk::EnhancedStore<FStoreState> &Store,
                         const FString &CommandKey,
                         const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("ghost_run")) {
    FString Suite = Args.Num() > 0 ? Args[0] : TEXT("default");
    int32 Duration = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 300;
    FGhostRunResponse Resp = Ops::GhostRun(Store, Suite, Duration);
    UE_LOG(LogTemp, Display, TEXT("Ghost session started: %s"), *Resp.SessionId);
    return just(Result::Success("Ghost run started"));
  }

  if (CommandKey == TEXT("ghost_status")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: ghost_status <sessionId>"));
    }
    FGhostStatusResponse Resp = Ops::GhostStatus(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Ghost status: %s"), *Resp.GhostStatus);
    return just(Result::Success("Ghost status retrieved"));
  }

  if (CommandKey == TEXT("ghost_results")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: ghost_results <sessionId>"));
    }
    Ops::GhostResults(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Ghost results retrieved"));
    return just(Result::Success("Ghost results retrieved"));
  }

  if (CommandKey == TEXT("ghost_stop")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: ghost_stop <sessionId>"));
    }
    FGhostStopResponse Resp = Ops::GhostStop(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Ghost stopped: %s"), *Resp.StopStatus);
    return just(Result::Success("Ghost stopped"));
  }

  if (CommandKey == TEXT("ghost_history")) {
    int32 Limit = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 10;
    TArray<FGhostHistoryEntry> History = Ops::GhostHistory(Store, Limit);
    UE_LOG(LogTemp, Display, TEXT("Ghost history: %d entries"), History.Num());
    return just(Result::Success("Ghost history retrieved"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
