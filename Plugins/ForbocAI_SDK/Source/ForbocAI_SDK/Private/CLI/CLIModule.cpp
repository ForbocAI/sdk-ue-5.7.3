#include "CLI/CLIModule.h"
#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"
#include <exception>

namespace CLIOps {

namespace {

rtk::EnhancedStore<FStoreState> &GetStore() {
  static rtk::EnhancedStore<FStoreState> Store = ConfigureStore();
  return Store;
}

} // namespace

func::TestResult<void> DispatchCommand(const FString &CommandKey,
                                       const TArray<FString> &Args) {
  using Result = func::TestResult<void>;
  using namespace Handlers;
  using Handler = std::function<HandlerResult(rtk::EnhancedStore<FStoreState> &,
                                              const FString &,
                                              const TArray<FString> &)>;

  rtk::EnhancedStore<FStoreState> &Store = GetStore();

  // Phase 3.4: Handler chain — first match wins
  static const std::vector<Handler> Handlers = {
      HandleSystem, HandleNpc,    HandleMemory, HandleCortex, HandleGhost,
      HandleBridge, HandleSoul,   HandleConfig, HandleVector, HandleSetup,
  };

  try {
    for (const auto &H : Handlers) {
      HandlerResult R = H(Store, CommandKey, Args);
      if (R.hasValue) {
        return R.value;
      }
    }
    return Result::Failure(
        TCHAR_TO_UTF8(*FString::Printf(TEXT("Unknown command: %s"), *CommandKey)));
  } catch (const std::exception &Error) {
    return Result::Failure(std::string(Error.what()));
  }
}

} // namespace CLIOps
