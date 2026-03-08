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

template <typename T, typename Enable = void> struct JsonDeserializer {
  static bool Deserialize(const FString &Content, T &OutValue) {
    return FJsonObjectConverter::JsonObjectStringToUStruct(Content, &OutValue, 0,
                                                           0);
  }
};

template <> struct JsonDeserializer<FString, void> {
  static bool Deserialize(const FString &Content, FString &OutValue) {
    OutValue = Content;
    return true;
  }
};

template <> struct JsonDeserializer<rtk::FEmptyPayload, void> {
  static bool Deserialize(const FString &Content,
                          rtk::FEmptyPayload &OutValue) {
    return true;
  }
};

template <> struct JsonDeserializer<TArray<FString>, void> {
  static bool Deserialize(const FString &Content, TArray<FString> &OutValue) {
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    TArray<TSharedPtr<FJsonValue>> JsonValues;
    if (!FJsonSerializer::Deserialize(Reader, JsonValues)) {
      return false;
    }

    OutValue.Empty(JsonValues.Num());
    for (int32 Index = 0; Index < JsonValues.Num(); ++Index) {
      const TSharedPtr<FJsonValue> &JsonValue = JsonValues[Index];
      if (!JsonValue.IsValid()) {
        return false;
      }
      OutValue.Add(JsonValue->AsString());
    }

    return true;
  }
};

template <typename T> struct JsonDeserializer<TArray<T>, void> {
  static bool Deserialize(const FString &Content, TArray<T> &OutValue) {
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    TArray<TSharedPtr<FJsonValue>> JsonValues;
    if (!FJsonSerializer::Deserialize(Reader, JsonValues)) {
      return false;
    }

    OutValue.Empty(JsonValues.Num());
    for (int32 Index = 0; Index < JsonValues.Num(); ++Index) {
      const TSharedPtr<FJsonValue> &JsonValue = JsonValues[Index];
      if (!JsonValue.IsValid()) {
        return false;
      }

      T Item;
      const TSharedPtr<FJsonObject> JsonObject = JsonValue->AsObject();
      if (!JsonObject.IsValid() ||
          !FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(),
                                                     T::StaticStruct(), &Item,
                                                     0, 0)) {
        return false;
      }
      OutValue.Add(Item);
    }

    return true;
  }
};

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

template <typename T>
func::AsyncResult<func::HttpResult<T>>
Post(const FString &Url, const FString &Payload,
     const FString &ApiKey = TEXT("")) {
  return detail::CreateRequest<T>(TEXT("POST"), Url, ApiKey, Payload);
}

template <typename T>
func::AsyncResult<func::HttpResult<T>> Get(const FString &Url,
                                           const FString &ApiKey = TEXT("")) {
  return detail::CreateRequest<T>(TEXT("GET"), Url, ApiKey);
}

template <typename T>
func::AsyncResult<func::HttpResult<T>>
Delete(const FString &Url, const FString &ApiKey = TEXT("")) {
  return detail::CreateRequest<T>(TEXT("DELETE"), Url, ApiKey);
}

} // namespace AsyncHttp
} // namespace func
