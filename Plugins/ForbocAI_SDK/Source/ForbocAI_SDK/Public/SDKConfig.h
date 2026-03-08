#pragma once

#include "CoreMinimal.h"

/**
 * SDK Configuration
 * Equivalent to `nodeConfig.ts`
 */
namespace SDKConfig {

inline FString GetApiUrl() {
  // Priority: Env Var > Default
  return TEXT("https://api.forboc.ai");
}

inline FString GetApiKey() {
  // Priority: Env Var > Config File
  return TEXT("");
}

inline FString GetSdkVersion() {
  return TEXT("0.6.1"); // Match TS SDK Parity
}

} // namespace SDKConfig
