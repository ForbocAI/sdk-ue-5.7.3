#pragma once

#include "CoreMinimal.h"

/**
 * SDK Configuration
 * Equivalent to `nodeConfig.ts`
 */
namespace SDKConfig {

inline FString &ApiUrlStorage() {
  static FString Value = TEXT("https://api.forboc.ai");
  return Value;
}

inline FString &ApiKeyStorage() {
  static FString Value = TEXT("");
  return Value;
}

inline void SetApiConfig(const FString &ApiUrl, const FString &ApiKey) {
  if (!ApiUrl.IsEmpty()) {
    ApiUrlStorage() = ApiUrl;
  }
  ApiKeyStorage() = ApiKey;
}

inline FString GetApiUrl() {
  return ApiUrlStorage();
}

inline FString GetApiKey() {
  return ApiKeyStorage();
}

inline FString GetSdkVersion() {
  return TEXT("0.6.1"); // Match TS SDK Parity
}

} // namespace SDKConfig
