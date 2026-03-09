#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

/**
 * SDK Configuration — env var > config file > defaults precedence.
 * Equivalent to TS nodeConfig.ts.
 */
namespace SDKConfig {

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

inline FString &ApiUrlStorage() {
  static FString Value = TEXT("https://api.forboc.ai");
  return Value;
}

inline FString &ApiKeyStorage() {
  static FString Value = TEXT("");
  return Value;
}

inline FString &ModelPathStorage() {
  static FString Value = TEXT("");
  return Value;
}

inline FString &DatabasePathStorage() {
  static FString Value = TEXT("");
  return Value;
}

inline int32 &VectorDimensionStorage() {
  static int32 Value = 384;
  return Value;
}

inline int32 &MaxRecallResultsStorage() {
  static int32 Value = 10;
  return Value;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

inline FString GetApiUrl() { return ApiUrlStorage(); }
inline FString GetApiKey() { return ApiKeyStorage(); }
inline FString GetModelPath() { return ModelPathStorage(); }
inline FString GetDatabasePath() { return DatabasePathStorage(); }
inline int32 GetVectorDimension() { return VectorDimensionStorage(); }
inline int32 GetMaxRecallResults() { return MaxRecallResultsStorage(); }

inline FString GetSdkVersion() {
  return TEXT("0.6.1");
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

inline void SetApiConfig(const FString &ApiUrl, const FString &ApiKey) {
  if (!ApiUrl.IsEmpty()) {
    ApiUrlStorage() = ApiUrl;
  }
  ApiKeyStorage() = ApiKey;
}

// ---------------------------------------------------------------------------
// Config file path
// ---------------------------------------------------------------------------

inline FString GetConfigFilePath() {
  return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("forbocai_config.json"));
}

// ---------------------------------------------------------------------------
// Load from environment variables
// ---------------------------------------------------------------------------

inline void LoadFromEnvironment() {
  FString EnvApiUrl = FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_API_URL"));
  if (!EnvApiUrl.IsEmpty()) {
    ApiUrlStorage() = EnvApiUrl;
  }

  FString EnvApiKey = FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_API_KEY"));
  if (!EnvApiKey.IsEmpty()) {
    ApiKeyStorage() = EnvApiKey;
  }

  FString EnvModelPath = FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_MODEL_PATH"));
  if (!EnvModelPath.IsEmpty()) {
    ModelPathStorage() = EnvModelPath;
  }

  FString EnvDbPath = FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_DATABASE_PATH"));
  if (!EnvDbPath.IsEmpty()) {
    DatabasePathStorage() = EnvDbPath;
  }

  FString EnvVecDim = FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_VECTOR_DIMENSION"));
  if (!EnvVecDim.IsEmpty()) {
    VectorDimensionStorage() = FCString::Atoi(*EnvVecDim);
  }

  FString EnvMaxRecall = FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_MAX_RECALL"));
  if (!EnvMaxRecall.IsEmpty()) {
    MaxRecallResultsStorage() = FCString::Atoi(*EnvMaxRecall);
  }
}

// ---------------------------------------------------------------------------
// Load from config file (JSON)
// ---------------------------------------------------------------------------

inline void LoadFromConfigFile() {
  FString ConfigPath = GetConfigFilePath();
  FString JsonString;
  if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath)) {
    return;
  }

  TSharedPtr<FJsonObject> JsonObject;
  TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
  if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid()) {
    return;
  }

  FString Val;
  if (JsonObject->TryGetStringField(TEXT("apiUrl"), Val) && !Val.IsEmpty()) {
    ApiUrlStorage() = Val;
  }
  if (JsonObject->TryGetStringField(TEXT("apiKey"), Val) && !Val.IsEmpty()) {
    ApiKeyStorage() = Val;
  }
  if (JsonObject->TryGetStringField(TEXT("modelPath"), Val) && !Val.IsEmpty()) {
    ModelPathStorage() = Val;
  }
  if (JsonObject->TryGetStringField(TEXT("databasePath"), Val) && !Val.IsEmpty()) {
    DatabasePathStorage() = Val;
  }

  int32 IntVal;
  if (JsonObject->TryGetNumberField(TEXT("vectorDimension"), IntVal)) {
    VectorDimensionStorage() = IntVal;
  }
  if (JsonObject->TryGetNumberField(TEXT("maxRecallResults"), IntVal)) {
    MaxRecallResultsStorage() = IntVal;
  }
}

// ---------------------------------------------------------------------------
// Initialize — call once at startup.
// Precedence: defaults (hardcoded) < config file < env vars
// ---------------------------------------------------------------------------

inline void InitializeConfig() {
  // 1. Defaults are already set in storage statics
  // 2. Config file overrides defaults
  LoadFromConfigFile();
  // 3. Env vars override config file
  LoadFromEnvironment();
}

} // namespace SDKConfig
