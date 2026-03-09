#include "CLI/SDKOps.h"
#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SDKStore.h"

using namespace rtk;

// ---------------------------------------------------------------------------
// Test: SDKOps::CreateNpc creates NPC and updates store
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSDKOpsCreateNpcTest,
                                 "ForbocAI.Integration.SDKOps.CreateNpc",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FSDKOpsCreateNpcTest::RunTest(const FString &Parameters) {
  EnhancedStore<FSDKState> Store = createSDKStore();

  FNPCInternalState Result = SDKOps::CreateNpc(Store, TEXT("A loyal guard"));

  TestFalse("NPC Id not empty", Result.Id.IsEmpty());
  TestEqual("Persona matches", Result.Persona,
            FString(TEXT("A loyal guard")));

  // Verify store state
  func::Maybe<FNPCInternalState> Active = SDKOps::GetActiveNpc(Store);
  TestTrue("Active NPC exists", Active.hasValue);
  if (Active.hasValue) {
    TestEqual("Active NPC Id matches created", Active.value.Id, Result.Id);
    TestEqual("Active NPC Persona", Active.value.Persona,
              FString(TEXT("A loyal guard")));
  }

  return true;
}

// ---------------------------------------------------------------------------
// Test: SDKOps::GetActiveNpc returns nothing on empty store
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSDKOpsGetActiveEmptyTest,
                                 "ForbocAI.Integration.SDKOps.GetActiveEmpty",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FSDKOpsGetActiveEmptyTest::RunTest(const FString &Parameters) {
  EnhancedStore<FSDKState> Store = createSDKStore();

  func::Maybe<FNPCInternalState> Active = SDKOps::GetActiveNpc(Store);
  TestFalse("No active NPC on fresh store", Active.hasValue);

  return true;
}

// ---------------------------------------------------------------------------
// Test: SDKOps::ListNpcs returns all created NPCs
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSDKOpsListNpcsTest,
                                 "ForbocAI.Integration.SDKOps.ListNpcs",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FSDKOpsListNpcsTest::RunTest(const FString &Parameters) {
  EnhancedStore<FSDKState> Store = createSDKStore();

  TArray<FNPCInternalState> Empty = SDKOps::ListNpcs(Store);
  TestEqual("Empty list initially", Empty.Num(), 0);

  SDKOps::CreateNpc(Store, TEXT("Guard"));
  SDKOps::CreateNpc(Store, TEXT("Merchant"));
  SDKOps::CreateNpc(Store, TEXT("Thief"));

  TArray<FNPCInternalState> All = SDKOps::ListNpcs(Store);
  TestEqual("Three NPCs listed", All.Num(), 3);

  return true;
}

// ---------------------------------------------------------------------------
// Test: SDKOps::ConfigGet / ConfigSet
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSDKOpsConfigTest,
                                 "ForbocAI.Integration.SDKOps.Config",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FSDKOpsConfigTest::RunTest(const FString &Parameters) {
  // ConfigGet version always returns something
  FString Version = SDKOps::ConfigGet(TEXT("version"));
  TestFalse("Version not empty", Version.IsEmpty());

  // ConfigSet + ConfigGet roundtrip for apiUrl
  SDKOps::ConfigSet(TEXT("apiUrl"), TEXT("https://test.forboc.ai"));
  FString Url = SDKOps::ConfigGet(TEXT("apiUrl"));
  TestEqual("ApiUrl roundtrip", Url,
            FString(TEXT("https://test.forboc.ai")));

  // ConfigSet + ConfigGet roundtrip for apiKey
  SDKOps::ConfigSet(TEXT("apiKey"), TEXT("sk_test_roundtrip"));
  FString Key = SDKOps::ConfigGet(TEXT("apiKey"));
  TestEqual("ApiKey roundtrip", Key, FString(TEXT("sk_test_roundtrip")));

  // Unknown key returns empty
  FString Unknown = SDKOps::ConfigGet(TEXT("nonexistent"));
  TestTrue("Unknown key returns empty", Unknown.IsEmpty());

  return true;
}

// ---------------------------------------------------------------------------
// Test: SDKOps::CreateNpc then remove via store dispatch
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSDKOpsCreateAndRemoveTest,
    "ForbocAI.Integration.SDKOps.CreateAndRemove",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FSDKOpsCreateAndRemoveTest::RunTest(const FString &Parameters) {
  EnhancedStore<FSDKState> Store = createSDKStore();

  FNPCInternalState Npc = SDKOps::CreateNpc(Store, TEXT("Ephemeral"));
  FString NpcId = Npc.Id;

  TestEqual("One NPC exists", SDKOps::ListNpcs(Store).Num(), 1);

  Store.dispatch(NPCSlice::Actions::RemoveNPC(NpcId));

  TestEqual("Zero NPCs after removal", SDKOps::ListNpcs(Store).Num(), 0);

  func::Maybe<FNPCInternalState> Active = SDKOps::GetActiveNpc(Store);
  TestFalse("No active NPC after removal", Active.hasValue);

  return true;
}
