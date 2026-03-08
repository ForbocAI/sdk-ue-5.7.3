#include "Core/rtk.hpp"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

using namespace rtk;

struct FNpcMockState {
  FString Id;
  int32 Health;

  bool operator==(const FNpcMockState &Other) const {
    return Id == Other.Id && Health == Other.Health;
  }
};

struct FAppMockState {
  FNpcMockState ActiveNpc;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkStoreAndSliceTest,
                                 "ForbocAI.Core.RTK.StoreAndSlice",
                                 EAutomationTestFlags::ApplicationContextMask |
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
      [](const FNpcMockState &State, const Action<FEmptyPayload> &Action) {
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkEntityAdapterTest,
                                 "ForbocAI.Core.RTK.EntityAdapter",
                                 EAutomationTestFlags::ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkEntityAdapterTest::RunTest(const FString &Parameters) {
  auto Adapter = createEntityAdapter<FNpcMockState>(
      [](const FNpcMockState &E) { return E.Id; });
  auto State = Adapter.getInitialState();
  auto Selectors = Adapter.getSelectors();

  State = Adapter.addOne(State, FNpcMockState{TEXT("1"), 100});
  TestEqual("addOne total count", Selectors.selectTotal(State), 1);

  auto Ent1 = Selectors.selectById(State, TEXT("1"));
  TestTrue("selectById finds entity", Ent1.hasValue);
  TestEqual("selectById accurate health", Ent1.value.Health, 100);

  State = Adapter.addMany(
      State, {FNpcMockState{TEXT("2"), 200}, FNpcMockState{TEXT("3"), 300}});
  TestEqual("addMany total count", Selectors.selectTotal(State), 3);

  State = Adapter.updateOne(State, TEXT("3"), [](const FNpcMockState &E) {
    FNpcMockState Next = E;
    Next.Health = 350;
    return Next;
  });

  auto Ent3 = Selectors.selectById(State, TEXT("3"));
  TestEqual("updateOne payload accurate", Ent3.value.Health, 350);

  State = Adapter.removeOne(State, TEXT("1"));
  TestEqual("removeOne total count", Selectors.selectTotal(State), 2);
  TestFalse("removeOne removed entity from lookup",
            Selectors.selectById(State, TEXT("1")).hasValue);

  State = Adapter.setAll(State, {FNpcMockState{TEXT("4"), 400}});
  TestEqual("setAll total count", Selectors.selectTotal(State), 1);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkAsyncThunkTest,
                                 "ForbocAI.Core.RTK.AsyncThunk",
                                 EAutomationTestFlags::ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkAsyncThunkTest::RunTest(const FString &Parameters) {
  // Build a mock async thunk
  auto TestThunk = createAsyncThunk<int, FString, FAppMockState>(
      TEXT("test/fetchData"),
      [](const FString &Arg,
         const ThunkApi<FAppMockState> &Api) -> func::AsyncResult<int> {
        return func::AsyncResult<int>::create(
            [Arg](std::function<void(int)> Resolve,
                  std::function<void(std::string)> Reject) {
              if (Arg == TEXT("fail")) {
                Reject("Mock error");
              } else {
                Resolve(42);
              }
            });
      });

  TArray<FString> DispatchedActions;
  std::function<AnyAction(const AnyAction &)> MockDispatch =
      [&DispatchedActions](const AnyAction &Action) {
        DispatchedActions.Add(Action.Type);
        return Action;
      };

  std::function<FAppMockState()> MockGetState = []() {
    return FAppMockState{};
  };

  // Test Success Path
  auto ThunkActionSuccess = TestThunk(TEXT("success"));
  auto ResultSuccess = ThunkActionSuccess(MockDispatch, MockGetState);
  ResultSuccess.execute();

  TestEqual("Dispatched pending first (success)", DispatchedActions[0],
            FString(TEXT("test/fetchData/pending")));
  TestEqual("Dispatched fulfilled second (success)", DispatchedActions[1],
            FString(TEXT("test/fetchData/fulfilled")));
  DispatchedActions.Empty();

  // Test Failure Path
  auto ThunkActionFail = TestThunk(TEXT("fail"));
  auto ResultFail = ThunkActionFail(MockDispatch, MockGetState);
  ResultFail.execute();

  TestEqual("Dispatched pending first (fail)", DispatchedActions[0],
            FString(TEXT("test/fetchData/pending")));
  TestEqual("Dispatched rejected second (fail)", DispatchedActions[1],
            FString(TEXT("test/fetchData/rejected")));

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkMiddlewareTest,
                                 "ForbocAI.Core.RTK.Middleware",
                                 EAutomationTestFlags::ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkMiddlewareTest::RunTest(const FString &Parameters) {
  TArray<FString> EventLog;

  // 1. Setup Base Dispatch and GetState
  std::function<AnyAction(const AnyAction &)> BaseDispatch =
      [&EventLog](const AnyAction &Action) {
        EventLog.Add(FString::Printf(TEXT("BaseDispatch:%s"), *Action.Type));
        return Action;
      };

  std::function<FAppMockState()> GetState = []() { return FAppMockState{}; };

  // 2. Setup Middleware A (Logging before and after)
  Middleware<FAppMockState> MiddlewareA =
      [&EventLog](const MiddlewareApi<FAppMockState> &Api) {
        return [&EventLog](NextDispatcher<FAppMockState> Next) {
          return [&EventLog, Next](const AnyAction &Action) -> AnyAction {
            EventLog.Add(TEXT("MwA_Before"));
            auto Result = Next(Action);
            EventLog.Add(TEXT("MwA_After"));
            return Result;
          };
        };
      };

  // 3. Setup Listener Middleware (MwB)
  ListenerMiddleware<FAppMockState> Listeners;
  Listeners.addListener(TEXT("trigger"),
                        [&EventLog](const AnyAction &Action,
                                    const MiddlewareApi<FAppMockState> &Api) {
                          EventLog.Add(TEXT("Listener_Triggered"));
                        });

  // 4. Compose
  std::vector<Middleware<FAppMockState>> Chain = {MiddlewareA,
                                                  Listeners.getMiddleware()};
  auto EnhancedDispatch = applyMiddleware(BaseDispatch, GetState, Chain);

  // 5. Execute
  EnhancedDispatch(
      AnyAction{TEXT("trigger"), std::make_shared<FEmptyPayload>()});

  // Validate Order
  TestEqual("Event Log Length", EventLog.Num(), 4);
  if (EventLog.Num() == 4) {
    TestEqual("0: MwA wraps everything", EventLog[0],
              FString(TEXT("MwA_Before")));
    TestEqual("1: Base runs inside", EventLog[1],
              FString(TEXT("BaseDispatch:trigger")));
    TestEqual("2: Listener runs post-reducer inside MwB", EventLog[2],
              FString(TEXT("Listener_Triggered")));
    TestEqual("3: MwA finishes", EventLog[3], FString(TEXT("MwA_After")));
  }

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkSelectorTest, "ForbocAI.Core.RTK.Selector",
                                 EAutomationTestFlags::ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkSelectorTest::RunTest(const FString &Parameters) {
  struct FTestState {
    int32 A;
    int32 B;

    bool operator==(const FTestState &Other) const {
      return A == Other.A && B == Other.B;
    }
  };

  auto SelectA = [](const FTestState &State) { return State.A; };
  auto SelectB = [](const FTestState &State) { return State.B; };

  int32 Computations = 0;

  auto ComplexSelector = createSelector<FTestState, int32>(
      std::make_tuple(SelectA, SelectB), [&Computations](int32 A, int32 B) {
        Computations++;
        return A + B;
      });

  FTestState State1{10, 20};

  // First call computes
  TestEqual("Initial computation", ComplexSelector(State1), 30);
  TestEqual("Computations count = 1", Computations, 1);

  // Second call with same state (by value equality) hits cache
  FTestState State2{10, 20};
  TestEqual("Cache hit computation", ComplexSelector(State2), 30);
  TestEqual("Computations count still = 1", Computations, 1);

  // Third call with new state computes
  FTestState State3{10, 25};
  TestEqual("New state computation", ComplexSelector(State3), 35);
  TestEqual("Computations count = 2", Computations, 2);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRtkApiSliceTest, "ForbocAI.Core.RTK.ApiSlice",
                                 EAutomationTestFlags::ApplicationContextMask |
                                     EAutomationTestFlags::EngineFilter)
bool FRtkApiSliceTest::RunTest(const FString &Parameters) {
  // 1. Define an API Endpoint
  ApiEndpoint<FString, int32> GetUserEndpoint;
  GetUserEndpoint.EndpointName = TEXT("getUser");
  GetUserEndpoint.ProvidesTags = {{TEXT("User"), TEXT("ID")}};

  // Mock HTTP Builder that resolves after parsing
  GetUserEndpoint.RequestBuilder = [](const FString &UserId) {
    return func::AsyncResult<func::HttpResult<int32>>::create(
        [UserId](auto Resolve, auto Reject) {
          if (UserId == TEXT("error")) {
            func::HttpResult<int32> FailedRes;
            FailedRes.IsSuccess = false;
            FailedRes.ErrorMessage = TEXT("Mock Network Failure");
            FailedRes.ResponseCode = 500;
            Resolve(FailedRes);
          } else {
            func::HttpResult<int32> SuccessRes;
            SuccessRes.IsSuccess = true;
            SuccessRes.Payload = 42;
            SuccessRes.ResponseCode = 200;
            Resolve(SuccessRes);
          }
        });
  };

  // 2. Register Endpoint in ApiSlice
  ApiSlice<FAppMockState> TestApi;
  TestApi.ReducerPath = TEXT("testApi");

  auto GetUserThunk = TestApi.injectEndpoint(GetUserEndpoint);

  TArray<FString> EventLog;
  std::function<AnyAction(const AnyAction &)> MockDispatch =
      [&EventLog](const AnyAction &Action) {
        EventLog.Add(Action.Type);
        return Action;
      };
  std::function<FAppMockState()> MockGetState = []() {
    return FAppMockState{};
  };

  // 3. Test Successful HTTP Call
  auto SuccessOp = GetUserThunk(TEXT("123"));
  SuccessOp(MockDispatch, MockGetState).execute();

  TestEqual("Dispatched pending first (API success)", EventLog[0],
            FString(TEXT("testApi/getUser/pending")));
  TestEqual("Dispatched fulfilled second (API success)", EventLog[1],
            FString(TEXT("testApi/getUser/fulfilled")));
  EventLog.Empty();

  // 4. Test Failed HTTP Call
  auto FailOp = GetUserThunk(TEXT("error"));
  FailOp(MockDispatch, MockGetState).execute();

  TestEqual("Dispatched pending first (API fail)", EventLog[0],
            FString(TEXT("testApi/getUser/pending")));
  TestEqual("Dispatched rejected second (API fail)", EventLog[1],
            FString(TEXT("testApi/getUser/rejected")));

  return true;
}
