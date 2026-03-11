#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "Dom/JsonObject.h"
#include "RuntimeStore.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace CLIOps {
namespace Handlers {

HandlerResult HandleMemory(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("memory_list")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: memory_list <npcId>"));
    }
    TArray<FMemoryItem> Items = Ops::MemoryList(Store, Args[0]);
    UE_LOG(LogTemp, Display, TEXT("Found %d memories"), Items.Num());
    return just(Result::Success("Memories listed"));
  }

  if (CommandKey == TEXT("memory_recall")) {
    if (Args.Num() < 2) {
      return just(Result::Failure("Usage: memory_recall <npcId> <query>"));
    }
    TArray<FMemoryItem> Items = Ops::MemoryRecall(Store, Args[0], Args[1]);
    UE_LOG(LogTemp, Display, TEXT("Recalled %d memories"), Items.Num());
    return just(Result::Success("Memories recalled"));
  }

  if (CommandKey == TEXT("memory_store")) {
    if (Args.Num() < 2) {
      return just(Result::Failure("Usage: memory_store <npcId> <observation>"));
    }
    Ops::MemoryStore(Store, Args[0], Args[1]);
    return just(Result::Success("Memory stored"));
  }

  if (CommandKey == TEXT("memory_clear")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: memory_clear <npcId>"));
    }
    Ops::MemoryClear(Store, Args[0]);
    return just(Result::Success("Memory cleared"));
  }

  if (CommandKey == TEXT("memory_export")) {
    if (Args.Num() < 1) {
      return just(Result::Failure("Usage: memory_export <npcId>"));
    }
    TArray<FMemoryItem> Items = Ops::MemoryList(Store, Args[0]);
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> JsonItems;
    for (const FMemoryItem &Item : Items) {
      TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
      Obj->SetStringField(TEXT("text"), Item.Text);
      Obj->SetStringField(TEXT("type"), Item.Type);
      Obj->SetNumberField(TEXT("importance"), Item.Importance);
      JsonItems.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("memories"), JsonItems);
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Root, Writer);
    UE_LOG(LogTemp, Display, TEXT("%s"), *JsonString);
    return just(Result::Success("Memories exported"));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
