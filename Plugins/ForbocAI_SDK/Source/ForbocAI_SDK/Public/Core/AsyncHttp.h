#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "rtk.hpp"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "functional_core.hpp"
#include <type_traits>

namespace func {
namespace AsyncHttp {
namespace detail {

template <typename JsonValueT>
bool DeserializeStringArrayRecursive(
    const TArray<TSharedPtr<JsonValueT>> &JsonValues, int32 Index,
    TArray<FString> &OutValue) {
  return Index == JsonValues.Num()
             ? true
             : !JsonValues[Index].IsValid()
                   ? false
                   : (OutValue.Add(JsonValues[Index]->AsString()),
                      DeserializeStringArrayRecursive(JsonValues, Index + 1,
                                                     OutValue));
}

template <typename T, typename JsonValueT>
bool DeserializeStructArrayRecursive(
    const TArray<TSharedPtr<JsonValueT>> &JsonValues, int32 Index,
    TArray<T> &OutValue) {
  return Index == JsonValues.Num()
             ? true
             : !JsonValues[Index].IsValid()
                   ? false
                   : !JsonValues[Index]->AsObject().IsValid()
                         ? false
                         : []() { return true; }(),
               [&]() -> bool {
                 T Item;
                 const TSharedPtr<FJsonObject> JsonObject =
                     JsonValues[Index]->AsObject();
                 return !FJsonObjectConverter::JsonObjectToUStruct(
                            JsonObject.ToSharedRef(), T::StaticStruct(), &Item,
                            0, 0)
                            ? false
                            : (OutValue.Add(Item),
                               DeserializeStructArrayRecursive<T>(
                                   JsonValues, Index + 1, OutValue));
               }();
}

template <typename CallbackT>
void NotifyCallbacksRecursive(const std::vector<CallbackT> &Callbacks,
                              size_t Index, const FString &Body,
                              HttpStatusCode Code) {
  Index == Callbacks.size()
      ? void()
      : (Callbacks[Index](Body, Code),
         NotifyCallbacksRecursive(Callbacks, Index + 1, Body, Code));
}

/**
 * Deserializes an HTTP payload into a UStruct-backed value.
 * User Story: As generic HTTP decoding, I need a default deserializer so API
 * responses can hydrate SDK structs without endpoint-specific code.
 */
template <typename T, typename Enable = void> struct JsonDeserializer {
  static bool Deserialize(const FString &Content, T &OutValue) {
    return FJsonObjectConverter::JsonObjectStringToUStruct(Content, &OutValue, 0,
                                                           0);
  }
};

/**
 * Deserializes an HTTP payload into a plain string.
 * User Story: As raw-response consumers, I need string passthrough decoding so
 * endpoints can return text bodies without JSON struct conversion.
 */
template <> struct JsonDeserializer<FString, void> {
  static bool Deserialize(const FString &Content, FString &OutValue) {
    OutValue = Content;
    return true;
  }
};

/**
 * Deserializes an empty HTTP payload.
 * User Story: As endpoints with no response body, I need empty-payload support
 * so success responses can still resolve cleanly.
 */
template <> struct JsonDeserializer<rtk::FEmptyPayload, void> {
  static bool Deserialize(const FString &Content,
                          rtk::FEmptyPayload &OutValue) {
    return true;
  }
};

/**
 * Deserializes an HTTP payload into an array of strings.
 * User Story: As list endpoints returning raw strings, I need array decoding so
 * preset ids and similar payloads can be consumed directly.
 */
template <> struct JsonDeserializer<TArray<FString>, void> {
  static bool Deserialize(const FString &Content, TArray<FString> &OutValue) {
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    TArray<TSharedPtr<FJsonValue>> JsonValues;
    if (!FJsonSerializer::Deserialize(Reader, JsonValues)) {
      return false;
    }

    OutValue.Empty(JsonValues.Num());
    return DeserializeStringArrayRecursive(JsonValues, 0, OutValue);
  }
};

/**
 * Deserializes an HTTP payload into an array of UStruct-backed values.
 * User Story: As collection endpoints, I need array decoding so JSON lists can
 * hydrate SDK structs without custom per-type parsers.
 */
template <typename T> struct JsonDeserializer<TArray<T>, void> {
  static bool Deserialize(const FString &Content, TArray<T> &OutValue) {
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    TArray<TSharedPtr<FJsonValue>> JsonValues;
    if (!FJsonSerializer::Deserialize(Reader, JsonValues)) {
      return false;
    }

    OutValue.Empty(JsonValues.Num());
    return DeserializeStructArrayRecursive<T>(JsonValues, 0, OutValue);
  }
};

/**
 * Builds the generic HTTP request wrapper for an async call.
 * User Story: As endpoint helpers, I need one request builder so GET, POST,
 * and DELETE calls share the same network, auth, and decode path.
 */
template <typename T>
func::AsyncResult<func::HttpResult<T>>
CreateRequest(const FString &Verb, const FString &Url,
              const FString &ApiKey = TEXT(""),
              const FString &Payload = TEXT("")) {
  return func::AsyncResult<func::HttpResult<T>>::create(
      [Verb, Url, ApiKey, Payload](std::function<void(func::HttpResult<T>)> Resolve,
                                   std::function<void(std::string)> Reject) {
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
            FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb(Verb);
        Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

        if (Verb == TEXT("POST") || Verb == TEXT("PUT") || Verb == TEXT("PATCH")) {
          Request->SetHeader(TEXT("Content-Type"),
                             TEXT("application/json; charset=utf-8"));
          Request->SetContentAsString(Payload);
        }

        if (!ApiKey.IsEmpty()) {
          Request->SetHeader(TEXT("Authorization"),
                             FString::Printf(TEXT("Bearer %s"), *ApiKey));
        }

        Request->OnProcessRequestComplete().BindLambda(
            [Resolve](FHttpRequestPtr Req, FHttpResponsePtr Res,
                      bool bWasSuccessful) {
              if (!bWasSuccessful || !Res.IsValid()) {
                Resolve(func::HttpResult<T>::Failure("Network failure", 0));
                return;
              }

              const HttpStatusCode Code = Res->GetResponseCode();
              const FString Content = Res->GetContentAsString();

              if (Code < 200 || Code >= 300) {
                Resolve(func::HttpResult<T>::Failure(TCHAR_TO_UTF8(*Content),
                                                     Code));
                return;
              }

              T ResultPayload;
              if (Content.IsEmpty()) {
                Resolve(func::HttpResult<T>::Success(ResultPayload, Code));
                return;
              }

              if (JsonDeserializer<T>::Deserialize(Content, ResultPayload)) {
                Resolve(func::HttpResult<T>::Success(ResultPayload, Code));
              } else {
                Resolve(func::HttpResult<T>::Failure("JSON deserialization failed",
                                                     Code));
              }
            });

        Request->ProcessRequest();
      });
}

} // namespace detail

/**
 * Builds an async POST request for a typed response.
 * User Story: As endpoint wrappers, I need a POST helper so write operations
 * can share the common request and decode path.
 */
template <typename T>
func::AsyncResult<func::HttpResult<T>>
Post(const FString &Url, const FString &Payload,
     const FString &ApiKey = TEXT("")) {
  return detail::CreateRequest<T>(TEXT("POST"), Url, ApiKey, Payload);
}

/**
 * Builds an async GET request for a typed response.
 * User Story: As endpoint wrappers, I need a GET helper so read operations can
 * share the common request and decode path.
 */
template <typename T>
func::AsyncResult<func::HttpResult<T>> Get(const FString &Url,
                                           const FString &ApiKey = TEXT("")) {
  return detail::CreateRequest<T>(TEXT("GET"), Url, ApiKey);
}

/**
 * Builds an async DELETE request for a typed response.
 * User Story: As endpoint wrappers, I need a DELETE helper so destructive
 * operations can share the common request and decode path.
 */
template <typename T>
func::AsyncResult<func::HttpResult<T>>
Delete(const FString &Url, const FString &ApiKey = TEXT("")) {
  return detail::CreateRequest<T>(TEXT("DELETE"), Url, ApiKey);
}

/**
 * G10: Request deduplication — prevents duplicate in-flight GET requests.
 * If a GET for the same URL is already in-flight, callers receive the
 * same result instead of firing a second HTTP request.
 * User Story: As a maintainer, I need this implementation note so I can understand which milestone behavior the surrounding code is preserving.
 */

namespace Dedup {

namespace detail {

struct InFlightEntry {
  std::vector<std::function<void(const FString &, HttpStatusCode)>> Callbacks;
  bool bCompleted;
  FString ResponseBody;
  HttpStatusCode StatusCode;

