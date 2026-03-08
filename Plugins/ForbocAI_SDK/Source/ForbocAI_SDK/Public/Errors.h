#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"

namespace Errors {

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

inline func::Maybe<FString>
requireApiKeyGuidance(const FString &ApiUrl, const FString &ApiKey) {
  if (ApiKey.IsEmpty() && ApiUrl.Contains(TEXT("api.forboc.ai"))) {
    return func::just<FString>(
        TEXT("An API key is required when targeting https://api.forboc.ai"));
  }
  return func::nothing<FString>();
}

} // namespace Errors
