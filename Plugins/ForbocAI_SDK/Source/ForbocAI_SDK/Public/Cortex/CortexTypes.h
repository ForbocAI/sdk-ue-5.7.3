#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "CortexTypes.generated.h"
// clang-format on

/**
 * Cortex Engine Type
 */
UENUM(BlueprintType)
enum class ECortexEngine : uint8 { Mock, Remote, NodeLlamaCpp, WebLlm };

/**
 * Cortex Init Request
 */
USTRUCT(BlueprintType)
struct FCortexInitRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString RequestedModel;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString AuthKey;

  FCortexInitRequest() {}
};

/**
 * Cortex Init Response
 */
USTRUCT(BlueprintType)
struct FCortexInitResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString CortexId;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Runtime;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString State;

  FCortexInitResponse() {}
};

/**
 * Cortex Complete Request
 */
USTRUCT(BlueprintType)
struct FCortexCompleteRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Prompt;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  int32 MaxTokens;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  float Temperature;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  TArray<FString> Stop;

  // JSON-encoded schema payload used for remote completion requests.
  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString JsonSchemaJson;

  FCortexCompleteRequest() : MaxTokens(-1), Temperature(-1.0f) {}
};

/**
 * Cortex Model Info
 */
USTRUCT(BlueprintType)
struct FCortexModelInfo {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Name;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  int32 Parameters;

  FCortexModelInfo() : Parameters(0) {}
};

/**
 * Cortex Status — Engine state metadata.
 */
USTRUCT(BlueprintType)
struct FCortexStatus {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Model;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  bool bReady;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  ECortexEngine Engine;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  float DownloadProgress;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Error;

  FCortexStatus()
      : bReady(false), Engine(ECortexEngine::Mock), DownloadProgress(0.0f) {}
};

/**
 * Cortex Configuration — Immutable data.
 */
USTRUCT(BlueprintType)
struct FCortexConfig {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Model;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  bool UseGPU;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  int32 MaxTokens;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  float Temperature;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  int32 TopK;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  float TopP;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  TArray<FString> Stop;

  // JSON-encoded schema payload used when a caller needs schema-constrained
  // output.
  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString JsonSchemaJson;

  FCortexConfig()
      : Model(TEXT("smollm2-135m")), UseGPU(false), MaxTokens(512),
        Temperature(0.7f), TopK(40), TopP(0.9f) {}
};

/**
 * Cortex Engine Handle — Opaque handle to the inference engine.
 */
struct FCortex {
  FCortexConfig Config;
  void *EngineHandle;
  FString Id;
  bool bReady;

  FCortex() : EngineHandle(nullptr), bReady(false) {}
};

USTRUCT(BlueprintType)
struct FCortexResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Text;

  UPROPERTY(BlueprintReadOnly, Category = "Cortex")
  FString Stats;
};

namespace TypeFactory {

inline FCortexInitRequest CortexInitRequest(FString RequestedModel,
                                            FString AuthKey = TEXT("")) {
  FCortexInitRequest R;
  R.RequestedModel = MoveTemp(RequestedModel);
  R.AuthKey = MoveTemp(AuthKey);
  return R;
}

inline FCortexCompleteRequest CortexCompleteRequest(FString Prompt) {
  FCortexCompleteRequest R;
  R.Prompt = MoveTemp(Prompt);
  return R;
}

inline FCortexCompleteRequest
CortexCompleteRequest(FString Prompt, int32 MaxTokens, float Temperature,
                      const TArray<FString> &Stop = TArray<FString>(),
                      FString JsonSchemaJson = TEXT("")) {
  FCortexCompleteRequest R;
  R.Prompt = MoveTemp(Prompt);
  R.MaxTokens = MaxTokens;
  R.Temperature = Temperature;
  R.Stop = Stop;
  R.JsonSchemaJson = MoveTemp(JsonSchemaJson);
  return R;
}

inline FCortexConfig
CortexConfig(FString Model = TEXT("smollm2-135m"), bool UseGPU = false,
             int32 MaxTokens = 512, float Temperature = 0.7f, int32 TopK = 40,
             float TopP = 0.9f, const TArray<FString> &Stop = TArray<FString>(),
             FString JsonSchemaJson = TEXT("")) {
  FCortexConfig C;
  C.Model = MoveTemp(Model);
  C.UseGPU = UseGPU;
  C.MaxTokens = MaxTokens;
  C.Temperature = Temperature;
  C.TopK = TopK;
  C.TopP = TopP;
  C.Stop = Stop;
  C.JsonSchemaJson = MoveTemp(JsonSchemaJson);
  return C;
}

} // namespace TypeFactory
