#pragma once

#include "CoreMinimal.h"
#include "Core/functional_core.hpp"
#include "ForbocAI_SDK_Types.generated.h" // UHT generated

// ==========================================================
// ForbocAI SDK — Data Types (Strict FP with Enhanced Functional Patterns)
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

// Functional Core Type Aliases for SDK types
namespace SDKTypes {
    using func::Maybe;
    using func::Either;
    using func::Pipeline;
    using func::Curried;
    using func::Lazy;
    using func::ValidationPipeline;
    using func::ConfigBuilder;
    using func::TestResult;
    using func::AsyncResult;
    
    // Type aliases for SDK operations
    using AgentCreationResult = Either<FString, FAgent>;
    using AgentValidationResult = Either<FString, FAgentState>;
    using AgentProcessResult = Either<FString, FAgentResponse>;
    using AgentExportResult = Either<FString, FSoul>;
    using MemoryStoreResult = Either<FString, FMemoryStore>;
    using CortexCreationResult = Either<FString, FCortex>;
    using GhostTestResult = TestResult<FGhostTestResult>;
    using BridgeValidationResult = Either<FString, FValidationResult>;
}

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

/**
 * Agent Entity — Pure immutable data.
 * Construction via AgentFactory namespace (factory functions).
 * Operations via AgentOps namespace (free functions).
 */
struct FAgent {
  /** Unique identifier for the agent. */
  FString Id;
  /** The persona or description of the agent. */
  FString Persona;
  /** The current state of the agent. */
  FAgentState State;
  /** The list of memories associated with the agent. */
  TArray<FMemoryItem> Memories;
  /** The API URL this agent communicates with. */
  FString ApiUrl;
};

/**
 * Memory Configuration — Immutable data.
 */
USTRUCT()
struct FMemoryConfig {
  GENERATED_BODY()

  /** The database file path. */
  UPROPERTY()
  FString DatabasePath;
  
  /** Maximum number of memories to store. */
  UPROPERTY()
  int32 MaxMemories;
  
  /** Vector dimension size. */
  UPROPERTY()
  int32 VectorDimension;
  
  /** Whether to use GPU acceleration if available. */
  UPROPERTY()
  bool UseGPU;
  
  /** Maximum results to return from recall. */
  UPROPERTY()
  int32 MaxRecallResults;

  FMemoryConfig()
      : DatabasePath(TEXT("ForbocAI_Memory.db")), MaxMemories(10000),
        VectorDimension(384), UseGPU(false), MaxRecallResults(10) {}
};

/**
 * Memory Store — Immutable data.
 */
USTRUCT()
struct FMemoryStore {
  GENERATED_BODY()

  /** Configuration for the memory store. */
  UPROPERTY()
  FMemoryConfig Config;

  /** List of immutable memory items. */
  UPROPERTY()
  TArray<FMemoryItem> Items;

  /** Database connection handle. */
  void *DatabaseHandle;

  /** Whether the store is initialized. */
  UPROPERTY()
  bool bInitialized;

  FMemoryStore() : DatabaseHandle(nullptr), bInitialized(false) {}
};

/**
 * Cortex Configuration — Immutable data.
 */
struct FCortexConfig {
  /** The model identifier to use. */
  FString Model;
  /** Whether to use GPU acceleration if available. */
  bool UseGPU;
  /** Maximum tokens to generate per request. */
  int32 MaxTokens;
  /** Temperature for generation (0.0 - 1.0). */
  float Temperature;
  /** Top-k sampling parameter. */
  int32 TopK;
  /** Top-p sampling parameter. */
  float TopP;

  FCortexConfig()
      : Model(TEXT("smalll2-135m")), UseGPU(false), MaxTokens(512),
        Temperature(0.7f), TopK(40), TopP(0.9f) {}
};

/**
 * Cortex Response — Immutable data.
 */
USTRUCT()
struct FCortexResponse {
  GENERATED_BODY()

  /** The generated text response. */
  UPROPERTY()
  FString Text;

  /** The estimated token count. */
  UPROPERTY()
  int32 TokenCount;

  /** Whether the generation completed successfully. */
  UPROPERTY()
  bool bSuccess;

  /** Error message if generation failed. */
  UPROPERTY()
  FString ErrorMessage;

  FCortexResponse() : TokenCount(0), bSuccess(false), ErrorMessage(TEXT("")) {}
};

/**
 * Ghost Test Configuration — Immutable data.
 */
