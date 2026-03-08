#pragma once

#include "CoreMinimal.h"
#include "SystemTypes.generated.h"

/**
 * Api Status Response
 */
USTRUCT(BlueprintType)
struct FApiStatusResponse {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Status;

  UPROPERTY(BlueprintReadOnly)
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
