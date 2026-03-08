#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "rtk_test_mocks.h"

using namespace rtk;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkStoreAndSliceTest,
                                 "ForbocAI.Core.RTK.StoreAndSlice",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkStoreAndSliceTest::RunTest(const FString &Parameters) {
  // Build Slice
  SliceBuilder<FNpcMockState> Builder(TEXT("npc"),
                                      FNpcMockState{TEXT(""), 100});

  auto SetInfoAction = Builder.createCase<FNpcMockState>(
      TEXT("setInfo"),
      [](const FNpcMockState &State, const Action<FNpcMockState> &Action) {
        return Action.PayloadValue;
      });

  auto ResetAction = Builder.createCase(
      TEXT("reset"),
      [](const FNpcMockState &State,
         const Action<rtk::FEmptyPayload> &Action) {
        return FNpcMockState{TEXT(""), 100};
      });

  Slice<FNpcMockState> NpcSlice = Builder.build();

  // Combine
  auto RootReducer = combineReducers<FAppMockState>()
                         .add(&FAppMockState::ActiveNpc, NpcSlice.Reducer)
                         .build();

  // Create Store
  FAppMockState InitialState{FNpcMockState{TEXT(""), 100}};
  Store<FAppMockState> AppStore(InitialState, RootReducer);

  int CallCount = 0;
  auto Unsub = AppStore.subscribe([&CallCount]() { CallCount++; });

  AppStore.dispatch(SetInfoAction(FNpcMockState{TEXT("npc_1"), 80}));

  TestEqual("Dispatch updates root state ID", AppStore.getState().ActiveNpc.Id,
            FString(TEXT("npc_1")));
  TestEqual("Dispatch updates root state Health",
            AppStore.getState().ActiveNpc.Health, 80);
  TestEqual("Subscriber triggered", CallCount, 1);

  AppStore.dispatch(ResetAction());
  TestEqual("Empty payload dispatch clears ID",
            AppStore.getState().ActiveNpc.Id, FString(TEXT("")));
  TestEqual("Subscriber triggered again", CallCount, 2);

  Unsub();
  AppStore.dispatch(SetInfoAction(FNpcMockState{TEXT("npc_2"), 50}));
  TestEqual("Subscriber not triggered after Unsub", CallCount, 2);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkConfigureStoreTest,
                                 "ForbocAI.Core.RTK.ConfigureStore",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkConfigureStoreTest::RunTest(const FString &Parameters) {
  // 1. Setup Reducer
  auto RootReducer = [](const FAppMockState &State, const AnyAction &Action) {
    FAppMockState Next = State;
    if (Action.Type == TEXT("trigger")) {
      Next.ActiveNpc.Health -= 10;
    }
    return Next;
  };

  FAppMockState PreloadState{FNpcMockState{TEXT("MockNpc"), 100}};

  // 2. Setup Middleware
  TArray<FString> EventLog;
  Middleware<FAppMockState> AuditMw =
      [&EventLog](const MiddlewareApi<FAppMockState> &Api) {
        return [&EventLog](NextDispatcher<FAppMockState> Next) {
          return [&EventLog, Next](const AnyAction &Action) -> AnyAction {
            EventLog.Add(FString::Printf(TEXT("MW:%s"), *Action.Type));
            return Next(Action);
          };
        };
      };

  std::vector<Middleware<FAppMockState>> Middlewares = {AuditMw};

  // 3. Configure Store
  auto Store =
      configureStore<FAppMockState>(RootReducer, PreloadState, Middlewares);

  // 4. Assert Preload
  TestEqual("Preloaded Health", Store.getState().ActiveNpc.Health, 100);

  // 5. Dispatch Action & Validate Middleware Chain + State update
  Store.dispatch(
      AnyAction{TEXT("trigger"), std::make_shared<rtk::FEmptyPayload>()});

  TestEqual("State Updated", Store.getState().ActiveNpc.Health, 90);
  TestEqual("Middleware Log Length", EventLog.Num(), 1);
  if (EventLog.Num() == 1) {
    TestEqual("Middleware intercept fired", EventLog[0],
              FString(TEXT("MW:trigger")));
  }

  return true;
}
