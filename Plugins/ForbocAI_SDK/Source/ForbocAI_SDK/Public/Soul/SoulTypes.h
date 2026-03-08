#pragma once

#include "CoreMinimal.h"
#include "Memory/MemoryTypes.h"
#include "NPC/AgentTypes.h"
#include "SoulTypes.generated.h"

/**
 * Arweave Types (Opaque stubs for parity)
 */
USTRUCT(BlueprintType)
struct FArweaveInstruction {
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly) FString Type;
  FArweaveInstruction() {}
};

USTRUCT(BlueprintType)
struct FArweaveResult {
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly) FString Status;
  FArweaveResult() {}
};

/**
 * Soul (Portable Identity) — Immutable data.
 */
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
  FAgentState State;

  UPROPERTY(BlueprintReadOnly)
  TArray<FMemoryItem> Memories;

  UPROPERTY(BlueprintReadOnly)
  FString Signature;

  FSoul() {}
};

/**
 * Soul Export Phase 1 Response
 */
USTRUCT(BlueprintType)
struct FSoulExportPhase1Response {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FArweaveInstruction se1Instruction;

  UPROPERTY(BlueprintReadOnly)
  FString se1Signature;

  FSoulExportPhase1Response() {}
};

/**
 * Soul Export Confirm Request
 */
USTRUCT(BlueprintType)
struct FSoulExportConfirmRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FArweaveResult secUploadResult;

  UPROPERTY(BlueprintReadOnly)
  FString secSignature;

  FSoulExportConfirmRequest() {}
};

/**
 * Soul Export Response
 */
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

  FSoulExportResponse() {}
};

/**
 * Soul Import Phase 1 Response
 */
USTRUCT(BlueprintType)
struct FSoulImportPhase1Response {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FArweaveInstruction si1Instruction;

  FSoulImportPhase1Response() {}
};

/**
 * Soul Import Confirm Request
 */
USTRUCT(BlueprintType)
struct FSoulImportConfirmRequest {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString sicTxId;

  UPROPERTY(BlueprintReadOnly)
  FArweaveResult sicDownloadResult;

  FSoulImportConfirmRequest() {}
};

/**
 * Soul List Item
 */
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

  FSoulListItem() {}
};

/**
 * Soul List Response
 */
USTRUCT(BlueprintType)
struct FSoulListResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  TArray<FSoulListItem> Souls;

  FSoulListResponse() {}
};

/**
 * Soul Verify Result
 */
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

inline FSoul Soul(FString Id, FString Version, FString Name, FString Persona,
                  FAgentState State, TArray<FMemoryItem> Memories,
                  FString Signature = TEXT("")) {
  FSoul S;
  S.Id = MoveTemp(Id);
  S.Version = MoveTemp(Version);
  S.Name = MoveTemp(Name);
  S.Persona = MoveTemp(Persona);
  S.State = MoveTemp(State);
  S.Memories = MoveTemp(Memories);
  S.Signature = MoveTemp(Signature);
  return S;
}

inline FSoulExportConfirmRequest
SoulExportConfirmRequest(FArweaveResult UploadResult, FString Signature) {
  FSoulExportConfirmRequest R;
  R.secUploadResult = MoveTemp(UploadResult);
  R.secSignature = MoveTemp(Signature);
  return R;
}

inline FSoulImportConfirmRequest
SoulImportConfirmRequest(FString TxId, FArweaveResult DownloadResult) {
  FSoulImportConfirmRequest R;
  R.sicTxId = MoveTemp(TxId);
  R.sicDownloadResult = MoveTemp(DownloadResult);
  return R;
}

} // namespace TypeFactory
