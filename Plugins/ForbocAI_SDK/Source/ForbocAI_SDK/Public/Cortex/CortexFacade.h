#pragma once
// G4: ICortex function-pointer struct for testability

#include "Cortex/CortexTypes.h"
#include "NativeEngine.h"

namespace Cortex {

/**
 * Function-pointer struct wrapping Native::Llama:: operations.
 * Defaults to real implementations. Swap individual pointers for
 * test doubles without touching thunk code.
 *
 * Usage:
 *   // Production (default)
 *   auto &Ops = Cortex::GetOps();
 *   auto Ctx = Ops.LoadModel(Path);
 *
 *   // Test override
 *   Cortex::FCortexOps TestOps;
 *   TestOps.Infer = [](void*, const FString&, const FCortexConfig&) {
 *       return FString(TEXT("test response"));
 *   };
 *   Cortex::SetOps(TestOps);
 */
struct FCortexOps {
  std::function<Native::Llama::Context(const FString &)> LoadModel;
  std::function<Native::Llama::Context(const FString &)> LoadEmbeddingModel;
  std::function<void(Native::Llama::Context)> FreeModel;
  std::function<FString(Native::Llama::Context, const FString &,
                         const FCortexConfig &)>
      Infer;
  std::function<FString(Native::Llama::Context, const FString &,
                         const FCortexConfig &,
                         const Native::Llama::TokenCallback &)>
      InferStream;
  std::function<TArray<float>(Native::Llama::Context, const FString &)> Embed;

  FCortexOps()
      : LoadModel(Native::Llama::LoadModel),
        LoadEmbeddingModel(Native::Llama::LoadEmbeddingModel),
        FreeModel(Native::Llama::FreeModel),
        Infer([](Native::Llama::Context Ctx, const FString &Prompt,
                 const FCortexConfig &Config) {
          return Native::Llama::Infer(Ctx, Prompt, Config);
        }),
        InferStream(Native::Llama::InferStream),
        Embed(Native::Llama::Embed) {}
};

inline FCortexOps &GetOps() {
  static FCortexOps Ops;
  return Ops;
}

inline void SetOps(const FCortexOps &Ops) { GetOps() = Ops; }

inline void ResetOps() { GetOps() = FCortexOps(); }

} // namespace Cortex
