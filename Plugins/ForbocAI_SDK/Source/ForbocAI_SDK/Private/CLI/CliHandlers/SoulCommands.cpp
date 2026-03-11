#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "RuntimeStore.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleSoul(rtk::EnhancedStore<FStoreState> &Store,
                        const FString &CommandKey,
                        const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("soul_export")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: soul_export <npcId>"));
    }
    FSoulExportResult Exported = Ops::ExportSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Soul exported: %s"), *Exported.TxId);
    return just(Result::Success("Soul exported"));
  }

  if (CommandKey == TEXT("soul_import")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: soul_import <txId>"));
    }
    FSoul Imported = Ops::ImportSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Soul imported: %s"), *Imported.Id);
    return just(Result::Success("Soul imported"));
  }

  if (CommandKey == TEXT("soul_import_npc")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: soul_import_npc <txId>"));
    }
    FImportedNpc Npc = Ops::ImportNpcFromSoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("NPC imported from soul: %s"), *Npc.NpcId);
    return just(Result::Success("NPC imported from soul"));
  }

  if (CommandKey == TEXT("soul_list")) {
    int32 Limit = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 50;
    TArray<FSoulListItem> Souls = Ops::ListSouls(Store, Limit);
    UE_LOG(LogTemp, Display, TEXT("Found %d souls"), Souls.Num());
    return just(Result::Success("Souls listed"));
  }

  if (CommandKey == TEXT("soul_verify")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: soul_verify <txId>"));
    }
    FSoulVerifyResult Verified = Ops::VerifySoul(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Soul verification: %s"),
           Verified.bValid ? TEXT("VALID") : TEXT("INVALID"));
    return just(Result::Success("Soul verified"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
