#pragma once

#include "CoreMinimal.h"
#include "Core/functional_core.hpp"
#include "RuntimeStore.h"

/**
 * Per-domain CLI command handlers.
 * Each handler returns Maybe<TestResult<void>>:
 *   - just(result) when the command belongs to that domain.
 *   - nothing() when the command is not for that domain.
 *
 * F.1 — Split monolithic CLIModule into domain-specific handlers.
 * All dispatch through store thunks via Ops; no raw module calls.
 */
namespace CLIOps {
namespace Handlers {

using Result = func::TestResult<void>;
using HandlerResult = func::Maybe<Result>;

/** System: version, status, doctor, system_status */
HandlerResult HandleSystem(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args);

/** NPC: npc_create, npc_list, npc_process, npc_active, npc_state, npc_update, npc_import, npc_chat */
HandlerResult HandleNpc(rtk::EnhancedStore<FStoreState> &Store,
                       const FString &CommandKey,
                       const TArray<FString> &Args);

/** Memory: memory_list, memory_recall, memory_store, memory_clear, memory_export */
HandlerResult HandleMemory(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args);

/** Cortex: cortex_init, cortex_init_remote, cortex_models, cortex_complete */
HandlerResult HandleCortex(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args);

/** Ghost: ghost_run, ghost_status, ghost_results, ghost_stop, ghost_history */
HandlerResult HandleGhost(rtk::EnhancedStore<FStoreState> &Store,
                         const FString &CommandKey,
                         const TArray<FString> &Args);

/** Bridge + Rules: bridge_validate, bridge_rules, bridge_preset, rules_* */
HandlerResult HandleBridge(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args);

/** Soul: soul_export, soul_import, soul_import_npc, soul_list, soul_verify */
HandlerResult HandleSoul(rtk::EnhancedStore<FStoreState> &Store,
                        const FString &CommandKey,
                        const TArray<FString> &Args);

/** Config: config_set, config_get, config_list */
HandlerResult HandleConfig(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args);

/** Vector: vector_init */
HandlerResult HandleVector(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args);

/** Setup: setup, setup_deps, setup_check, setup_verify, setup_build_llama */
HandlerResult HandleSetup(rtk::EnhancedStore<FStoreState> &Store,
                         const FString &CommandKey,
                         const TArray<FString> &Args);

} // namespace Handlers
} // namespace CLIOps
