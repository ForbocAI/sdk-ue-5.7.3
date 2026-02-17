#include "SoulModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"

// ==========================================
// SOUL OPERATIONS — Pure free functions
// ==========================================

FSoul SoulOps::FromAgent(const FAgentState &State,
                         const TArray<FMemoryItem> &Memories,
                         const FString &AgentId, const FString &Persona) {
  return TypeFactory::Soul(AgentId, TEXT("1.0.0"), TEXT("Agent Soul"), Persona,
                           State, Memories);
}

FString SoulOps::Serialize(const FSoul &Soul) {
  FString JsonString;
  FJsonObjectConverter::UStructToJsonObjectString(Soul, JsonString);
  return JsonString;
}

FSoul SoulOps::Deserialize(const FString &Json) {
  FSoul Soul;
  if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Soul, 0, 0)) {
    return FSoul();
  }
  return Soul;
}

FValidationResult SoulOps::Validate(const FSoul &Soul) {
  if (Soul.Id.IsEmpty()) {
    return TypeFactory::Invalid(TEXT("Missing Soul ID"));
  }
  if (Soul.Persona.IsEmpty()) {
    return TypeFactory::Invalid(TEXT("Missing Persona"));
  }

  return TypeFactory::Valid(TEXT("Valid Soul"));
}

void SoulOps::ExportToArweave(const FSoul &Soul, const FString &ApiUrl,
                              TFunction<void(FString)> OnComplete) {
  if (ApiUrl.IsEmpty()) {
    if (OnComplete)
      OnComplete("Error: Missing API URL");
    return;
  }

  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
      FHttpModule::Get().CreateRequest();
  Request->SetVerb("POST");
  Request->SetURL(ApiUrl + "/agents/" + Soul.Id + "/soul/export");
  Request->SetHeader("Content-Type", "application/json");

  // Send Agent ID as ref, API handles state lookup/serialization if needed,
  // or we could send full body. For parity with TS SDK 'exportSoulToIPFS' (API
  // fallback), we assume API can handle the export if we just trigger it, OR we
  // send body. TS SDK implementation sends: body: JSON.stringify({ agentIdRef:
  // agentId })

  FString Content = FString::Printf(TEXT("{\"agentIdRef\":\"%s\"}"), *Soul.Id);
  Request->SetContentAsString(Content);

  Request->OnProcessRequestComplete().BindLambda(
      [OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnected) {
        if (bConnected && Res.IsValid() &&
            EHttpResponseCodes::IsOk(Res->GetResponseCode())) {
          // Parse JSON response for TXID / CID
          TSharedPtr<FJsonObject> JsonObj;
          TSharedRef<TJsonReader<>> Reader =
              TJsonReaderFactory<>::Create(Res->GetContentAsString());
          if (FJsonSerializer::Deserialize(Reader, JsonObj)) {
            // Check for 'cid' or 'txId'
            FString TxId;
            if (JsonObj->TryGetStringField("cid", TxId) ||
                JsonObj->TryGetStringField("txId", TxId)) {
              if (OnComplete)
                OnComplete(TxId);
              return;
            }
          }
        }
        if (OnComplete)
          OnComplete("Error: Export Failed");
      });

  Request->ProcessRequest();
}
