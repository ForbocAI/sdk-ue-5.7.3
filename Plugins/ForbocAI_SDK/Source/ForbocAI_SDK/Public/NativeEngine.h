#pragma once

#include "CoreMinimal.h"

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

} // namespace Llama

namespace Sqlite {

using DB = void *;

/** Opens a vector-enabled sqlite database */
FORBOCAI_SDK_API DB Open(const FString &Path);

/** Closes the database */
FORBOCAI_SDK_API void Close(DB Database);

/** Performs vector similarity search */
FORBOCAI_SDK_API TArray<FString>
Search(DB Database, const TArray<float> &Vector, int32 TopK = 5);

/** Stores a vector embedding */
FORBOCAI_SDK_API void Store(DB Database, const FString &Id,
                            const TArray<float> &Vector,
                            const FString &Metadata);

} // namespace Sqlite
} // namespace Native
