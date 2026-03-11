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

  rtk::EnhancedStore<FStoreState> &Store = GetStore();

  try {
    HandlerResult R;

    R = HandleSystem(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleNpc(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleMemory(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleCortex(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleGhost(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleBridge(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleSoul(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleConfig(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    R = HandleVector(Store, CommandKey, Args);
    if (R.hasValue) {
      return R.value;
    }

    return Result::Failure(
        TCHAR_TO_UTF8(*FString::Printf(TEXT("Unknown command: %s"), *CommandKey)));
  } catch (const std::exception &Error) {
    return Result::Failure(std::string(Error.what()));
  }
}

} // namespace CLIOps
