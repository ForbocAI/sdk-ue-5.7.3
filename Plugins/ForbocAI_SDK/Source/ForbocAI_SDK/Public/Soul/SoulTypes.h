#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "Memory/MemoryTypes.h"
#include "NPC/NPCBaseTypes.h"
#include "SoulTypes.generated.h"
// clang-format on

USTRUCT(BlueprintType)
struct FArweaveUploadInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString UploadUrl;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString GatewayUrl;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString PayloadJson;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ContentType;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString AuiAuthHeader;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TagsJson;
};

USTRUCT(BlueprintType)
struct FArweaveUploadResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Status;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  int32 StatusCode;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  bool bSuccess;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Error;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ArweaveUrl;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ResponseJson;

  FArweaveUploadResult() : StatusCode(0), bSuccess(false) {}
};

USTRUCT(BlueprintType)
struct FArweaveDownloadInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ExpectedTxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString GatewayUrl;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString DownloadUrl;
};

USTRUCT(BlueprintType)
struct FArweaveDownloadResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString BodyJson;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Payload;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Status;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  int32 StatusCode;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  bool bSuccess;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Error;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ResponseJson;

  FArweaveDownloadResult() : StatusCode(0), bSuccess(false) {}
};

USTRUCT(BlueprintType)
struct FSoul {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Id;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Version;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Name;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  TArray<FMemoryItem> Memories;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FAgentState State;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Signature;
};

USTRUCT(BlueprintType)
struct FSoulExportPhase1Request {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString NpcIdRef;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Persona;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FAgentState NpcState;
};

USTRUCT(BlueprintType)
struct FSoulExportPhase1Response {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FArweaveUploadInstruction se1Instruction;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString se1SignedPayload;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString se1Signature;
};

USTRUCT(BlueprintType)
struct FSoulExportConfirmRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FArweaveUploadResult secUploadResult;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString secSignedPayload;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString secSignature;
};

USTRUCT(BlueprintType)
struct FSoulExportResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ArweaveUrl;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Signature;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FSoul Soul;
};

USTRUCT(BlueprintType)
struct FSoulImportPhase1Request {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxIdRef;
};

USTRUCT(BlueprintType)
struct FSoulImportPhase1Response {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FArweaveDownloadInstruction si1Instruction;
};

USTRUCT(BlueprintType)
struct FSoulImportConfirmRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString sicTxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FArweaveDownloadResult sicDownloadResult;
};

USTRUCT(BlueprintType)
struct FSoulExportResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Url;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FSoul Soul;
};

USTRUCT(BlueprintType)
struct FSoulListItem {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString TxId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Name;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString NpcId;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ExportedAt;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString ArweaveUrl;
};

USTRUCT(BlueprintType)
struct FSoulListResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  TArray<FSoulListItem> Souls;
};

USTRUCT(BlueprintType)
struct FSoulVerifyResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  bool bValid;

  UPROPERTY(BlueprintReadOnly, Category = "Soul")
  FString Reason;

  FSoulVerifyResult() : bValid(false) {}
};

namespace TypeFactory {

inline FSoul Soul(const FString &Id, const FString &Version,
                  const FString &Name, const FString &Persona,
                  const FAgentState &State, const TArray<FMemoryItem> &Memories,
                  const FString &Signature = TEXT("")) {
  FSoul Value;
  Value.Id = Id;
  Value.Version = Version;
  Value.Name = Name;
  Value.Persona = Persona;
  Value.State = State;
  Value.Memories = Memories;
  Value.Signature = Signature;
  return Value;
}

inline FSoulExportPhase1Request
SoulExportPhase1Request(const FString &NpcIdRef, const FString &Persona,
                        const FAgentState &NpcState) {
  FSoulExportPhase1Request Value;
  Value.NpcIdRef = NpcIdRef;
  Value.Persona = Persona;
  Value.NpcState = NpcState;
  return Value;
}

inline FSoulExportConfirmRequest
SoulExportConfirmRequest(const FArweaveUploadResult &UploadResult,
                         const FString &SignedPayload,
                         const FString &Signature) {
  FSoulExportConfirmRequest Value;
  Value.secUploadResult = UploadResult;
  Value.secSignedPayload = SignedPayload;
  Value.secSignature = Signature;
  return Value;
}

inline FSoulImportPhase1Request
SoulImportPhase1Request(const FString &TxIdRef) {
  FSoulImportPhase1Request Value;
  Value.TxIdRef = TxIdRef;
  return Value;
}

inline FSoulImportConfirmRequest
SoulImportConfirmRequest(const FString &TxId,
                         const FArweaveDownloadResult &DownloadResult) {
  FSoulImportConfirmRequest Value;
  Value.sicTxId = TxId;
  Value.sicDownloadResult = DownloadResult;
  return Value;
}

inline FSoulExportResult SoulExportResult(const FString &TxId,
                                          const FString &Url,
                                          const FSoul &SoulValue) {
  FSoulExportResult Value;
  Value.TxId = TxId;
  Value.Url = Url;
  Value.Soul = SoulValue;
  return Value;
}

inline FSoulVerifyResult SoulVerifyResult(bool bValid,
                                          const FString &Reason = TEXT("")) {
  FSoulVerifyResult Value;
  Value.bValid = bValid;
  Value.Reason = Reason;
  return Value;
}

} // namespace TypeFactory
