#pragma once

#include "CoreMinimal.h"
#include "ForbocAI_SDK_Types.generated.h" // UHT generated

// ==========================================================
// ForbocAI SDK — Data Types (Strict FP)
// ==========================================================
//
// All types are plain data structs. NO parameterized
// constructors, NO member functions, NO classes.
//
// USTRUCTs retain a default constructor only (UE-required
// for reflection). Construction with specific values is
// done exclusively through factory functions in TypeFactory.
//
// Plain structs (non-USTRUCT) use aggregate initialization
// directly or via factory functions.
// ==========================================================

/**
 * Agent State — Immutable data.
 * Default constructor required by UE reflection.
 */
USTRUCT()
struct FAgentState {
  GENERATED_BODY()

  /** JSON-serialized state data. */
  UPROPERTY()
  FString JsonData;

  FAgentState() : JsonData(TEXT("{}")) {}
};

/**
 * Memory Item — Immutable data.
 */
USTRUCT()
struct FMemoryItem {
  GENERATED_BODY()

  /** Unique identifier for the memory. */
  UPROPERTY()
  FString Id;

  /** The content of the memory. */
  UPROPERTY()
  FString Text;

  /** The type of memory (e.g. observation). */
  UPROPERTY()
  FString Type;

  /** Importance score (0.0 - 1.0). */
  UPROPERTY()
  float Importance;

  /** Timestamp of creation. */
  UPROPERTY()
  int64 Timestamp;

  FMemoryItem() : Importance(0.5f), Timestamp(0) {}
};

/**
 * Agent Action — Immutable data.
 */
USTRUCT()
struct FAgentAction {
  GENERATED_BODY()

  /** The type of action to perform. */
  UPROPERTY()
  FString Type;

  /** The target entity or object for the action. */
  UPROPERTY()
  FString Target;

  /** The reasoning behind the action. */
  UPROPERTY()
  FString Reason;

  /** Additional JSON payload for the action. */
  UPROPERTY()
  FString PayloadJson;

  FAgentAction() {}
};

/**
 * Validation Result — Immutable data.
 */
USTRUCT()
struct FValidationResult {
  GENERATED_BODY()

  /** Whether the action is valid. */
  UPROPERTY()
  bool bValid;

  /** Reason for invalidity, if applicable. */
  UPROPERTY()
  FString Reason;

  FValidationResult() : bValid(true) {}
};

/**
 * Soul (Portable Identity) — Immutable data.
 */
USTRUCT()
struct FSoul {
  GENERATED_BODY()

  /** Unique identifier for the soul. */
  UPROPERTY()
  FString Id;

  /** Version of the soul format. */
  UPROPERTY()
  FString Version;

  /** Name of the entity. */
  UPROPERTY()
  FString Name;

  /** Persona description. */
  UPROPERTY()
  FString Persona;

  /** Current state snapshot. */
  UPROPERTY()
  FAgentState State;

  /** List of memories. */
  UPROPERTY()
  TArray<FMemoryItem> Memories;

  FSoul() {}
};

/**
 * Agent Response — Plain data (aggregate-initializable).
 */
struct FAgentResponse {
  /** The agent's spoken dialogue. */
  FString Dialogue;
  /** The action the agent decided to take. */
  FAgentAction Action;
  /** The agent's internal thought process. */
  FString Thought;
};

/**
 * Agent Configuration — Plain data (aggregate-initializable).
 */
struct FAgentConfig {
  /** The persona description. */
  FString Persona;
  /** The API URL to use. */
  FString ApiUrl;
  /** The initial state of the agent. */
  FAgentState InitialState;
};

// ==========================================================
// TypeFactory — Factory functions for USTRUCTs
// ==========================================================
// Since USTRUCTs cannot use aggregate initialization
// (GENERATED_BODY adds hidden members), all construction
// with specific values goes through these factories.
// ==========================================================

namespace TypeFactory {

// --- FAgentState ---
inline FAgentState AgentState(FString JsonData) {
  FAgentState S;
  S.JsonData = MoveTemp(JsonData);
  return S;
}

// --- FMemoryItem ---
inline FMemoryItem MemoryItem(FString Id, FString Text, FString Type,
                              float Importance, int64 Timestamp) {
  FMemoryItem M;
  M.Id = MoveTemp(Id);
  M.Text = MoveTemp(Text);
  M.Type = MoveTemp(Type);
  M.Importance = Importance;
  M.Timestamp = Timestamp;
  return M;
}

// --- FAgentAction ---
inline FAgentAction Action(FString Type, FString Target,
                           FString Reason = TEXT("")) {
  FAgentAction A;
  A.Type = MoveTemp(Type);
  A.Target = MoveTemp(Target);
  A.Reason = MoveTemp(Reason);
  return A;
}

// --- FValidationResult ---
inline FValidationResult Valid(FString Reason) {
  FValidationResult R;
  R.bValid = true;
  R.Reason = MoveTemp(Reason);
  return R;
}

inline FValidationResult Invalid(FString Reason) {
  FValidationResult R;
  R.bValid = false;
  R.Reason = MoveTemp(Reason);
  return R;
}

// --- FSoul ---
inline FSoul Soul(FString Id, FString Version, FString Name, FString Persona,
                  FAgentState State, TArray<FMemoryItem> Memories) {
  FSoul S;
  S.Id = MoveTemp(Id);
  S.Version = MoveTemp(Version);
  S.Name = MoveTemp(Name);
  S.Persona = MoveTemp(Persona);
  S.State = MoveTemp(State);
  S.Memories = MoveTemp(Memories);
  return S;
}

} // namespace TypeFactory
