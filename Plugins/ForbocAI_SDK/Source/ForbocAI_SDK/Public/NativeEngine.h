#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "Cortex/CortexTypes.h"
#include "Memory/MemoryTypes.h"

/**
 * Native Engine Wrappers
 *
 * Isolates 3rd-party library (llama.cpp, sqlite-vec) mechanics from the SDK
 * logic. Requires native binaries — returns errors when unavailable.
 */
namespace Native {
namespace Llama {

using Context = void *;

/** Loads a GGUF model for inference (SmolLM, Llama3, etc.) */
FORBOCAI_SDK_API Context LoadModel(const FString &Path);

/** Loads a GGUF embedding model (e.g. all-MiniLM-L6-v2) for memory. */
FORBOCAI_SDK_API Context LoadEmbeddingModel(const FString &Path);

/** Frees the model context */
FORBOCAI_SDK_API void FreeModel(Context Ctx);

/** Performs synchronous inference */
FORBOCAI_SDK_API FString Infer(Context Ctx, const FString &Prompt,
                               int32 MaxTokens = 512);

/** Performs synchronous inference with SDK completion options. */
FORBOCAI_SDK_API FString Infer(Context Ctx, const FString &Prompt,
                               const FCortexConfig &Config);

/** Per-token callback for streaming inference. */
using TokenCallback = std::function<void(const FString &Token)>;

/** Performs streaming inference, calling OnToken per generated token.
 *  Returns the full accumulated text. Stop tokens are checked after each token. */
FORBOCAI_SDK_API FString InferStream(Context Ctx, const FString &Prompt,
                                     const FCortexConfig &Config,
                                     const TokenCallback &OnToken);

/** Generates an embedding vector for text using the loaded embedding model. */
FORBOCAI_SDK_API TArray<float> Embed(Context Ctx, const FString &Text);

} // namespace Llama

namespace File {

/**
 * Downloads a binary file from a URL to a local path asynchronously.
 * Supports simple redirects if needed. Returns empty on failure.
 */
FORBOCAI_SDK_API func::AsyncResult<FString>
DownloadBinary(const FString &Url, const FString &DestPath);

} // namespace File

namespace Sqlite {

using DB = void *;

/** Opens a vector-enabled sqlite database */
FORBOCAI_SDK_API DB Open(const FString &Path);

/** Closes the database */
FORBOCAI_SDK_API void Close(DB Database);

/** Clears all rows for a database handle. */
FORBOCAI_SDK_API void Clear(DB Database);

/** Clears all rows for a database path. */
FORBOCAI_SDK_API void ClearPath(const FString &Path);

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
