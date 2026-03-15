#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#if PLATFORM_MAC || PLATFORM_UNIX
#include <sys/stat.h>
#endif

/**
 * SDK Configuration — env var > config file > defaults precedence.
 * Equivalent to TS nodeConfig.ts plus CLI config persistence in sdkOps.ts.
 */
namespace SDKConfig {

/** Default: localhost for dev. Override via FORBOCAI_API_URL env var or config file. */
inline constexpr TCHAR DEFAULT_API_URL[] = TEXT("http://localhost:8080");
inline constexpr TCHAR PRODUCTION_API_URL[] = TEXT("https://api.forboc.ai");
inline constexpr int32 DEFAULT_VECTOR_DIMENSION = 384;
inline constexpr int32 DEFAULT_MAX_RECALL_RESULTS = 10;

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

inline FString &ApiUrlStorage() {
  static FString Value = DEFAULT_API_URL;
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
  static int32 Value = DEFAULT_VECTOR_DIMENSION;
  return Value;
}

inline int32 &MaxRecallResultsStorage() {
  static int32 Value = DEFAULT_MAX_RECALL_RESULTS;
  return Value;
}

inline bool &InitializedStorage() {
  static bool bInitialized = false;
  return bInitialized;
}

inline FString &ConfigFilePathOverrideStorage() {
  static FString Value;
  return Value;
}

inline void ResetToDefaults() {
  ApiUrlStorage() = DEFAULT_API_URL;
  ApiKeyStorage() = TEXT("");
  ModelPathStorage() = TEXT("");
  DatabasePathStorage() = TEXT("");
  VectorDimensionStorage() = DEFAULT_VECTOR_DIMENSION;
  MaxRecallResultsStorage() = DEFAULT_MAX_RECALL_RESULTS;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

inline void InitializeConfig();

inline void EnsureInitialized() {
  if (!InitializedStorage()) {
    InitializeConfig();
  }
}

inline void ReloadConfig() {
  InitializedStorage() = false;
  InitializeConfig();
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

inline FString GetApiUrl() {
  EnsureInitialized();
  return ApiUrlStorage();
}

inline FString GetApiKey() {
  EnsureInitialized();
  return ApiKeyStorage();
}

inline FString GetModelPath() {
  EnsureInitialized();
  return ModelPathStorage();
}

inline FString GetDatabasePath() {
  EnsureInitialized();
  return DatabasePathStorage();
}

inline int32 GetVectorDimension() {
  EnsureInitialized();
  return VectorDimensionStorage();
}

inline int32 GetMaxRecallResults() {
  EnsureInitialized();
  return MaxRecallResultsStorage();
}

inline FString GetSdkVersion() { return TEXT("0.6.3"); }

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

inline void SetApiConfig(const FString &ApiUrl, const FString &ApiKey) {
  EnsureInitialized();
  if (!ApiUrl.IsEmpty()) {
    ApiUrlStorage() = ApiUrl;
  }
  ApiKeyStorage() = ApiKey;
}

inline void SetConfigFilePathOverride(const FString &ConfigFilePath) {
  ConfigFilePathOverrideStorage() = ConfigFilePath;
}

// ---------------------------------------------------------------------------
// Config file helpers
// ---------------------------------------------------------------------------

inline FString GetConfigFilePath() {
  if (!ConfigFilePathOverrideStorage().IsEmpty()) {
    return ConfigFilePathOverrideStorage();
  }

  FString HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
  if (HomeDir.IsEmpty()) {
    HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE"));
  }

  return FPaths::Combine(HomeDir.IsEmpty() ? FPaths::ProjectDir() : HomeDir,
                         TEXT(".forbocai.json"));
}

inline TSharedPtr<FJsonObject> LoadConfigJsonObject() {
  const FString ConfigPath = GetConfigFilePath();
  FString JsonString;
  if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath)) {
    return MakeShared<FJsonObject>();
  }

  TSharedPtr<FJsonObject> JsonObject;
  const TSharedRef<TJsonReader<>> Reader =
      TJsonReaderFactory<>::Create(JsonString);
  if (!FJsonSerializer::Deserialize(Reader, JsonObject) ||
      !JsonObject.IsValid()) {
    return MakeShared<FJsonObject>();
  }

  return JsonObject;
}

inline bool WriteConfigJsonObject(const TSharedRef<FJsonObject> &JsonObject) {
  const FString ConfigPath = GetConfigFilePath();
  const FString Directory = FPaths::GetPath(ConfigPath);
  if (!Directory.IsEmpty()) {
    IFileManager::Get().MakeDirectory(*Directory, true);
  }

  FString JsonString;
  const TSharedRef<TJsonWriter<>> Writer =
      TJsonWriterFactory<>::Create(&JsonString);
  FJsonSerializer::Serialize(JsonObject, Writer);

  const bool bSaved = FFileHelper::SaveStringToFile(JsonString, *ConfigPath);
#if PLATFORM_MAC || PLATFORM_UNIX
  if (bSaved) {
    chmod(TCHAR_TO_UTF8(*ConfigPath), 0600);
  }
#endif
  return bSaved;
}

// ---------------------------------------------------------------------------
// Load from environment variables
// ---------------------------------------------------------------------------

inline void LoadFromEnvironment() {
  const FString EnvApiUrl =
      FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_API_URL"));
  if (!EnvApiUrl.IsEmpty()) {
    ApiUrlStorage() = EnvApiUrl;
  }

  const FString EnvApiKey =
      FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_API_KEY"));
  if (!EnvApiKey.IsEmpty()) {
    ApiKeyStorage() = EnvApiKey;
  }

  const FString EnvModelPath =
      FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_MODEL_PATH"));
  if (!EnvModelPath.IsEmpty()) {
    ModelPathStorage() = EnvModelPath;
  }

  const FString EnvDbPath =
      FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_DATABASE_PATH"));
  if (!EnvDbPath.IsEmpty()) {
    DatabasePathStorage() = EnvDbPath;
  }

  const FString EnvVecDim =
      FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_VECTOR_DIMENSION"));
  if (!EnvVecDim.IsEmpty()) {
    VectorDimensionStorage() = FCString::Atoi(*EnvVecDim);
  }

  const FString EnvMaxRecall =
      FPlatformMisc::GetEnvironmentVariable(TEXT("FORBOCAI_MAX_RECALL"));
  if (!EnvMaxRecall.IsEmpty()) {
    MaxRecallResultsStorage() = FCString::Atoi(*EnvMaxRecall);
  }
}

// ---------------------------------------------------------------------------
// Load from config file (JSON)
// ---------------------------------------------------------------------------

inline void LoadFromConfigFile() {
  const TSharedPtr<FJsonObject> JsonObject = LoadConfigJsonObject();
  if (!JsonObject.IsValid()) {
    return;
  }

  FString Value;
  if (JsonObject->TryGetStringField(TEXT("apiUrl"), Value) && !Value.IsEmpty()) {
    ApiUrlStorage() = Value;
  }
  if (JsonObject->TryGetStringField(TEXT("apiKey"), Value) && !Value.IsEmpty()) {
    ApiKeyStorage() = Value;
  }
  if (JsonObject->TryGetStringField(TEXT("modelPath"), Value) &&
      !Value.IsEmpty()) {
    ModelPathStorage() = Value;
  }
  if (JsonObject->TryGetStringField(TEXT("databasePath"), Value) &&
      !Value.IsEmpty()) {
    DatabasePathStorage() = Value;
  }

  int32 IntValue = 0;
  if (JsonObject->TryGetNumberField(TEXT("vectorDimension"), IntValue)) {
    VectorDimensionStorage() = IntValue;
  }
  if (JsonObject->TryGetNumberField(TEXT("maxRecallResults"), IntValue)) {
    MaxRecallResultsStorage() = IntValue;
  }
}

inline bool SaveToConfigFile() {
  EnsureInitialized();

  const TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
  JsonObject->SetStringField(TEXT("apiUrl"), ApiUrlStorage());
  if (!ApiKeyStorage().IsEmpty()) {
    JsonObject->SetStringField(TEXT("apiKey"), ApiKeyStorage());
  }
  if (!ModelPathStorage().IsEmpty()) {
    JsonObject->SetStringField(TEXT("modelPath"), ModelPathStorage());
  }
  if (!DatabasePathStorage().IsEmpty()) {
    JsonObject->SetStringField(TEXT("databasePath"), DatabasePathStorage());
  }
  JsonObject->SetNumberField(TEXT("vectorDimension"), VectorDimensionStorage());
  JsonObject->SetNumberField(TEXT("maxRecallResults"),
                             MaxRecallResultsStorage());

  return WriteConfigJsonObject(JsonObject);
}

inline void SetConfigValue(const FString &Key, const FString &Value) {
  TSharedPtr<FJsonObject> JsonObject = LoadConfigJsonObject();
  if (!JsonObject.IsValid()) {
    JsonObject = MakeShared<FJsonObject>();
  }

  if (Key == TEXT("apiUrl")) {
    JsonObject->SetStringField(TEXT("apiUrl"), Value);
  } else if (Key == TEXT("apiKey")) {
    JsonObject->SetStringField(TEXT("apiKey"), Value);
  } else if (Key == TEXT("modelPath")) {
    JsonObject->SetStringField(TEXT("modelPath"), Value);
  } else if (Key == TEXT("databasePath")) {
    JsonObject->SetStringField(TEXT("databasePath"), Value);
  } else if (Key == TEXT("vectorDimension")) {
    JsonObject->SetNumberField(TEXT("vectorDimension"), FCString::Atoi(*Value));
  } else if (Key == TEXT("maxRecallResults")) {
    JsonObject->SetNumberField(TEXT("maxRecallResults"),
                               FCString::Atoi(*Value));
  } else {
    return;
  }

  if (WriteConfigJsonObject(JsonObject.ToSharedRef())) {
    ReloadConfig();
  }
}

inline FString GetConfigValue(const FString &Key) {
  if (Key == TEXT("version")) {
    return GetSdkVersion();
  }

  const TSharedPtr<FJsonObject> JsonObject = LoadConfigJsonObject();
  if (!JsonObject.IsValid()) {
    return TEXT("");
  }

  FString Value;
  if (Key == TEXT("apiUrl")) {
    return JsonObject->TryGetStringField(TEXT("apiUrl"), Value) ? Value
                                                                : TEXT("");
  }
  if (Key == TEXT("apiKey")) {
    return JsonObject->TryGetStringField(TEXT("apiKey"), Value) ? Value
                                                                : TEXT("");
  }
  if (Key == TEXT("modelPath")) {
    return JsonObject->TryGetStringField(TEXT("modelPath"), Value) ? Value
                                                                   : TEXT("");
  }
  if (Key == TEXT("databasePath")) {
    return JsonObject->TryGetStringField(TEXT("databasePath"), Value) ? Value
                                                                      : TEXT("");
  }

  int32 IntValue = 0;
  if (Key == TEXT("vectorDimension")) {
    return JsonObject->TryGetNumberField(TEXT("vectorDimension"), IntValue)
               ? FString::FromInt(IntValue)
               : TEXT("");
  }
  if (Key == TEXT("maxRecallResults")) {
    return JsonObject->TryGetNumberField(TEXT("maxRecallResults"), IntValue)
               ? FString::FromInt(IntValue)
               : TEXT("");
  }

  return TEXT("");
}

// ---------------------------------------------------------------------------
// Initialize — call once at startup.
// Precedence: defaults < config file < env vars
// ---------------------------------------------------------------------------

inline void InitializeConfig() {
  if (InitializedStorage()) {
    return;
  }

  ResetToDefaults();
  LoadFromConfigFile();
  LoadFromEnvironment();
  InitializedStorage() = true;
}

} // namespace SDKConfig
