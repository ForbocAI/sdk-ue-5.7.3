#pragma once

#include "CoreMinimal.h"

namespace NPCId {

inline FString ToBase36(uint64 Value) {
  const TCHAR Digits[] = TEXT("0123456789abcdefghijklmnopqrstuvwxyz");
  if (Value == 0) {
    return TEXT("0");
  }

  FString Encoded;
  while (Value > 0) {
    const int32 Index = static_cast<int32>(Value % 36);
    Encoded.InsertAt(0, FString::Chr(Digits[Index]));
    Value /= 36;
  }
  return Encoded;
}

inline FString GenerateNPCId() {
  const int64 Milliseconds =
      FDateTime::UtcNow().GetTicks() / ETimespan::TicksPerMillisecond;
  return TEXT("ag_") + ToBase36(static_cast<uint64>(Milliseconds));
}

} // namespace NPCId
