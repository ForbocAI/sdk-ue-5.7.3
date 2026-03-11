#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "Cortex/CortexTypes.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

namespace {

FString DescribeCortexStatus(const FCortexStatus &Status) {
  if (Status.bReady) {
    return TEXT("ready");
  }
  if (!Status.Error.IsEmpty()) {
    return Status.Error;
  }
  return TEXT("initializing");
}

} // namespace

HandlerResult HandleCortex(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("cortex_init")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: cortex_init <model>"));
    }
    FCortexStatus Status = Ops::InitCortex(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Cortex initialized: %s"),
           *DescribeCortexStatus(Status));
    return just(Result::Success("Cortex initialized"));
  }

  if (CommandKey == TEXT("cortex_init_remote")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: cortex_init_remote <model> [authKey]"));
    }
    FString AuthKey = Args.Num() > 1 ? Args[1] : TEXT("");
    FCortexStatus Status = Ops::InitRemoteCortex(Store, Args[0], AuthKey);
    UE_LOG(LogTemp, Display, TEXT("Remote cortex initialized: %s"),
           *DescribeCortexStatus(Status));
    return just(Result::Success("Remote cortex initialized"));
  }

  if (CommandKey == TEXT("cortex_models")) {
    TArray<FCortexModelInfo> Models = Ops::ListCortexModels(Store);
    UE_LOG(LogTemp, Display, TEXT("Found %d models"), Models.Num());
    return just(Result::Success("Models listed"));
  }

  if (CommandKey == TEXT("cortex_complete")) {
    if (Args.Num() < 2) {
      return just(Result::Failure("Usage: cortex_complete <cortexId> <prompt>"));
    }
    FCortexResponse Resp = Ops::CompleteRemoteCortex(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("Completion: %s"), *Resp.Text);
    return just(Result::Success("Completion done"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
