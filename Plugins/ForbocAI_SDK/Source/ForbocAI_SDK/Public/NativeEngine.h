#pragma once

#include "CoreMinimal.h"
#include "Cortex/CortexTypes.h"
#include "Memory/MemoryTypes.h"

/**
 * Native Engine Wrappers
 *
 * Isolates 3rd-party library (llama.cpp, sqlite-vss) mechanics from the SDK
 * logic. Supports a "mock" mode for CI environments without native binaries.
 */
namespace Native {
namespace Llama {

using Context = void *;

/** Loads a GGUF model from path */
FORBOCAI_SDK_API Context LoadModel(const FString &Path);

/** Frees the model context */
FORBOCAI_SDK_API void FreeModel(Context Ctx);

/** Performs synchronous inference */
FORBOCAI_SDK_API FString Infer(Context Ctx, const FString &Prompt,
                               int32 MaxTokens = 512);

/** Performs synchronous inference with SDK completion options. */
FORBOCAI_SDK_API FString Infer(Context Ctx, const FString &Prompt,
                               const FCortexConfig &Config);

/** Generates a deterministic embedding vector for text. */
FORBOCAI_SDK_API TArray<float> Embed(Context Ctx, const FString &Text);

} // namespace Llama

namespace Sqlite {

using DB = void *;

/** Opens a vector-enabled sqlite database */
FORBOCAI_SDK_API DB Open(const FString &Path);

/** Closes the database */
FORBOCAI_SDK_API void Close(DB Database);

/** Performs vector similarity search and returns full memory rows. */
FORBOCAI_SDK_API TArray<FMemoryItem>
SearchRows(DB Database, const TArray<float> &Vector, int32 TopK = 5);

/** Inserts or updates a full memory row. */
FORBOCAI_SDK_API bool Upsert(DB Database, const FMemoryItem &Item);

/** Main row-based search entry point used by the SDK thunk path. */
FORBOCAI_SDK_API TArray<FMemoryItem>
Search(DB Database, const TArray<float> &Vector, int32 TopK = 5);

/** Compatibility overload while callers finish collapsing onto full rows. */
FORBOCAI_SDK_API bool Upsert(DB Database, const FMemoryItem &Item,
                             const TArray<float> &Vector);

} // namespace Sqlite
} // namespace Native