struct FGhostConfig {
  /** The agent to test. */
  FAgent Agent;
  /** Test scenarios to run. */
  TArray<FString> Scenarios;
  /** Maximum number of test iterations. */
  int32 MaxIterations;
  /** Expected response patterns. */
  TMap<FString, FString> ExpectedResponses;
  /** Whether to run in verbose mode. */
  bool bVerbose;
  /** API URL for the agent. */
  FString ApiUrl;

  FGhostConfig() : MaxIterations(100), bVerbose(false) {}
};

/**
 * Ghost Test Result — Immutable data.
 */
USTRUCT()
struct FGhostTestResult {
  GENERATED_BODY()

  /** The test scenario that was run. */
  UPROPERTY()
  FString Scenario;

  /** Whether the test passed. */
  UPROPERTY()
  bool bPassed;

  /** The actual response from the agent. */
  UPROPERTY()
  FString ActualResponse;

  /** The expected response. */
  UPROPERTY()
  FString ExpectedResponse;

  /** Any error messages encountered. */
  UPROPERTY()
  FString ErrorMessage;

  /** The iteration number when the test completed. */
  UPROPERTY()
  int32 Iteration;

  FGhostTestResult() : bPassed(false), Iteration(0) {}
};

/**
 * Ghost Test Report — Immutable data.
 */
USTRUCT()
struct FGhostTestReport {
  GENERATED_BODY()

  /** The overall test configuration. */
  FGhostConfig Config;

  /** All test results. */
  UPROPERTY()
  TArray<FGhostTestResult> Results;

  /** The total number of tests run. */
  UPROPERTY()
  int32 TotalTests;

  /** The number of tests that passed. */
  UPROPERTY()
  int32 PassedTests;

  /** The number of tests that failed. */
  UPROPERTY()
  int32 FailedTests;

  /** The overall success rate. */
  UPROPERTY()
  float SuccessRate;

  /** Any summary messages. */
  UPROPERTY()
  FString Summary;

  FGhostTestReport()
      : TotalTests(0), PassedTests(0), FailedTests(0), SuccessRate(0.0f) {}
};

/**
 * Ghost Test Engine — Immutable data.
 */
struct FGhost {
  /** The configuration for this test engine. */
  FGhostConfig Config;
  /** Whether this ghost has been initialized. */
  bool bInitialized;

  FGhost() : bInitialized(false) {}
};

/**
 * Cortex Engine Handle — Opaque handle to the inference engine.
 * Note: Not a USTRUCT because void* is not supported by reflection.
 */
struct FCortex {
  void *EngineHandle;
  FCortex() : EngineHandle(nullptr) {}
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

// --- FMemoryConfig ---
inline FMemoryConfig MemoryConfig(FString DatabasePath = TEXT("ForbocAI_Memory.db"),
                                 int32 MaxMemories = 10000,
                                 int32 VectorDimension = 384,
                                 bool UseGPU = false,
                                 int32 MaxRecallResults = 10) {
  FMemoryConfig C;
  C.DatabasePath = MoveTemp(DatabasePath);
  C.MaxMemories = MaxMemories;
  C.VectorDimension = VectorDimension;
  C.UseGPU = UseGPU;
  C.MaxRecallResults = MaxRecallResults;
  return C;
}

// --- FCortexConfig ---
inline FCortexConfig CortexConfig(FString Model = TEXT("smalll2-135m"),
                                 bool UseGPU = false,
                                 int32 MaxTokens = 512,
                                 float Temperature = 0.7f,
                                 int32 TopK = 40,
                                 float TopP = 0.9f) {
  FCortexConfig C;
  C.Model = MoveTemp(Model);
  C.UseGPU = UseGPU;
  C.MaxTokens = MaxTokens;
  C.Temperature = Temperature;
  C.TopK = TopK;
  C.TopP = TopP;
  return C;
}

// --- FGhostConfig ---
inline FGhostConfig GhostConfig(FAgent Agent,
                               TArray<FString> Scenarios = {},
                               int32 MaxIterations = 100,
                               TMap<FString, FString> ExpectedResponses = {},
                               bool bVerbose = false,
                               FString ApiUrl = TEXT("")) {
  FGhostConfig C;
  C.Agent = MoveTemp(Agent);
  C.Scenarios = MoveTemp(Scenarios);
  C.MaxIterations = MaxIterations;
  C.ExpectedResponses = MoveTemp(ExpectedResponses);
  C.bVerbose = bVerbose;
  C.ApiUrl = MoveTemp(ApiUrl);
  return C;
}

} // namespace TypeFactory
