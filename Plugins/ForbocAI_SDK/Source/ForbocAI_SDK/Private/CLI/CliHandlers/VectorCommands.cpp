#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleVector(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args) {
  (void)Args;
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("vector_init")) {
    Ops::InitVector(Store);
    return just(Result::Success("Vector initialized"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
