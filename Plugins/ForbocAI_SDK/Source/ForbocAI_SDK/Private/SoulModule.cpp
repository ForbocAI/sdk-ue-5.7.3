#include "SoulModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"

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
          reject("Error: Missing API URL");
          return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
            FHttpModule::Get().CreateRequest();
        Request->SetVerb("POST");
        Request->SetURL(ApiUrl + "/agents/" + Soul.Id + "/soul/export");
        Request->SetHeader("Content-Type", "application/json");

        FString Content =
            FString::Printf(TEXT("{\"agentIdRef\":\"%s\"}"), *Soul.Id);
        Request->SetContentAsString(Content);

        Request->OnProcessRequestComplete().BindLambda(
            [resolve, reject](FHttpRequestPtr Req, FHttpResponsePtr Res,
                              bool bConnected) {
              if (bConnected && Res.IsValid() &&
                  EHttpResponseCodes::IsOk(Res->GetResponseCode())) {
                // Parse JSON response for TXID / CID
                TSharedPtr<FJsonObject> JsonObj;
                TSharedRef<TJsonReader<>> Reader =
                    TJsonReaderFactory<>::Create(Res->GetContentAsString());
                if (FJsonSerializer::Deserialize(Reader, JsonObj)) {
                  // Check for 'cid' or 'txId'
                  FString TxId;
                  if (JsonObj->TryGetStringField(TEXT("cid"), TxId) ||
                      JsonObj->TryGetStringField(TEXT("txId"), TxId)) {
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
