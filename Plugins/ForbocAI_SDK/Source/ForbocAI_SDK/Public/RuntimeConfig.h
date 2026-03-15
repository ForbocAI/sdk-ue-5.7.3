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
 * User Story: As an SDK integrator, I need this type or module note so I can understand the role of the surrounding API surface quickly.
 */
namespace SDKConfig {

/**
 * Default: localhost for dev. Override via FORBOCAI_API_URL env var or config file.
 * User Story: As a maintainer, I need this note so the surrounding API intent stays clear during maintenance and integration.
 */
inline constexpr TCHAR DEFAULT_API_URL[] = TEXT("http://localhost:8080");
inline constexpr TCHAR PRODUCTION_API_URL[] = TEXT("https://api.forboc.ai");
inline constexpr int32 DEFAULT_VECTOR_DIMENSION = 384;
inline constexpr int32 DEFAULT_MAX_RECALL_RESULTS = 10;

/**
 * Returns the mutable storage backing the configured API URL.
 * User Story: As config loading and override helpers, I need shared URL
 * storage so disk, environment, and runtime writes update one source of truth.
 */
inline FString &ApiUrlStorage() {
  static FString Value = DEFAULT_API_URL;
  return Value;
}

/**
 * Returns the mutable storage backing the configured API key.
 * User Story: As config loading and override helpers, I need shared API key
 * storage so every config source reads and writes the same secret value.
 */
inline FString &ApiKeyStorage() {
  static FString Value = TEXT("");
  return Value;
}

/**
 * Returns the mutable storage backing the configured model path.
 * User Story: As local model configuration, I need shared path storage so
 * runtime code and tooling resolve the same model location.
 */
inline FString &ModelPathStorage() {
  static FString Value = TEXT("");
  return Value;
}

/**
 * Returns the mutable storage backing the configured database path.
 * User Story: As memory persistence configuration, I need shared database path
 * storage so local memory features point at the same file.
 */
inline FString &DatabasePathStorage() {
  static FString Value = TEXT("");
  return Value;
}

/**
 * Returns the mutable storage backing the configured embedding width.
 * User Story: As embedding configuration, I need shared vector dimension
 * storage so recall and indexing use the same width.
 */
inline int32 &VectorDimensionStorage() {
  static int32 Value = DEFAULT_VECTOR_DIMENSION;
  return Value;
}

/**
 * Returns the mutable storage backing the configured recall limit.
 * User Story: As recall configuration, I need shared limit storage so default
 * recall behavior stays consistent across runtime entry points.
 */
inline int32 &MaxRecallResultsStorage() {
  static int32 Value = DEFAULT_MAX_RECALL_RESULTS;
  return Value;
}

/**
 * Returns the initialization flag used to guard lazy config loading.
 * User Story: As lazy config access, I need a shared initialized flag so
 * getters can avoid reloading config on every call.
 */
inline bool &InitializedStorage() {
  static bool bInitialized = false;
  return bInitialized;
}

/**
 * Returns the optional config-path override used by tests and tooling.
 * User Story: As tests and CLI tooling, I need an override slot so config I/O
 * can target a temporary file instead of the user default path.
 */
inline FString &ConfigFilePathOverrideStorage() {
  static FString Value;
  return Value;
}

/**
 * Resets all in-memory config values back to their built-in defaults.
 * User Story: As config reload flows, I need a default reset step so stale
 * values do not survive before disk and environment overrides are reapplied.
 */
inline void ResetToDefaults() {
  ApiUrlStorage() = DEFAULT_API_URL;
  ApiKeyStorage() = TEXT("");
  ModelPathStorage() = TEXT("");
  DatabasePathStorage() = TEXT("");
  VectorDimensionStorage() = DEFAULT_VECTOR_DIMENSION;
  MaxRecallResultsStorage() = DEFAULT_MAX_RECALL_RESULTS;
}

/**
 * Loads config from defaults, disk, and environment in precedence order.
 * User Story: As runtime startup, I need one initialization entry point so all
 * getters observe the same resolved configuration values.
 */
inline void InitializeConfig();

/**
 * Ensures config has been loaded before a getter reads from storage.
 * User Story: As config consumers, I need getters to self-initialize so reads
 * work even if startup code did not initialize config explicitly.
 */
inline void EnsureInitialized() {
  if (!InitializedStorage()) {
    InitializeConfig();
  }
}

/**
 * Forces config to reload from disk and environment.
 * User Story: As config mutation flows, I need a reload helper so persisted or
 * environment changes can be reflected without restarting the runtime.
 */
inline void ReloadConfig() {
  InitializedStorage() = false;
  InitializeConfig();
}

/**
 * Returns the active API base URL.
 * User Story: As networked SDK features, I need the resolved API URL so all
 * requests target the current environment.
 */
inline FString GetApiUrl() {
  EnsureInitialized();
  return ApiUrlStorage();
}

/**
 * Returns the active API key.
 * User Story: As authenticated SDK requests, I need the resolved API key so
 * outbound calls can include the current credential.
 */
inline FString GetApiKey() {
  EnsureInitialized();
  return ApiKeyStorage();
}

/**
 * Returns the configured local model path.
 * User Story: As local inference setup, I need the resolved model path so the
 * runtime can load the configured on-device model.
 */
inline FString GetModelPath() {
  EnsureInitialized();
  return ModelPathStorage();
}

/**
 * Returns the configured local database path.
 * User Story: As local memory storage, I need the resolved database path so
 * persistence code opens the intended datastore.
 */
inline FString GetDatabasePath() {
  EnsureInitialized();
  return DatabasePathStorage();
}

/**
 * Returns the configured vector dimension for local memory embeddings.
 * User Story: As embedding-backed recall flows, I need the resolved dimension
 * so vector operations use the correct width.
 */
inline int32 GetVectorDimension() {
  EnsureInitialized();
  return VectorDimensionStorage();
}

/**
 * Returns the configured default recall limit.
 * User Story: As memory recall flows, I need the resolved default limit so
 * callers can cap results consistently when no explicit limit is provided.
 */
inline int32 GetMaxRecallResults() {
  EnsureInitialized();
  return MaxRecallResultsStorage();
}

/**
 * Returns the SDK version string baked into the plugin build.
 * User Story: As diagnostics and tooling, I need the runtime SDK version so I
 * can report which plugin build is running.
 */
inline FString GetSdkVersion() { return TEXT("0.6.3"); }

/**
 * Updates the in-memory API URL and API key overrides.
 * User Story: As runtime override flows, I need to swap API connection values
 * in memory so subsequent requests use the new endpoint and credential.
 */
inline void SetApiConfig(const FString &ApiUrl, const FString &ApiKey) {
  EnsureInitialized();
  if (!ApiUrl.IsEmpty()) {
    ApiUrlStorage() = ApiUrl;
  }
  ApiKeyStorage() = ApiKey;
}

/**
 * Overrides the config file path used for subsequent disk reads and writes.
 * User Story: As tests and tooling, I need to redirect config I/O so I can
 * isolate runs from the user's real config file.
 */
inline void SetConfigFilePathOverride(const FString &ConfigFilePath) {
  ConfigFilePathOverrideStorage() = ConfigFilePath;
}

/**
 * Resolves the config file path, honoring any explicit override first.
 * User Story: As config file helpers, I need the effective path so reads and
 * writes target the correct config location.
 */
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

/**
 * Loads the raw config JSON object from disk, or an empty object when absent.
 * User Story: As config parsing helpers, I need raw JSON config data so I can
 * populate in-memory settings from persisted values.
 */
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

/**
 * Persists a raw config JSON object to disk with restricted file permissions.
 * User Story: As config persistence flows, I need a safe write helper so
 * resolved settings can be saved with private file permissions when possible.
 */
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

/**
 * Applies supported FORBOCAI_* environment variables to in-memory config.
 * User Story: As deployment configuration, I need environment overrides to win
 * so runtime settings can be injected without editing files.
 */
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

/**
 * Applies persisted JSON config values to in-memory config storage.
 * User Story: As local config loading, I need persisted settings applied so
 * runtime defaults can be overridden by the saved config file.
 */
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

/**
 * Saves the current in-memory configuration values back to disk.
 * User Story: As config editing flows, I need current settings persisted so
 * future runs start with the same resolved configuration.
 */
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

/**
 * Writes a single supported config value to disk and reloads in-memory state.
 * User Story: As config editing tools, I need to update one key at a time so
 * CLI and editor flows can persist targeted changes simply.
 */
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

/**
 * Reads a single supported config value from disk or derived version state.
 * User Story: As config inspection tools, I need a single-key lookup helper so
 * CLI commands can print one value without loading call-site parsing logic.
 */
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

/**
 * Initializes config once using defaults, persisted JSON, then env overrides.
 * User Story: As runtime startup, I need deterministic config precedence so
 * defaults, file values, and environment overrides resolve predictably.
 */
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
