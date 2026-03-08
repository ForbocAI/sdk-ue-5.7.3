#pragma once

#include "CoreMinimal.h"
#include "Memory/MemoryTypes.h"
#include "NPC/AgentTypes.h"
#include "SoulTypes.generated.h"

USTRUCT(BlueprintType)
struct FArweaveUploadInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString UploadUrl;

  UPROPERTY(BlueprintReadOnly)
  FString GatewayUrl;

  UPROPERTY(BlueprintReadOnly)
  FString ContentType;

  UPROPERTY(BlueprintReadOnly)
  FString AuiAuthHeader;

  UPROPERTY(BlueprintReadOnly)
  FString TagsJson;
};

USTRUCT(BlueprintType)
struct FArweaveUploadResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxId;

  UPROPERTY(BlueprintReadOnly)
  FString Status;

  UPROPERTY(BlueprintReadOnly)
  FString ArweaveUrl;

  UPROPERTY(BlueprintReadOnly)
  FString ResponseJson;
};

USTRUCT(BlueprintType)
struct FArweaveDownloadInstruction {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxId;

  UPROPERTY(BlueprintReadOnly)
  FString GatewayUrl;

  UPROPERTY(BlueprintReadOnly)
  FString DownloadUrl;
};

USTRUCT(BlueprintType)
struct FArweaveDownloadResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxId;

  UPROPERTY(BlueprintReadOnly)
  FString Payload;

  UPROPERTY(BlueprintReadOnly)
  FString Status;

  UPROPERTY(BlueprintReadOnly)
  FString ResponseJson;
};

USTRUCT(BlueprintType)
struct FSoul {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Version;

  UPROPERTY(BlueprintReadOnly)
  FString Name;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  TArray<FMemoryItem> Memories;

  UPROPERTY(BlueprintReadOnly)
  FAgentState State;

  UPROPERTY(BlueprintReadOnly)
  FString Signature;
};

USTRUCT(BlueprintType)
struct FSoulExportPhase1Request {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString NpcIdRef;

  UPROPERTY(BlueprintReadOnly)
  FString Persona;

  UPROPERTY(BlueprintReadOnly)
  FAgentState NpcState;
};

USTRUCT(BlueprintType)
struct FSoulExportPhase1Response {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FArweaveUploadInstruction se1Instruction;

  UPROPERTY(BlueprintReadOnly)
  FString se1SignedPayload;

  UPROPERTY(BlueprintReadOnly)
  FString se1Signature;
};

USTRUCT(BlueprintType)
struct FSoulExportConfirmRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FArweaveUploadResult secUploadResult;

  UPROPERTY(BlueprintReadOnly)
  FString secSignedPayload;

  UPROPERTY(BlueprintReadOnly)
  FString secSignature;
};

USTRUCT(BlueprintType)
struct FSoulExportResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxId;

  UPROPERTY(BlueprintReadOnly)
  FString ArweaveUrl;

  UPROPERTY(BlueprintReadOnly)
  FString Signature;

  UPROPERTY(BlueprintReadOnly)
  FSoul Soul;
};

USTRUCT(BlueprintType)
struct FSoulImportPhase1Request {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxIdRef;
};

USTRUCT(BlueprintType)
struct FSoulImportPhase1Response {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FArweaveDownloadInstruction si1Instruction;
};

USTRUCT(BlueprintType)
struct FSoulImportConfirmRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString sicTxId;

  UPROPERTY(BlueprintReadOnly)
  FArweaveDownloadResult sicDownloadResult;
};

USTRUCT(BlueprintType)
struct FSoulExportResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxId;

  UPROPERTY(BlueprintReadOnly)
  FString Url;

  UPROPERTY(BlueprintReadOnly)
  FSoul Soul;
};

USTRUCT(BlueprintType)
struct FSoulListItem {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString TxId;

  UPROPERTY(BlueprintReadOnly)
  FString Name;

  UPROPERTY(BlueprintReadOnly)
  FString NpcId;

  UPROPERTY(BlueprintReadOnly)
  FString ExportedAt;

  UPROPERTY(BlueprintReadOnly)
  FString ArweaveUrl;
};

USTRUCT(BlueprintType)
struct FSoulListResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  TArray<FSoulListItem> Souls;
};

USTRUCT(BlueprintType)
struct FSoulVerifyResult {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  bool bValid;

  UPROPERTY(BlueprintReadOnly)
  FString Reason;

  FSoulVerifyResult() : bValid(false) {}
};

namespace TypeFactory {

inline FSoul Soul(const FString &Id, const FString &Version, const FString &Name,
                  const FString &Persona, const FAgentState &State,
                  const TArray<FMemoryItem> &Memories,
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

inline FSoulImportPhase1Request SoulImportPhase1Request(const FString &TxIdRef) {
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
