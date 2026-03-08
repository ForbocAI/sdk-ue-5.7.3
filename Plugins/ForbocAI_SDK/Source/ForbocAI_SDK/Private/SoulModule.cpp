#include "SoulModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"
#include "SoulSlice.h"

// ==========================================
// SOUL OPERATIONS — Pure free functions
// ==========================================

SoulTypes::SoulCreationResult
SoulOps::FromAgent(const FAgentState &State,
                   const TArray<FMemoryItem> &Memories, const FString &AgentId,
                   const FString &Persona) {
  try {
    FSoul Soul = TypeFactory::Soul(AgentId, TEXT("1.0.0"), TEXT("Agent Soul"),
                                   Persona, State, Memories);
    return SoulTypes::make_right(FString(), Soul);
  } catch (const std::exception &e) {
    return SoulTypes::make_left(FString(e.what()));
  }
}

SoulTypes::SoulSerializationResult SoulOps::Serialize(const FSoul &Soul) {
  try {
    FString JsonString;
    if (FJsonObjectConverter::UStructToJsonObjectString(Soul, JsonString)) {
      return SoulTypes::make_right(FString(), JsonString);
    }
    return SoulTypes::make_left(
        FString(TEXT("Failed to serialize Soul to JSON")));
  } catch (const std::exception &e) {
    return SoulTypes::make_left(FString(e.what()));
  }
}

SoulTypes::SoulDeserializationResult SoulOps::Deserialize(const FString &Json) {
  try {
    FSoul Soul;
    if (FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Soul, 0, 0)) {
      return SoulTypes::make_right(FString(), Soul);
    }
    return SoulTypes::make_left(
        FString(TEXT("Failed to deserialize JSON to Soul")));
  } catch (const std::exception &e) {
    return SoulTypes::make_left(FString(e.what()));
  }
}

SoulTypes::SoulValidationResult SoulOps::Validate(const FSoul &Soul) {
  // Use the functional validation pipeline from helpers
  auto result = SoulHelpers::soulValidationPipeline().run(Soul);

  if (result.isLeft) {
    return SoulTypes::make_left(result.left);
  }

  return SoulTypes::make_right(FString(), Soul);
}

SoulTypes::SoulExportResult SoulOps::ExportToArweave(const FSoul &Soul,
                                                     const FString &ApiUrl) {
  return SoulTypes::AsyncResult<FString>::create(
      [Soul, ApiUrl](std::function<void(FString)> resolve,
                     std::function<void(std::string)> reject) {
    if (ApiUrl.IsEmpty()) {
      reject("Missing API URL");
      return;
    }

    auto SDKStore = ConfigureSDKStore();
    SDKStore.dispatch(SoulActions::SoulExportPending());

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();
    Request->SetVerb(TEXT("POST"));
    Request->SetURL(ApiUrl + TEXT("/soul/export"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer sk_soul_key"));

    FString Content;
    auto SoulRes = Serialize(Soul);
    if (SoulRes.isRight) {
      Content = SoulRes.right;
    }
    Request->SetContentAsString(Content);

        Request->OnProcessRequestComplete().BindLambda(
            [resolve, reject, SDKStore](FHttpRequestPtr Req, FHttpResponsePtr Res,
                               bool bSuccess) {
      if (bSuccess && Res.IsValid() &&
          EHttpResponseCodes::IsOk(Res->GetResponseCode())) {
        resolve(TxId);
        return;
      }
                }
              }
              reject("Error: Export Failed or Invalid Response");
});

Request->ProcessRequest();
});
}
