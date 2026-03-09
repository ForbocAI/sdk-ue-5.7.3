# C++11 RTK (Redux Toolkit) Guide

This guide documents the RTK pattern implemented in `rtk.hpp` for strict C++11 Unreal Engine projects.

## Core Concepts

RTK provides predictable state management via:
- **Store**: Single source of truth holding application state
- **Slices**: Domain-specific state + reducer logic
- **Actions**: Plain data describing "what happened"
- **Thunks**: Async operations that dispatch actions
- **Selectors**: Pure functions to read derived state
- **Entity Adapters**: Normalized collections (ids + entities map)
- **Middleware**: Composable dispatch interceptors

## Creating a Slice

```cpp
#include "Core/rtk.hpp"

struct FMyState {
  FString Name;
  int32 Count;
};

inline rtk::Slice<FMyState> CreateMySlice() {
  rtk::SliceBuilder<FMyState> Builder(TEXT("mySlice"), FMyState{TEXT(""), 0});

  // Each case handles one action type
  Builder.createCase<FMyState>(
      TEXT("setInfo"),
      [](const FMyState &State, const rtk::Action<FMyState> &Action) {
        return Action.PayloadValue;  // Replace state entirely
      });

  Builder.createCase<int32>(
      TEXT("increment"),
      [](const FMyState &State, const rtk::Action<int32> &Action) {
        FMyState Next = State;
        Next.Count = State.Count + Action.PayloadValue;
        return Next;
      });

  return Builder.build();
}
```

## Actions

Actions are created via `ActionCreator<PayloadType>`:

```cpp
namespace Actions {
inline const rtk::ActionCreator<FMyState> &SetInfoActionCreator() {
  static const rtk::ActionCreator<FMyState> Creator(TEXT("mySlice/setInfo"));
  return Creator;
}

// Convenience wrapper
inline rtk::AnyAction SetInfo(const FMyState &Info) {
  return SetInfoActionCreator()(Info);
}
}
```

## Combining Reducers & Creating a Store

```cpp
struct FAppState {
  FMyState MyDomain;
  FOtherState Other;
};

inline rtk::EnhancedStore<FAppState> ConfigureMyStore() {
  auto RootReducer =
      rtk::combineReducers<FAppState>()
          .add(&FAppState::MyDomain, CreateMySlice().Reducer)
          .add(&FAppState::Other, CreateOtherSlice().Reducer)
          .build();

  FAppState Initial{FMyState{TEXT(""), 0}, FOtherState{}};
  return rtk::configureStore(Initial, RootReducer);
}
```

## Dispatching Actions

```cpp
auto Store = ConfigureMyStore();

// Sync action
Store.dispatch(Actions::SetInfo(FMyState{TEXT("hello"), 42}));

// Read state
FAppState Current = Store.getState();
```

## Thunks (Async Operations)

Thunks are functions that receive `dispatch` and `getState`:

```cpp
inline rtk::ThunkAction<FMyResult, FAppState>
myAsyncThunk(const FString &Input) {
  return rtk::ThunkAction<FMyResult, FAppState>(
      [Input](rtk::ThunkApi<FAppState> Api)
          -> func::AsyncResult<FMyResult> {
        // Dispatch a "pending" action
        Api.dispatch(Actions::SetPending());

        // Do async work
        return func::AsyncResult<FMyResult>::create(
            [Input, Api](std::function<void(FMyResult)> Resolve,
                         std::function<void(std::string)> Reject) {
              // ... HTTP call, SLM inference, etc.
              FMyResult Result;
              Result.Output = Input + TEXT("_processed");
              Api.dispatch(Actions::SetSuccess(Result));
              Resolve(Result);
            });
      });
}

// Usage:
func::AsyncResult<FMyResult> Result = Store.dispatch(myAsyncThunk(TEXT("data")));
```

## Entity Adapters

Normalized state for collections:

```cpp
struct FItem {
  FString Id;
  FString Name;
};

inline rtk::EntityAdapter<FItem> &GetItemAdapter() {
  static rtk::EntityAdapter<FItem> Adapter(
      [](const FItem &Item) { return Item.Id; });
  return Adapter;
}

// In reducers:
State.Entities = GetItemAdapter().addOne(State.Entities, NewItem);
State.Entities = GetItemAdapter().removeOne(State.Entities, ItemId);
State.Entities = GetItemAdapter().updateOne(State.Entities, ItemId, Updated);

// Selectors:
TArray<FItem> All = GetItemAdapter().getSelectors().selectAll(State.Entities);
func::Maybe<FItem> One = GetItemAdapter().getSelectors().selectById(State.Entities, Id);
```

## Selectors

Pure functions deriving data from state:

```cpp
inline func::Maybe<FItem> SelectActiveItem(const FMySliceState &State) {
  if (State.ActiveId.IsEmpty()) {
    return func::Maybe<FItem>();  // Nothing
  }
  return GetItemAdapter().getSelectors().selectById(State.Entities, State.ActiveId);
}
```

## Middleware

Intercept and transform dispatched actions:

```cpp
// Logger middleware
template <typename State>
rtk::Middleware<State> LoggerMiddleware() {
  return [](rtk::MiddlewareApi<State> Api) {
    return [Api](rtk::Dispatcher Next) {
      return [Api, Next](const rtk::AnyAction &Action) -> rtk::AnyAction {
        UE_LOG(LogTemp, Display, TEXT("Dispatch: %s"), *Action.Type);
        rtk::AnyAction Result = Next(Action);
        UE_LOG(LogTemp, Display, TEXT("Next state ready"));
        return Result;
      };
    };
  };
}
```

## Listener Middleware

React to specific actions (e.g., cascade cleanup):

```cpp
rtk::ListenerMiddleware<FAppState> Listeners;
Listeners.addListener(
    TEXT("mySlice/removeItem"),
    [](const rtk::AnyAction &Action, rtk::ListenerApi<FAppState> Api) {
      // Cascade: clear related state
      Api.dispatch(OtherActions::ClearRelated());
    });
```

## API Slice (RTK Query Equivalent)

Define API endpoints as thunk creators:

```cpp
inline auto getItems() {
  return Detail::MakeGet<TArray<FItem>>(TEXT("getItems"),
                                        SDKConfig::GetApiUrl() + TEXT("/items"));
}

inline auto postItem(const FItemRequest &Request) {
  TArray<FApiEndpointTag> Invalidates;
  Invalidates.Add(FApiEndpointTag{TEXT("Item"), TEXT("LIST")});
  return Detail::MakePost<FItemRequest, FItem>(
      TEXT("postItem"), SDKConfig::GetApiUrl() + TEXT("/items"),
      Request, Invalidates);
}
```

## C++11 Constraints

- No `auto` in lambda parameters — use explicit types
- No structured bindings — use `.field` access
- No `if constexpr` — use SFINAE or runtime checks
- No generic lambdas — all lambda params need explicit types
- Use `std::function<>` for type-erased callables

## File Structure

| File | Purpose |
|------|---------|
| `Core/rtk.hpp` | Complete RTK substrate |
| `Core/functional_core.hpp` | FP primitives (Maybe, Either, AsyncResult, Pipeline) |
| `SDKStore.h` | App state + configureStore |
| `*Slice.h` | Domain slices (NPC, Memory, Ghost, Bridge, Soul, Cortex) |
| `*SliceActions.h` | Action creators per domain |
| `Thunks.h` | All async thunk implementations |
| `API/APISlice.h` | HTTP endpoint definitions |
| `CLI/SDKOps.h` | Store-dispatching operations for CLI |
