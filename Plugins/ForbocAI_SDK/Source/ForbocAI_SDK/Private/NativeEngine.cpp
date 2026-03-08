#include "NativeEngine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_FORBOC_NATIVE
#include "llama.h"
// #include "sqlite_vss.h" // Assume this exists or similar
#endif

namespace Native {
namespace Llama {

Context LoadModel(const FString &Path) {
#if WITH_FORBOC_NATIVE
  auto utf8Path = StringCast<UTF8CHAR>(*Path);
  return reinterpret_cast<Context>(llama::load_model(utf8Path.Get()));
#else
  // Mock Implementation for CI/Testing
  if (FPaths::FileExists(Path) || Path.Contains(TEXT("test_model.bin"))) {
    return reinterpret_cast<Context>(new int(42));
  }
  return nullptr;
#endif
}

void FreeModel(Context Ctx) {
  if (!Ctx)
    return;
#if WITH_FORBOC_NATIVE
  llama::free_model(reinterpret_cast<llama::context *>(Ctx));
#else
  delete reinterpret_cast<int *>(Ctx);
#endif
}

// The user's instruction seems to want to add a new Embed function and
// potentially modify or add another Infer function.
// Given the context, the most sensible interpretation is to add the Embed
// function and keep the existing Infer function as is, as the provided
// "new Infer" is a mock and the existing one already has a mock.
// The insertion point is after FreeModel and before the existing Infer.

TArray<float> Embed(Context Ctx, const FString &Text) {
  // Mock implementation: generate 384-dimensional vector (MiniLM size)
  TArray<float> Vector;
  Vector.SetNumZeroed(384);
  if (!Text.IsEmpty()) {
    Vector[0] = static_cast<float>(Text.Len()) / 100.0f;
  }
  return Vector;
}

FString Infer(Context Ctx, const FString &Prompt, int32 MaxTokens) {
  if (!Ctx)
    return TEXT("Error: Model not loaded");
#if WITH_FORBOC_NATIVE
  auto utf8Prompt = StringCast<UTF8CHAR>(*Prompt);
  auto result = llama::infer(reinterpret_cast<llama::context *>(Ctx),
                             utf8Prompt.Get(), MaxTokens);
  return FString(UTF8_TO_TCHAR(result));
#else
  FPlatformProcess::Sleep(0.1f); // Simulate work
  return FString::Printf(TEXT("Simulated local response for: %s"),
                         *Prompt.Left(30));
#endif
}

} // namespace Llama

namespace Sqlite {

DB Open(const FString &Path) {
#if WITH_FORBOC_NATIVE
  // Real sqlite-vss open logic
  return nullptr; // Placeholder for real implementation
#else
  return reinterpret_cast<DB>(new int(1337));
#endif
}

void Close(DB Database) {
  if (!Database)
    return;
#if WITH_FORBOC_NATIVE
  // Real close logic
#else
  delete reinterpret_cast<int *>(Database);
#endif
}

TArray<FString> Search(DB Database, const TArray<float> &Vector, int32 TopK) {
  TArray<FString> Results;
#if WITH_FORBOC_NATIVE
  // Real vss search logic
#else
  Results.Add(TEXT("memory_mock_1"));
  Results.Add(TEXT("memory_mock_2"));
#endif
  return Results;
}

void Store(DB Database, const FString &Id, const TArray<float> &Vector,
           const FString &Metadata) {
#if WITH_FORBOC_NATIVE
  // Real store logic
#endif
}

} // namespace Sqlite
} // namespace Native
