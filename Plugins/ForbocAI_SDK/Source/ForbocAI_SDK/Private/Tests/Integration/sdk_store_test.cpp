#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SDKStore.h"

using namespace rtk;

// ---------------------------------------------------------------------------
// Test: SDKReducer processes NPC creation across all slices
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSDKStoreNPCCreationTest,
                                 "ForbocAI.Integration.SDKStore.NPCCreation",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FSDKStoreNPCCreationTest::RunTest(const FString &Parameters) {
  FSDKState State;

  FNPCInternalState Info;
  Info.Id = TEXT("int_npc_1");
  Info.Persona = TEXT("Test NPC");

  State = SDKReducer(State, NPCSlice::Actions::SetNPCInfo(Info));

  TestEqual("NPC active", State.NPCs.ActiveNpcId,
            FString(TEXT("int_npc_1")));
  TestTrue("NPC in entities",
           NPCSlice::SelectNPCById(State.NPCs, TEXT("int_npc_1")).hasValue);

  // Other slices remain at initial state
  TestEqual("Memory idle", State.Memory.StorageStatus,
            FString(TEXT("idle")));
  TestEqual("Ghost idle", State.Ghost.Status, FString(TEXT("idle")));
  TestEqual("Bridge idle", State.Bridge.Status, FString(TEXT("idle")));
  TestEqual("Soul export idle", State.Soul.ExportStatus,
            FString(TEXT("idle")));
  TestTrue("Cortex idle",
           State.Cortex.Status == CortexSlice::ECortexEngineStatus::Idle);

  return true;
}

// ---------------------------------------------------------------------------
// Test: NPC removal middleware cascades clears
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSDKStoreRemovalCascadeTest,
    "ForbocAI.Integration.SDKStore.RemovalCascade",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FSDKStoreRemovalCascadeTest::RunTest(const FString &Parameters) {
  EnhancedStore<FSDKState> Store = createSDKStore();

  // Create NPC
  FNPCInternalState Info;
  Info.Id = TEXT("cascade_npc");
  Info.Persona = TEXT("Cascade test");
  Store.dispatch(NPCSlice::Actions::SetNPCInfo(Info));

  TestEqual("NPC active before removal", Store.getState().NPCs.ActiveNpcId,
            FString(TEXT("cascade_npc")));

  // Add some memory
  FMemoryItem MemItem;
  MemItem.Id = TEXT("cascade_mem");
  MemItem.Text = TEXT("Important memory");
  MemItem.Importance = 0.9f;
  Store.dispatch(MemorySlice::Actions::MemoryStoreSuccess(MemItem));
  TestEqual("Memory exists", MemorySlice::SelectAllMemories(
                                  Store.getState().Memory)
                                  .Num(),
            1);

  // Add bridge validation
  Store.dispatch(BridgeSlice::Actions::BridgeValidationPending());
  TestEqual("Bridge validating", Store.getState().Bridge.Status,
            FString(TEXT("validating")));

  // Start ghost session
  Store.dispatch(
      GhostSlice::Actions::GhostSessionStarted(TEXT("gs_1"), TEXT("running")));
  TestEqual("Ghost running", Store.getState().Ghost.Status,
            FString(TEXT("running")));

  // Add a directive
  Store.dispatch(DirectiveSlice::Actions::DirectiveRunStarted(
      TEXT("dir_1"), TEXT("cascade_npc"), TEXT("observe")));

  // Remove NPC — should cascade clear
  Store.dispatch(NPCSlice::Actions::RemoveNPC(TEXT("cascade_npc")));

  TestTrue("NPC removed",
           !NPCSlice::SelectNPCById(Store.getState().NPCs, TEXT("cascade_npc"))
                .hasValue);
  TestTrue("ActiveNpcId cleared",
           Store.getState().NPCs.ActiveNpcId.IsEmpty());

  // Cascade effects
  TestEqual("Memory cleared after active NPC removal",
            MemorySlice::SelectAllMemories(Store.getState().Memory).Num(), 0);
  TestEqual("Bridge reset", Store.getState().Bridge.Status,
            FString(TEXT("idle")));
  TestEqual("Ghost reset", Store.getState().Ghost.Status,
            FString(TEXT("idle")));
  TestTrue("Directive cleared",
           Store.getState().Directives.ActiveDirectiveId.IsEmpty());
  TestEqual("Soul reset", Store.getState().Soul.ExportStatus,
            FString(TEXT("idle")));

  return true;
}

// ---------------------------------------------------------------------------
// Test: Multiple NPCs, remove non-active — no cascade on memory
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSDKStoreRemoveNonActiveTest,
    "ForbocAI.Integration.SDKStore.RemoveNonActive",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FSDKStoreRemoveNonActiveTest::RunTest(const FString &Parameters) {
  EnhancedStore<FSDKState> Store = createSDKStore();

  // Create two NPCs
  FNPCInternalState A;
  A.Id = TEXT("npc_a");
  A.Persona = TEXT("Alpha");
  Store.dispatch(NPCSlice::Actions::SetNPCInfo(A));

  FNPCInternalState B;
  B.Id = TEXT("npc_b");
  B.Persona = TEXT("Beta");
  Store.dispatch(NPCSlice::Actions::SetNPCInfo(B));

  // Active is now B
  TestEqual("Active is B", Store.getState().NPCs.ActiveNpcId,
            FString(TEXT("npc_b")));

  // Store memory
  FMemoryItem Mem;
  Mem.Id = TEXT("keep_mem");
  Mem.Text = TEXT("Should survive");
  Mem.Importance = 0.5f;
  Store.dispatch(MemorySlice::Actions::MemoryStoreSuccess(Mem));

  // Remove A (not active) — memory should NOT be cleared
  Store.dispatch(NPCSlice::Actions::RemoveNPC(TEXT("npc_a")));

  TestFalse("A removed",
            NPCSlice::SelectNPCById(Store.getState().NPCs, TEXT("npc_a"))
                .hasValue);
  TestEqual("Active still B", Store.getState().NPCs.ActiveNpcId,
            FString(TEXT("npc_b")));
  TestEqual("Memory preserved",
            MemorySlice::SelectAllMemories(Store.getState().Memory).Num(), 1);

  return true;
}
