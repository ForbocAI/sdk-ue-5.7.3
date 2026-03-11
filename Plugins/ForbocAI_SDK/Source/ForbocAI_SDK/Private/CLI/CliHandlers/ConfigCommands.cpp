#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleConfig(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args) {
  (void)Store;
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("config_set")) {
    if (Args.Num() < 2) {
      return just(Result::Failure("Usage: config_set <key> <value>"));
    }
    Ops::ConfigSet(Args[0], Args[1]);
    return just(Result::Success("Config updated"));
  }

  if (CommandKey == TEXT("config_get")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: config_get <key>"));
    }
    FString Value = Ops::ConfigGet(Args[0]);
    UE_LOG(LogTemp, Display, TEXT("%s = %s"), *Args[0], *Value);
    return just(Result::Success("Config retrieved"));
  }

  if (CommandKey == TEXT("config_list")) {
    static const TArray<FString> ConfigKeys = {
        TEXT("version"), TEXT("apiUrl"), TEXT("apiKey"), TEXT("modelPath"),
        TEXT("databasePath"), TEXT("vectorDimension"), TEXT("maxRecallResults")};
    for (const FString &Key : ConfigKeys) {
      FString Value = Ops::ConfigGet(Key);
      bool bMask = Key.Contains(TEXT("key"), ESearchCase::IgnoreCase) ||
                   Key.Contains(TEXT("Key"), ESearchCase::CaseSensitive);
      UE_LOG(LogTemp, Display, TEXT("  %s = %s"), *Key,
             bMask && !Value.IsEmpty() ? TEXT("********") : *Value);
    }
    return just(Result::Success("Config listed"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