  InFlightEntry() : bCompleted(false), StatusCode(0) {}
};

inline TMap<FString, TSharedPtr<InFlightEntry>> &InFlightRequests() {
  static TMap<FString, TSharedPtr<InFlightEntry>> Map;
  return Map;
}

} // namespace detail

/**
 * Builds a deduplicated async GET request for a typed response.
 * User Story: As request fan-out control, I need GET deduplication so repeated
 * callers can share one in-flight network request instead of spamming the API.
 */
template <typename T>
func::AsyncResult<func::HttpResult<T>>
GetDeduped(const FString &Url, const FString &ApiKey = TEXT("")) {
  const FString Key = Url;

  auto &Map = detail::InFlightRequests();
  if (Map.Contains(Key)) {
    TSharedPtr<detail::InFlightEntry> Existing = Map[Key];
    if (!Existing->bCompleted) {
      /**
       * Already in-flight — piggyback on existing request
       * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
       */
      return func::AsyncResult<func::HttpResult<T>>::create(
          [Existing](std::function<void(func::HttpResult<T>)> Resolve,
                     std::function<void(std::string)>) {
            Existing->Callbacks.push_back(
                [Resolve](const FString &Body, HttpStatusCode Code) {
                  if (Code >= 200 && Code < 300) {
                    T Payload;
                    if (AsyncHttp::detail::JsonDeserializer<T>::Deserialize(
                            Body, Payload)) {
                      Resolve(func::HttpResult<T>::Success(Payload, Code));
                    } else {
                      Resolve(func::HttpResult<T>::Failure(
                          "JSON deserialization failed", Code));
                    }
                  } else {
                    Resolve(func::HttpResult<T>::Failure(
                        TCHAR_TO_UTF8(*Body), Code));
                  }
                });
          });
    }
  }

  /**
   * First request for this URL — create entry and fire
   * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
   */
  TSharedPtr<detail::InFlightEntry> Entry =
      MakeShared<detail::InFlightEntry>();
  Map.Add(Key, Entry);

  return func::AsyncChain::then<func::HttpResult<T>, func::HttpResult<T>>(
      AsyncHttp::Get<T>(Url, ApiKey),
      [Key, Entry](const func::HttpResult<T> &Result) {
        auto &Map = detail::InFlightRequests();
        Entry->bCompleted = true;

        /**
         * Notify piggybacked callers
         * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
         */
        const FString Body =
            Result.bSuccess ? TEXT("") : FString(UTF8_TO_TCHAR(Result.Error.c_str()));
        detail::NotifyCallbacksRecursive(Entry->Callbacks, 0, Body,
                                         Result.StatusCode);

        Map.Remove(Key);
        return func::AsyncResult<func::HttpResult<T>>::create(
            [Result](std::function<void(func::HttpResult<T>)> Resolve,
                     std::function<void(std::string)>) { Resolve(Result); });
      });
}

} // namespace Dedup

} // namespace AsyncHttp
} // namespace func
