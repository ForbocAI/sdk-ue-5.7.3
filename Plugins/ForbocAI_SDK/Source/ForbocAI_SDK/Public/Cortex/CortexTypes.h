#pragma once

#include "CoreMinimal.h"
#include "CortexTypes.generated.h"

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

  UPROPERTY(BlueprintReadOnly)
  FString RequestedModel;

  UPROPERTY(BlueprintReadOnly)
  FString AuthKey;

  FCortexInitRequest() {}
};

/**
 * Cortex Init Response
 */
USTRUCT(BlueprintType)
struct FCortexInitResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString CortexId;

  UPROPERTY(BlueprintReadOnly)
  FString Runtime;

  UPROPERTY(BlueprintReadOnly)
  FString State;

  FCortexInitResponse() {}
};

/**
 * Cortex Complete Request
 */
USTRUCT(BlueprintType)
struct FCortexCompleteRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString CortexId;

  UPROPERTY(BlueprintReadOnly)
  FString Prompt;

  FCortexCompleteRequest() {}
};

/**
 * Cortex Model Info
 */
USTRUCT(BlueprintType)
struct FCortexModelInfo {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Name;

  UPROPERTY(BlueprintReadOnly)
  int32 Parameters;

  FCortexModelInfo() : Parameters(0) {}
};

/**
 * Cortex Status — Engine state metadata.
 */
USTRUCT(BlueprintType)
struct FCortexStatus {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Model;

  UPROPERTY(BlueprintReadOnly)
  bool bReady;

  UPROPERTY(BlueprintReadOnly)
  ECortexEngine Engine;

  UPROPERTY(BlueprintReadOnly)
  float DownloadProgress;

  UPROPERTY(BlueprintReadOnly)
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

  UPROPERTY(BlueprintReadOnly)
  FString Model;

  UPROPERTY(BlueprintReadOnly)
  bool UseGPU;

  UPROPERTY(BlueprintReadOnly)
  int32 MaxTokens;

  UPROPERTY(BlueprintReadOnly)
  float Temperature;

  UPROPERTY(BlueprintReadOnly)
  int32 TopK;

  UPROPERTY(BlueprintReadOnly)
  float TopP;

  FCortexConfig()
      : Model(TEXT("smalll2-135m")), UseGPU(false), MaxTokens(512),
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

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Text;

  UPROPERTY(BlueprintReadOnly)
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

inline FCortexCompleteRequest CortexCompleteRequest(FString CortexId,
                                                    FString Prompt) {
  FCortexCompleteRequest R;
  R.CortexId = MoveTemp(CortexId);
  R.Prompt = MoveTemp(Prompt);
  return R;
}

inline FCortexConfig CortexConfig(FString Model = TEXT("smalll2-135m"),
                                  bool UseGPU = false, int32 MaxTokens = 512,
                                  float Temperature = 0.7f, int32 TopK = 40,
                                  float TopP = 0.9f) {
  FCortexConfig C;
  C.Model = MoveTemp(Model);
  C.UseGPU = UseGPU;
  C.MaxTokens = MaxTokens;
  C.Temperature = Temperature;
  C.TopK = TopK;
  C.TopP = TopP;
  return C;
}

} // namespace TypeFactory
