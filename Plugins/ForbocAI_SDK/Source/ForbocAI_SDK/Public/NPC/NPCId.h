#pragma once

#include "CoreMinimal.h"

namespace NPCId {

/**
 * Encodes an integer value as base36 text.
 * User Story: As id generation, I need compact base36 encoding so NPC ids stay
 * short while preserving timestamp uniqueness.
 */
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

/**
 * Generates an NPC id matching TS SDK shape: ag_<base36 timestamp>
 * TS: generateNPCId() => `ag_${Date.now().toString(36)}`
 * Parity: Unix milliseconds, base36.
 * User Story: As cross-SDK id generation, I need UE NPC ids to match the TS
 * format so imported and synchronized agents share one identifier shape.
 */
inline FString GenerateNPCId() {
  const int64 UnixMs =
      FDateTime::UtcNow().ToUnixTimestamp() * 1000;
  return TEXT("ag_") + ToBase36(static_cast<uint64>(UnixMs));
}

} // namespace NPCId
