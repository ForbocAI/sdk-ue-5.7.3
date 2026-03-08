#include "Soul/SoulModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"
#include "Soul/SoulSlice.h"

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
      [Soul](std::function<void(FString)> resolve,
             std::function<void(std::string)> reject) {
        auto SDKStore = ConfigureSDKStore();

        // Dispatch the export thunk using the Soul's ID as the AgentId
        auto ThunkResult = rtk::exportSoulThunk(Soul.Id)(
            [SDKStore](const rtk::AnyAction &a) {
              return SDKStore.dispatch(a);
            },
            [SDKStore]() { return SDKStore.getState(); });

        // Chain the result back
        func::AsyncChain::then<FString, void>(ThunkResult, [resolve](
                                                               FString TxId) {
          resolve(TxId);
        }).catch_([reject](std::string Error) { reject(Error); });
      });
}
