#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "functional_core.hpp"

namespace func {
namespace AsyncHttp {

template <typename T>
AsyncResult<HttpResult<T>> Post(const FString &Url, const FString &Payload,
                                const FString &ApiKey = TEXT("")) {
  return AsyncResult<HttpResult<T>>::create([Url, Payload, ApiKey](
                                                auto resolve, auto reject) {
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"),
                       TEXT("application/json; charset=utf-8"));
    if (!ApiKey.IsEmpty()) {
      Request->SetHeader(TEXT("Authorization"),
                         FString::Printf(TEXT("Bearer %s"), *ApiKey));
    }
    Request->SetContentAsString(Payload);

    Request->OnProcessRequestComplete().BindLambda(
        [resolve, reject](FHttpRequestPtr Req, FHttpResponsePtr Res,
                          bool bWasSuccessful) {
          if (!bWasSuccessful || !Res.IsValid()) {
            resolve(HttpResult<T>::Failure("Network failure", 0));
            return;
          }

          HttpStatusCode Code = Res->GetResponseCode();
          FString Content = Res->GetContentAsString();

          if (Code >= 200 && Code < 300) {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Content);
            if (FJsonSerializer::Deserialize(Reader, JsonObject) &&
                JsonObject.IsValid()) {
              T ResultPayload;
              if (FJsonObjectConverter::JsonObjectToUStruct(
                      JsonObject.ToSharedRef(), T::StaticStruct(),
                      &ResultPayload)) {
                resolve(HttpResult<T>::Success(ResultPayload, Code));
              } else {
                resolve(HttpResult<T>::Failure("JSON Mapping failed", Code));
              }
            } else {
              resolve(HttpResult<T>::Failure("JSON Parsing failed", Code));
            }
          } else {
            resolve(HttpResult<T>::Failure(TCHAR_TO_UTF8(*Content), Code));
          }
        });

    Request->ProcessRequest();
  });
}

template <typename T>
AsyncResult<HttpResult<T>> Get(const FString &Url,
                               const FString &ApiKey = TEXT("")) {
  return AsyncResult<HttpResult<T>>::create([Url, ApiKey](auto resolve,
                                                          auto reject) {
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    if (!ApiKey.IsEmpty()) {
      Request->SetHeader(TEXT("Authorization"),
                         FString::Printf(TEXT("Bearer %s"), *ApiKey));
    }

    Request->OnProcessRequestComplete().BindLambda(
        [resolve, reject](FHttpRequestPtr Req, FHttpResponsePtr Res,
                          bool bWasSuccessful) {
          if (!bWasSuccessful || !Res.IsValid()) {
            resolve(HttpResult<T>::Failure("Network failure", 0));
            return;
          }

          HttpStatusCode Code = Res->GetResponseCode();
          FString Content = Res->GetContentAsString();

          if (Code >= 200 && Code < 300) {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Content);
            if (FJsonSerializer::Deserialize(Reader, JsonObject) &&
                JsonObject.IsValid()) {
              T ResultPayload;
              if (FJsonObjectConverter::JsonObjectToUStruct(
                      JsonObject.ToSharedRef(), T::StaticStruct(),
                      &ResultPayload)) {
                resolve(HttpResult<T>::Success(ResultPayload, Code));
              } else {
                resolve(HttpResult<T>::Failure("JSON Mapping failed", Code));
              }
            } else {
              resolve(HttpResult<T>::Failure("JSON Parsing failed", Code));
            }
          } else {
            resolve(HttpResult<T>::Failure(TCHAR_TO_UTF8(*Content), Code));
          }
        });

    Request->ProcessRequest();
  });
}

} // namespace AsyncHttp
} // namespace func
