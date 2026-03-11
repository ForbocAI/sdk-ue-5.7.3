#pragma once

// clang-format off
#include "CoreMinimal.h"
#include "SystemTypes.generated.h"
// clang-format on

/**
 * Api Status Response
 */
USTRUCT(BlueprintType)
struct FApiStatusResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "System")
  FString Status;

  UPROPERTY(BlueprintReadOnly, Category = "System")
  FString Version;

  FApiStatusResponse() {}
};

namespace TypeFactory {

inline FApiStatusResponse ApiStatusResponse(FString Status, FString Version) {
  FApiStatusResponse R;
  R.Status = MoveTemp(Status);
  R.Version = MoveTemp(Version);
  return R;
}

} // namespace TypeFactory
