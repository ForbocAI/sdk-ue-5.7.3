#pragma once

#include "Core/functional_core.hpp"
#include "CoreMinimal.h"

// Subdomain Types
#include "Bridge/BridgeTypes.h"
#include "Core/SystemTypes.h"
#include "Cortex/CortexTypes.h"
#include "Ghost/GhostTypes.h"
#include "Memory/MemoryTypes.h"
#include "NPC/AgentTypes.h"
#include "Protocol/ProtocolRequestTypes.h"
#include "Protocol/ProtocolTypes.h"
#include "Soul/SoulTypes.h"

#include "Types.generated.h" // UHT generated

// ==========================================================
// ForbocAI SDK — Data Types (Strict FP with Master Include)
// ==========================================================
//
// This file aggregates all subdomain types for convenience.
// All types are plain data structs. NO parameterized
// constructors, NO member functions, NO classes.
// ==========================================================

// Functional Core Type Aliases for SDK types
namespace SDKTypes {
using func::AsyncResult;
using func::ConfigBuilder;
using func::Curried;
using func::Either;
using func::Lazy;
using func::Maybe;
using func::Pipeline;
using func::TestResult;
using func::ValidationPipeline;

// Type aliases for SDK operations
using AgentCreationResult = Either<FString, FAgent>;
using AgentValidationResult = Either<FString, FAgentState>;
using AgentProcessResult = Either<FString, FAgentResponse>;
using AgentExportResult = Either<FString, FSoul>;
using MemoryStoreResult = Either<FString, FMemoryStore>;
using CortexCreationResult = Either<FString, FCortex>;
using GhostTestResult = TestResult<FGhostTestResult>;
using BridgeValidationResult = Either<FString, FValidationResult>;
} // namespace SDKTypes
