#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"

namespace Errors {

// ---------------------------------------------------------------------------
// G9: Consistent error shape — all SDK errors flow through this type
// ---------------------------------------------------------------------------

enum class EErrorCategory : uint8 {
  Network,    // Connection failed, timeout, DNS
  Http,       // Non-2xx response
  Validation, // Missing required field, invalid input
  Protocol,   // Protocol loop errors (max turns, unknown instruction)
  Memory,     // Vector DB / embedding errors
  Cortex,     // SLM inference errors
  Arweave,    // Soul upload/download errors
  Config,     // Configuration errors (missing key, invalid URL)
  Unknown     // Unclassified
};

struct FSDKError {
  EErrorCategory Category;
  FString Code;
  FString Message;
  int32 StatusCode; // HTTP status code (0 for non-HTTP errors)

  FSDKError()
      : Category(EErrorCategory::Unknown), Code(TEXT("UNKNOWN")),
        Message(TEXT("")), StatusCode(0) {}
};

// Factory functions for each error category
inline FSDKError NetworkError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Network;
  E.Code = TEXT("NETWORK_ERROR");
  E.Message = Message;
  return E;
}

inline FSDKError HttpError(int32 StatusCode, const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Http;
  E.Code = FString::Printf(TEXT("HTTP_%d"), StatusCode);
  E.Message = Message;
  E.StatusCode = StatusCode;
  return E;
}

inline FSDKError ValidationError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Validation;
  E.Code = TEXT("VALIDATION_ERROR");
  E.Message = Message;
  return E;
}

inline FSDKError ProtocolError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Protocol;
  E.Code = TEXT("PROTOCOL_ERROR");
  E.Message = Message;
  return E;
}

inline FSDKError MemoryError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Memory;
  E.Code = TEXT("MEMORY_ERROR");
  E.Message = Message;
  return E;
}

inline FSDKError CortexError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Cortex;
  E.Code = TEXT("CORTEX_ERROR");
  E.Message = Message;
  return E;
}

inline FSDKError ArweaveError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Arweave;
  E.Code = TEXT("ARWEAVE_ERROR");
  E.Message = Message;
  return E;
}

inline FSDKError ConfigError(const FString &Message) {
  FSDKError E;
  E.Category = EErrorCategory::Config;
  E.Code = TEXT("CONFIG_ERROR");
  E.Message = Message;
  return E;
}

// ---------------------------------------------------------------------------
// G1: Error normalization — classify raw error strings into FSDKError
// Uses multi_match (§20) for pattern-based classification
// ---------------------------------------------------------------------------

inline FSDKError classifyError(const FString &RawError) {
  std::vector<func::MatchCase<FString, FSDKError>> cases;

  // Network errors
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) { return E.Contains(TEXT("network_error")); },
      [](const FString &E) { return NetworkError(E); }));
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) { return E.Contains(TEXT("request_failed")); },
      [](const FString &E) { return NetworkError(E); }));
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) { return E.Contains(TEXT("timeout")); },
      [](const FString &E) { return NetworkError(E); }));

  // HTTP errors (status code in message)
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) { return E.Contains(TEXT("status_4")); },
      [](const FString &E) { return HttpError(400, E); }));
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) { return E.Contains(TEXT("status_5")); },
      [](const FString &E) { return HttpError(500, E); }));

  // Arweave errors
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) {
        return E.Contains(TEXT("Arweave")) || E.Contains(TEXT("upload")) ||
               E.Contains(TEXT("download"));
      },
      [](const FString &E) { return ArweaveError(E); }));

  // Protocol errors
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) {
        return E.Contains(TEXT("Protocol")) || E.Contains(TEXT("max turns")) ||
               E.Contains(TEXT("instruction type"));
      },
      [](const FString &E) { return ProtocolError(E); }));

  // Memory errors
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) {
        return E.Contains(TEXT("memory")) || E.Contains(TEXT("Memory"));
      },
      [](const FString &E) { return MemoryError(E); }));

  // Cortex errors
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) {
        return E.Contains(TEXT("cortex")) || E.Contains(TEXT("Cortex")) ||
               E.Contains(TEXT("inference"));
      },
      [](const FString &E) { return CortexError(E); }));

  // Config/validation errors
  cases.push_back(func::when<FString, FSDKError>(
      [](const FString &E) {
        return E.Contains(TEXT("API key")) || E.Contains(TEXT("apiKey")) ||
               E.Contains(TEXT("Missing"));
      },
      [](const FString &E) { return ConfigError(E); }));

  // Wildcard default
  cases.push_back(func::when<FString, FSDKError>(
      func::wildcard<FString>(), [](const FString &E) {
        FSDKError Err;
        Err.Category = EErrorCategory::Unknown;
        Err.Code = TEXT("UNKNOWN");
        Err.Message = E;
        return Err;
      }));

  return func::multi_match<FString, FSDKError>(RawError, cases)
      .value; // wildcard guarantees a match
}

// ---------------------------------------------------------------------------
// extractThunkErrorMessage — existing API preserved, enhanced
// ---------------------------------------------------------------------------

inline FString extractThunkErrorMessage(const FString &Message,
                                        const FString &Fallback =
                                            TEXT("Request failed")) {
  return Message.IsEmpty() ? Fallback : Message;
}

inline FString extractThunkErrorMessage(const std::string &Message,
                                        const FString &Fallback =
                                            TEXT("Request failed")) {
  return Message.empty() ? Fallback : FString(UTF8_TO_TCHAR(Message.c_str()));
}

// ---------------------------------------------------------------------------
// requireApiKeyGuidance — returns guidance message if key missing for prod
// ---------------------------------------------------------------------------

inline func::Maybe<FString>
requireApiKeyGuidance(const FString &ApiUrl, const FString &ApiKey) {
  if (ApiKey.IsEmpty() && ApiUrl.Contains(TEXT("api.forboc.ai"))) {
    return func::just<FString>(
        TEXT("Missing API key. Set FORBOCAI_API_KEY (or run `forboc config set "
             "apiKey <key>`) for production API calls."));
  }
  return func::nothing<FString>();
}

} // namespace Errors
